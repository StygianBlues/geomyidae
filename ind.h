/*
 * Copy me if you can.
 * by 20h
 */

#ifndef IND_H
#define IND_H

#include <stdio.h>

extern int glfd;

typedef struct Elems Elems;
struct Elems {
	char **e;
	int num;
};

typedef struct Indexs Indexs;
struct Indexs {
	Elems **n;
	int num;
};

typedef struct filetype filetype;
struct filetype {
        char *end;
        char *type;
        void (* f)(int, char *, char *, char *, char *, char *, char *,
		char *, int);
};

filetype *gettype(char *filename);
void *xcalloc(size_t, size_t);
void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *str);
int xsendfile(int, int);
Indexs *scanfile(char *fname);
Elems *getadv(char *str);
int pendingbytes(int sock);
void waitforpendingbytes(int sock);
int printelem(int fd, Elems *el, char *file, char *base, char *addr, char *port);
void addindexs(Indexs *idx, Elems *el);
void addelem(Elems *e, char *s);
void freeindex(Indexs *i);
void freeelem(Elems *e);
char *smprintf(char *fmt, ...);
char *reverselookup(char *host);
void setcgienviron(char *file, char *path, char *port, char *base,
		char *args, char *sear, char *ohost, char *chost,
		int istls);
char *humansize(off_t n);
char *humantime(const time_t *clock);

#endif

