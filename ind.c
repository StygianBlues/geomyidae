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
	{"default", "0", handlebin},
	{"gph", "1", handlegph},
	{"cgi", "0", handlecgi},
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

void *
gmallocz(int l, int d)
{
	char *ret;

	ret = malloc(l);
	if(ret == nil) {
		perror("malloc");
		exit(1);
	}

	if(d)
		memset(ret, 0, l);

	return (void *)ret;
}

char *
gstrdup(char *str)
{
	char *ret;

	ret = strdup(str);
	if(ret == nil) {
		perror("strdup");
		exit(1);
	}

	return ret;
}

char *
readln(int fd)
{
	char *ret;
	int len;

	len = 1;

	ret = malloc(2);
	while(read(fd, &ret[len - 1], 1) > 0 && ret[len - 1] != '\n')
		ret = realloc(ret, ++len + 1);
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

	e->num++;
	e->e = realloc(e->e, sizeof(char *) * e->num);
	e->e[e->num - 1] = gmallocz(strlen(s) + 1, 0);
	strcpy(e->e[e->num - 1], s);

	return;
}

Elems *
getadv(char *str)
{
	char *b, *e;
	Elems *ret;

	ret = gmallocz(sizeof(Elems), 2);
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

Indexs *
scanfile(char *fname)
{
	char *ln;
	int fd;
	Indexs *ret;
	Elems *el;

	fd = open(fname, O_RDONLY);
	if(fd < 0)
		return nil;

	ret = gmallocz(sizeof(Indexs), 2);

	while((ln = readln(fd)) != nil) {
		el = getadv(ln);
		free(ln);
		if(el == nil)
			continue;

		ret->num++;
		ret->n = realloc(ret->n, sizeof(Elems) * ret->num);
		ret->n[ret->num - 1] = el;
		el = nil;
	}
	close(fd);

	if(ret->n == nil) {
		free(ret);
		return nil;
	}

	return ret;
}

void
tprintf(int fd, char *fmt, ...)
{
	va_list fmtargs;
	int fd2;
	FILE *fp;

	fd2 = dup(fd);
	fp = fdopen(fd2, "w");
	if(fp == nil) {
		perror("fdopen");
		return;
	}

	va_start(fmtargs, fmt);
	vfprintf(fp, fmt, fmtargs);
	va_end(fmtargs);

	fclose(fp);

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

        ret = "";

        va_start(fmtargs, fmt);
        size = vsnprintf(ret, 0, fmt, fmtargs);
        va_end(fmtargs);

        ret = gmallocz(++size, 2);
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
			rethost = strdup(client->h_name);
	}

	if(rethost == NULL)
		rethost = gstrdup(host);

	return rethost;
}

