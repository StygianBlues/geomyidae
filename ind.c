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
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

/* for sendfile(2) and SIOCOUTQ */
#ifdef __linux__
#include <sys/sendfile.h>
#include <linux/sockios.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/types.h>
#include <sys/uio.h>
#endif

#include "arg.h"
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
        {"md", "0", handlebin},
        {"c", "0", handlebin},
        {"sh", "0", handlebin},
        {"patch", "0", handlebin},
        {nil, nil, nil},
};

int
pendingbytes(int sock)
{
	int pending;

	pending = 0;
	ioctl(sock, SIOCOUTQ, &pending);

	return pending;
}

void
waitforpendingbytes(int sock)
{

	while (pendingbytes(sock) > 0)
		usleep(10);
}

int
xsendfile(int fd, int sock)
{
	struct stat st;
	char *sendb;
	size_t bufsiz = BUFSIZ, count = 0;
	int len, sent, optval;

	USED(optval);

/* Tell the kernel to not send small packets on every write. */
#ifdef TCP_CORK
	optval = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_CORK, &optval, sizeof(int));
#endif

/* TCP_CORK for FreeBSD */
#ifdef TCP_NOPUSH
	optval = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH, &optval, sizeof(int));
#endif

/*
 * Enforce Nagle algorithm so big packets are sent out, if possible.
 */
#ifdef TCP_NODELAY
	optval = 0;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));
#endif

	if (fstat(fd, &st) >= 0) {
		if ((bufsiz = st.st_blksize) < BUFSIZ)
			bufsiz = BUFSIZ;
		count = st.st_size;
	}

/*
 * If we are on the said operating systems, use some special method for data
 * transfer.
 */
#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
	count = 0;
#endif

	if (count > 0) {
		sendb = xmalloc(bufsiz);
		while ((len = read(fd, sendb, bufsiz)) > 0) {
			while (len > 0) {
				if ((sent = send(sock, sendb, len, 0)) < 0) {
					free(sendb);
					return -1;
				}
				len -= sent;
			}
		}
		free(sendb);
		return 0;
	}

/* Different sendfile(2) implementations on different platforms. :/ */
#ifdef __linux__
	return sendfile(sock, fd, NULL, count);
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
	return sendfile(fd, sock, 0, count, NULL, NULL, 0);
#endif
	return -1;
}

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
	if (end == nil)
		return &type[0];
	end++;

	for (i = 0; type[i].end != nil; i++)
		if (!strcasecmp(end, type[i].end))
			return &type[i];

	return &type[0];
}

void
freeelem(Elems *e)
{
	if (e != nil) {
		if (e->e != nil) {
			for (;e->num > 0; e->num--)
				if (e->e[e->num - 1] != nil)
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
	if (i != nil) {
		if (i->n != nil) {
			for (;i->num > 0; i->num--)
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
	char *b, *e, *o, *bo;
	Elems *ret;

	ret = xcalloc(1, sizeof(Elems));

	if (strchr(str, '\t')) {
		addelem(ret, "i");
		addelem(ret, "Happy helping ☃ here: You tried to "
			"output a spurious tab character. This will "
			"break gopher. Please review your scripts. "
			"Have a nice day!");
		addelem(ret, "Err");
		addelem(ret, "server");
		addelem(ret, "port");

		return ret;
	}

	if (str[0] == '[') {
		o = xstrdup(str);
		b = o + 1;
		bo = b;
		while ((e = strchr(bo, '|')) != nil) {
			if (e != bo && e[-1] == '\\') {
				memmove(&e[-1], e, strlen(e));
				bo = e;
				continue;
			}
			*e = '\0';
			e++;
			addelem(ret, b);
			b = e;
			bo = b;
		}

		e = strchr(b, ']');
		if (e != nil) {
			*e = '\0';
			addelem(ret, b);
		}
		free(o);
		if (ret->e != nil && ret->num == 5)
			return ret;

		/* Invalid entry: Give back the whole line. */
		freeelem(ret);
		ret = xcalloc(1, sizeof(Elems));
	}

	b = str;
	if (*str == 't')
		b++;
	addelem(ret, "i");
	addelem(ret, b);
	addelem(ret, "Err");
	addelem(ret, "server");
	addelem(ret, "port");

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

	if (ret->n == nil) {
		free(ret);
		return nil;
	}

	return ret;
}

int
printelem(int fd, Elems *el, char *addr, char *port)
{
	if (!strcmp(el->e[3], "server")) {
		free(el->e[3]);
		el->e[3] = xstrdup(addr);
	}
	if (!strcmp(el->e[4], "port")) {
		free(el->e[4]);
		el->e[4] = xstrdup(port);
	}
	if (dprintf(fd, "%.1s%s\t%s\t%s\t%s\r\n", el->e[0], el->e[1], el->e[2],
			el->e[3], el->e[4]) < 0) {
		perror("printelem: dprintf");
		return -1;
	}
	return 0;
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

	if (inet_pton(AF_INET, host, &hoststr)) {
		client = gethostbyaddr((const void *)&hoststr,
				sizeof(hoststr), AF_INET);
		if (client != NULL)
			rethost = xstrdup(client->h_name);
	}

	if (rethost == NULL)
		rethost = xstrdup(host);

	return rethost;
}

void
setcgienviron(char *file, char *path, char *port, char *base, char *args,
		char *sear, char *ohost, char *chost)
{

	unsetenv("AUTH_TYPE");
	unsetenv("CONTENT_LENGTH");
	unsetenv("CONTENT_TYPE");
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
	/* TODO: Separate, if run like rest.dcgi. */
	setenv("PATH_INFO", file, 1);
	setenv("PATH_TRANSLATED", path, 1);
	setenv("QUERY_STRING", args, 1);
	setenv("REMOTE_ADDR", chost, 1);
	/*
	 * Don't do a reverse lookup on every call. Only do when needed, in
	 * the script. The RFC allows us to set the IP to the value.
	 */
	setenv("REMOTE_HOST", chost, 1);
	unsetenv("REMOTE_IDENT");
	unsetenv("REMOTE_USER");
	/* Make PHP happy. */
	setenv("REDIRECT_STATUS", "", 1);
	/*
	 * Only GET is possible in gopher. POST emulation would be really
	 * ugly.
	 */
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("SCRIPT_NAME", file, 1);
	setenv("SERVER_NAME", ohost, 1);
	setenv("SERVER_PORT", port, 1);
	setenv("SERVER_PROTOCOL", "gopher/1.0", 1);
	setenv("SERVER_SOFTWARE", "geomyidae", 1);

	setenv("X_GOPHER_SEARCH", sear, 1);
}

