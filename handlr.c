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
#include "ind.h"
#include "arg.h"

void
handledir(int sock, char *path, char *port, char *base, char *args,
		char *sear)
{
	char *pa, *file, *e, *addr, *par, *b;
	struct dirent **dirent;
	int ndir, i;
	struct stat st;
	filetype *type;

	USED(sear);
	addr = nil;

	pa = gstrdup(path);
	e = strrchr(pa, '/');
	if(e != nil) {
		*e = '\0';

		if(args == nil) {
			addr = gmallocz(512, 2);
                        if(gethostname(addr, 512) == -1) {
                                perror("gethostname");
				free(addr);
				free(pa);
                                return;
                        }
		} else
			addr = gstrdup(args);

                par = gstrdup(pa);
                b = strrchr(par + strlen(base), '/');
                if(b != nil) {
			*b = '\0';
                        tprintf(sock, "1..\t%s\t%s\t%s\r\n",
                                par + strlen(base), addr, port);
                }
		free(par);

		ndir = scandir(pa, &dirent, 0, alphasort);
		if(ndir < 0) {
			perror("scandir");
			free(addr);
			free(pa);
			return;
		} else {
			for(i = 0; i < ndir; i++) {
				if(dirent[i]->d_name[0] == '.') {
					free(dirent[i]);
					continue;
				}

				type = gettype(dirent[i]->d_name);
				file = smprintf("%s/%s", pa,
						dirent[i]->d_name);
				if(stat(file, &st) >= 0 && S_ISDIR(st.st_mode))
					type = gettype("index.gph");
				e = file + strlen(base);
				tprintf(sock, "%c%s\t%s\t%s\t%s\r\n", *type->type,
					dirent[i]->d_name, e, addr, port);
				free(file);
				free(dirent[i]);
			}
			free(dirent);
		}
		tprintf(sock, "\r\n");
	}

	if(addr != nil)
		free(addr);
	free(pa);
}

void
handlegph(int sock, char *file, char *port, char *base, char *args,
		char *sear)
{
	Indexs *act;
	int i;
	char addr[512];

	USED(base);
	USED(args);
	USED(sear);

	act = scanfile(file);
	if(act != nil) {
                if(args == nil) {
                        if(gethostname(addr, sizeof(addr)) == -1) {
                                perror("gethostname");
                                return;
                        }
                } else
                        snprintf(addr, sizeof(addr), "%s", args);

		for(i = 0; i < act->num; i++) {
			printelem(sock, act->n[i], addr, port);
			freeelem(act->n[i]);
			act->n[i] = nil;
		}
		tprintf(sock, "\r\n.\r\n\r\n");

		freeindex(act);
	}
}

void
handlebin(int sock, char *file, char *port, char *base, char *args,
		char *sear)
{
	char sendb[1024];
	int len, fd;

	len = -1;
	USED(port);
	USED(base);
	USED(args);
	USED(sear);

	fd = open(file, O_RDONLY);
	if(fd >= 0) {
		while((len = read(fd, sendb, sizeof(sendb))) > 0)
			send(sock, sendb, len, 0);
		close(fd);
	}
}

void
handlecgi(int sock, char *file, char *port, char *base, char *args,
		char *sear)
{
	char *p, *path;

	USED(base);
	USED(port);

	path = gstrdup(file);
	p = strrchr(path, '/');
	if (p != nil)
		p[1] = '\0';
	else {
		free(path);
		path = nil;
	}

	p = strrchr(file, '/');
	if (p == nil)
		p = file;

	if(sear == nil)
		sear = "";

	dup2(sock, 0);
	dup2(sock, 1);
	dup2(sock, 2);
	switch(fork()) {
	case 0:
		if (path != nil)
			chdir(path);
		execl(file, p, sear, args, (char *)nil);
	case -1:
		break;
	default:
		wait(NULL);
		if (path != nil)
			free(path);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		break;
	}
}

void
handledcgi(int sock, char *file, char *port, char *base, char *args,
		char *sear)
{
	char *p, *ln, addr[512];
	int outpipe[2];
	Elems *el;

	USED(base);

	if(pipe(outpipe) < 0)
		return;

	p = strrchr(file, '/');
	if(p == nil)
		p = file;

	if(args == nil) {
		if(gethostname(addr, sizeof(addr)) == -1) {
			perror("gethostname");
			return;
		}
	} else
		snprintf(addr, sizeof(addr), "%s", args);

	if(sear == nil)
		sear = "";

	dup2(sock, 0);
	dup2(sock, 2);
	switch(fork()) {
	case 0:
		dup2(outpipe[1], 1);
		close(outpipe[0]);
		execl(file, p, sear, args, (char *)nil);
	case -1:
		break;
	default:
		dup2(sock, 1);
		close(outpipe[1]);

		while((ln = readln(outpipe[0])) != nil) {
			el = getadv(ln);
			if (el == nil)
				continue;

			printelem(sock, el, addr, port);
			freeelem(el);
		}
		tprintf(sock, "\r\n.\r\n\r\n");

		wait(NULL);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		break;
	}
}
