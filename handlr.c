/*
 * Copy me if you can.
 * by 20h
 */

#include <unistd.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <errno.h>

#include "ind.h"
#include "arg.h"

void
handledir(int sock, char *path, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost, int istls)
{
	char *pa, *file, *e, *par, *b;
	struct dirent **dirent;
	int ndir, i, ret = 0;
	struct stat st;
	filetype *type;

	USED(args);
	USED(sear);

	pa = xstrdup(path);
	e = pa + strlen(pa) - 1;
	if (e[0] == '/')
		*e = '\0';

	par = xstrdup(pa);
	b = strrchr(par + strlen(base), '/');
	if (b != NULL) {
		*b = '\0';
		dprintf(sock, "1..\t%s\t%s\t%s\r\n",
			par + strlen(base), ohost, port);
	}
	free(par);

	ndir = scandir(pa[0] ? pa : ".", &dirent, 0, alphasort);
	if (ndir < 0) {
		perror("scandir");
		free(pa);
		return;
	} else {
		for (i = 0; i < ndir && ret >= 0; i++) {
			if (dirent[i]->d_name[0] == '.') {
				free(dirent[i]);
				continue;
			}

			type = gettype(dirent[i]->d_name);
			file = smprintf("%s/%s", pa,
					dirent[i]->d_name);
			if (stat(file, &st) >= 0 && S_ISDIR(st.st_mode))
				type = gettype("index.gph");
			e = file + strlen(base);
			ret = dprintf(sock,
					"%c%-50.50s %10s %16s\t%s\t%s\t%s\r\n",
					*type->type,
					dirent[i]->d_name,
					humansize(st.st_size),
					humantime(&(st.st_mtim.tv_sec)),
					e, ohost, port);
			free(file);
			free(dirent[i]);
		}
		free(dirent);
	}
	dprintf(sock, ".\r\n");

	free(pa);
}

void
handlegph(int sock, char *file, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost, int istls)
{
	Indexs *act;
	int i, ret = 0;

	USED(args);
	USED(sear);

	act = scanfile(file);
	if (act != NULL) {
		for (i = 0; i < act->num && ret >= 0; i++) {
			ret = printelem(sock, act->n[i], file, base, ohost, port);
			freeelem(act->n[i]);
			act->n[i] = NULL;
		}
		dprintf(sock, ".\r\n");

		freeindex(act);
	}
}

void
handlebin(int sock, char *file, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost, int istls)
{
	int fd;

	USED(port);
	USED(base);
	USED(args);
	USED(sear);
	USED(ohost);

	fd = open(file, O_RDONLY);
	if (fd >= 0) {
		if (xsendfile(fd, sock) < 0)
			perror("sendfile");
		close(fd);
	}
}

void
handlecgi(int sock, char *file, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost, int istls)
{
	char *p, *path;

	USED(base);
	USED(port);

	path = xstrdup(file);
	p = strrchr(path, '/');
	if (p != NULL)
		p[1] = '\0';
	else {
		free(path);
		path = NULL;
	}

	p = strrchr(file, '/');
	if (p == NULL)
		p = file;

	if (sear == NULL)
		sear = "";
	if (args == NULL)
		args = "";

	dup2(sock, 0);
	dup2(sock, 1);
	dup2(sock, 2);
	switch (fork()) {
	case 0:
		if (path != NULL) {
			if (chdir(path) < 0)
				break;
		}

		setcgienviron(p, file, port, base, args, sear, ohost, chost,
				istls);

		if (execl(file, p, sear, args, ohost, port,
				(char *)NULL) == -1) {
			perror("execl");
			_exit(1);
		}
	case -1:
		perror("fork");
		break;
	default:
		wait(NULL);
		free(path);
		break;
	}
}

void
handledcgi(int sock, char *file, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost, int istls)
{
	FILE *fp;
	char *p, *path, *ln = NULL;
	size_t linesiz = 0;
	ssize_t n;
	int outpipe[2], ret = 0;
	Elems *el;

	if (pipe(outpipe) < 0)
		return;

	path = xstrdup(file);
	p = strrchr(path, '/');
	if (p != NULL)
		p[1] = '\0';
	else {
		free(path);
		path = NULL;
	}

	p = strrchr(file, '/');
	if (p == NULL)
		p = file;

	if (sear == NULL)
		sear = "";
	if (args == NULL)
		args = "";

	while (dup2(sock, 0) < 0 && errno == EINTR);
	while (dup2(sock, 2) < 0 && errno == EINTR);
	switch (fork()) {
	case 0:
		while(dup2(outpipe[1], 1) < 0 && errno == EINTR);
		close(outpipe[0]);
		if (path != NULL) {
			if (chdir(path) < 0)
				break;
		}

		setcgienviron(p, file, port, base, args, sear, ohost, chost,
				istls);

		if (execl(file, p, sear, args, ohost, port,
				(char *)NULL) == -1) {
			perror("execl");
			_exit(1);
		}
		break;
	case -1:
		perror("fork");
		break;
	default:
		while(dup2(sock, 1) < 0 && errno == EINTR);
		close(outpipe[1]);

		if (!(fp = fdopen(outpipe[0], "r"))) {
			perror("fdopen");
			break;
		}

		while ((n = getline(&ln, &linesiz, fp)) > 0 && ret >= 0) {
			if (ln[n - 1] == '\n')
				ln[--n] = '\0';

			el = getadv(ln);
			if (el == NULL)
				continue;

			ret = printelem(sock, el, file, base, ohost, port);
			freeelem(el);
		}
		if (ferror(fp))
			perror("getline");
		dprintf(sock, ".\r\n");

		free(ln);
		free(path);
		fclose(fp);
		wait(NULL);
		break;
	}
}

