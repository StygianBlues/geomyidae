/*
 * Copy me if you can.
 * by 20h
 */

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ind.h"
#include "handlr.h"

filetype type[] = {
	{"default", "9", handlebin},
	{"gph", "1", handlegph},
	{"cgi", "0", handlecgi},
	{"dcgi", "1", handledcgi},
	{"bin", "9", handlebin},
	{"tgz", "9", handlebin},
	{"gz", "9", handlebin},
        {"jpg", "I", handlebin},
        {"gif", "g", handlebin},
        {"png", "I", handlebin},
        {"bmp", "I", handlebin},
        {"txt", "0", handlebin},
        {"html", "0", handlebin},
        {"htm", "0", handlebin},
        {"xhtml", "0", handlebin},
        {"css", "0", handlebin},
        {nil, nil, nil},
};

void *
xcalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size))) {
		perror("calloc");
		exit(1);
	}

	return p;
}

void *
xmalloc(size_t size)
{
	void *p;

	if (!(p = malloc(size))) {
		perror("malloc");
		exit(1);
	}

	return p;
}

void *
xrealloc(void *ptr, size_t size)
{
	if (!(ptr = realloc(ptr, size))) {
		perror("realloc");
		exit(1);
	}

	return ptr;
}

char *
xstrdup(const char *str)
{
	char *ret;

	if (!(ret = strdup(str))) {
		perror("strdup");
		exit(1);
	}

	return ret;
}

filetype *
gettype(char *filename)
{
	char *end;
	int i;

	end = strrchr(filename, '.');
	if(end == nil)
		return &type[0];
	end++;

	for(i = 0; type[i].end != nil; i++)
		if(!strncasecmp(end, type[i].end, strlen(type[i].end)))
			return &type[i];

	return &type[0];
}

char *
readln(int fd)
{
	char *ret;
	int len;

	len = 1;

	ret = xmalloc(2);
	while(read(fd, &ret[len - 1], 1) > 0 && ret[len - 1] != '\n')
		ret = xrealloc(ret, ++len + 1);
	if(ret[len - 1] != '\n') {
		free(ret);
		return nil;
	}
	ret[len - 1] = '\0';

	return ret;
}

void
freeelem(Elems *e)
{

	if(e != nil) {
		if(e->e != nil) {
			for(;e->num > 0; e->num--)
				if(e->e[e->num - 1] != nil)
					free(e->e[e->num - 1]);
			free(e->e);
		}
		free(e);
	}
	return;
}

void
freeindex(Indexs *i)
{

	if(i != nil) {
		if(i->n != nil) {
			for(;i->num > 0; i->num--)
				if(i->n[i->num - 1] != nil)
					freeelem(i->n[i->num - 1]);
			free(i->n);
		}
		free(i);
	}

	return;
}

void
addelem(Elems *e, char *s)
{
	int slen;

	slen = strlen(s) + 1;

	e->num++;
	e->e = xrealloc(e->e, sizeof(char *) * e->num);
	e->e[e->num - 1] = xcalloc(1, slen);
	strncpy(e->e[e->num - 1], s, slen - 1);

	return;
}

Elems *
getadv(char *str)
{
	char *b, *e;
	Elems *ret;

	ret = xcalloc(1, sizeof(Elems));
	if(*str != '[') {
		b = str;
		if(*str == 't')
			b++;
		addelem(ret, "i");
		addelem(ret, b);
		addelem(ret, "Err");
		addelem(ret, "server");
		addelem(ret, "port");

		return ret;
	}

	b = str + 1;
	while((e = strchr(b, '|')) != nil) {
		*e = '\0';
		e++;
		addelem(ret, b);
		b = e;
	}

	e = strchr(b, ']');
	if(e != nil) {
		*e = '\0';
		addelem(ret, b);
	}
	if(ret->e == nil) {
		free(ret);
		return nil;
	}

	return ret;
}

void
addindexs(Indexs *idx, Elems *el)
{

	idx->num++;
	idx->n = xrealloc(idx->n, sizeof(Elems) * idx->num);
	idx->n[idx->num - 1] = el;

	return;
}

Indexs *
scanfile(char *fname)
{
	char *ln = NULL;
	size_t linesiz = 0;
	ssize_t n;
	FILE *fp;
	Indexs *ret;
	Elems *el;

	if (!(fp = fopen(fname, "r")))
		return nil;

	ret = xcalloc(1, sizeof(Indexs));

	while ((n = getline(&ln, &linesiz, fp)) > 0) {
		if (ln[n - 1] == '\n')
			ln[--n] = '\0';
		el = getadv(ln);
		if(el == nil)
			continue;

		addindexs(ret, el);
	}
	if (ferror(fp))
		perror("getline");
	free(ln);
	fclose(fp);

	if(ret->n == nil) {
		free(ret);
		return nil;
	}

	return ret;
}

void
printelem(int fd, Elems *el, char *addr, char *port)
{

	if(!strncmp(el->e[3], "server", 6)) {
		free(el->e[3]);
		el->e[3] = xstrdup(addr);
	}
	if(!strncmp(el->e[4], "port", 4)) {
		free(el->e[4]);
		el->e[4] = xstrdup(port);
	}
	dprintf(fd, "%.1s%s\t%s\t%s\t%s\r\n", el->e[0], el->e[1], el->e[2],
			el->e[3], el->e[4]);

	return;
}

int
initlogging(char *logf)
{
	int fd;

	fd = open(logf, O_APPEND | O_WRONLY | O_CREAT, 0644);

	return fd;
}

void
stoplogging(int fd)
{

	close(fd);

	return;
}

char *
smprintf(char *fmt, ...)
{
        va_list fmtargs;
        char *ret;
        int size;

        va_start(fmtargs, fmt);
        size = vsnprintf(NULL, 0, fmt, fmtargs);
        va_end(fmtargs);

        ret = xcalloc(1, ++size);
        va_start(fmtargs, fmt);
        vsnprintf(ret, size, fmt, fmtargs);
        va_end(fmtargs);

        return ret;
}

char *
reverselookup(char *host)
{
	struct in_addr hoststr;
	struct hostent *client;
	char *rethost;

	rethost = NULL;

	if(inet_pton(AF_INET, host, &hoststr)) {
		client = gethostbyaddr((const void *)&hoststr,
				sizeof(hoststr), AF_INET);
		if(client != NULL)
			rethost = xstrdup(client->h_name);
	}

	if(rethost == NULL)
		rethost = xstrdup(host);

	return rethost;
}

