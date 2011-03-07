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
			if(!strncmp(act->n[i]->e[3], "server", 6)) {
				free(act->n[i]->e[3]);
				act->n[i]->e[3] = gstrdup(addr);
			}
			if(!strncmp(act->n[i]->e[4], "port", 4)) {
				free(act->n[i]->e[4]);
				act->n[i]->e[4] = gstrdup(port);
			}
			tprintf(sock, "%.1s%s\t%s\t%s\t%s\r\n",
				act->n[i]->e[0], act->n[i]->e[1],
				act->n[i]->e[2], act->n[i]->e[3],
				act->n[i]->e[4]);

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
	char *p;

	USED(port);
	USED(base);

	p = strrchr(file, '/');
	if(p == nil)
		p = file;

	dup2(sock, 1);
	dup2(sock, 0);
	dup2(sock, 2);

	if(sear == nil)
		sear = "";

	switch(fork()) {
	case 0:
		execl(file, p, sear, args, (char *)nil);
	case -1:
		break;
	default:
		wait(NULL);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		break;
	}
}

