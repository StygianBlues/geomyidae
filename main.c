/*
 * Copy me if you can.
 * by 20h
 */

#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <arpa/inet.h>

#include "ind.h"
#include "handlr.h"
#include "arg.h"

enum {
	NOLOG	= 0,
	FILES	= 1,
	DIRS	= 2,
	HTTP	= 4,
	ERRORS	= 8,
	CONN	= 16
};

int glfd = -1;
int loglvl = 15;
int running = 1;
int listfd = -1;
char *logfile = nil;

char *argv0;
char *stdbase = "/var/gopher";
char *stdport = "70";
char *indexf[] = {"/index.gph", "/index.cgi", "/index.dcgi"};
char *err = "3Sorry, but the requested token '%s' could not be found.\tErr"
	    "\tlocalhost\t70\r\n.\r\n\r\n";
char *htredir = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
		"	\"DTD/xhtml-transitional.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\">\n"
		"  <head>\n"
		"    <title>gopher redirect</title>\n"
		"\n"
		"    <meta http-equiv=\"Refresh\" content=\"1;url=%s\" />\n"
		"  </head>\n"
		"  <body>\n"
		"    This page is for redirecting you to: <a href=\"%s\">%s</a>.\n"
		"  </body>\n"
		"</html>\n";

int
dropprivileges(struct group *gr, struct passwd *pw)
{
	if(gr != nil)
		if(setgroups(1, &gr->gr_gid) != 0 || setgid(gr->gr_gid) != 0)
			return -1;
	if(pw != nil) {
		if(gr == nil) {
			if(setgroups(1, &pw->pw_gid) != 0 ||
			    setgid(pw->pw_gid) != 0)
				return -1;
		}
		if(setuid(pw->pw_uid) != 0)
			return -1;
	}

	return 0;
}

char *
securepath(char *p, int len)
{
	int i;

	if(len < 2)
		return p;

	for(i = 1; i < strlen(p); i++) {
		if(p[i - 1] == '.' && p[i] == '.') {
			if(p[i - 2] == '/')
				p[i] = '/';
			if(p[i + 1] == '/')
				p[i] = '/';
			if(len == 2)
				p[i] = '/';
		}
	}

	return p;
}

void
logentry(char *host, char *port, char *qry, char *status)
{
	time_t tim;
	struct tm *ptr;
	char timstr[128], *ahost;

        if(glfd >= 0) {
		tim = time(0);
		ptr = localtime(&tim);

		ahost = reverselookup(host);
		strftime(timstr, sizeof(timstr), "%a %b %d %H:%M:%S %Z %Y",
					ptr);

		dprintf(glfd, "[%s|%s:%s] %s (%s)\n",
			timstr, ahost, port, qry, status);
		free(ahost);
        }

	return;
}

void
handlerequest(int sock, char *base, char *ohost, char *port, char *clienth,
			char *clientp)
{
	struct stat dir;
	char recvc[1025], recvb[1025], path[1025], *args, *sear, *c;
	int len, fd, i;
	filetype *type;

	memset(&dir, 0, sizeof(dir));
	memset(recvb, 0, sizeof(recvb));
	memset(recvc, 0, sizeof(recvc));
	args = nil;

	len = recv(sock, recvb, sizeof(recvb)-1, 0);
	if (len <= 0)
		return;

	c = strchr(recvb, '\r');
	if(c)
		c[0] = '\0';
	c = strchr(recvb, '\n');
	if(c)
		c[0] = '\0';
	memmove(recvc, recvb, len+1);

	if(!strncmp(recvb, "URL:", 4)) {
		len = snprintf(path, sizeof(path), htredir,
				recvb + 4, recvb + 4, recvb + 4);
		if(len > sizeof(path))
			len = sizeof(path);
		send(sock, path, len, 0);
		if(loglvl & HTTP)
			logentry(clienth, clientp, recvc, "HTTP redirect");
		return;
	}

	sear = strchr(recvb, '\t');
	if(sear != nil)
		*sear++ = '\0';
	args = strchr(recvb, '?');
	if(args != nil)
		*args++ = '\0';

	securepath(recvb, len - 2);
	if(strlen(recvb) == 0) {
		recvb[0] = '/';
		recvb[1] = '\0';
	}

	snprintf(path, sizeof(path), "%s%s", base, recvb);

	fd = -1;
	if(stat(path, &dir) != -1 && S_ISDIR(dir.st_mode)) {
		for(i = 0; i < sizeof(indexf)/sizeof(indexf)[0]; i++) {
			strncat(path, indexf[i], sizeof(path) - strlen(path));
			fd = open(path, O_RDONLY);
			if(fd >= 0)
				break;
			path[strlen(path)-strlen(indexf[i])] = '\0';
		}
	} else {
		fd = open(path, O_RDONLY);
		if(fd < 0) {
			if(loglvl & ERRORS)
				logentry(clienth, clientp, recvc, strerror(errno));
			return;
		}
	}

	if(fd >= 0) {
		close(fd);
		if(loglvl & FILES)
			logentry(clienth, clientp, recvc, "serving");

		c = strrchr(path, '/');
		if(c == nil)
			c = path;
		type = gettype(c);
		type->f(sock, path, port, base, args, sear, ohost);
	} else {
		if(S_ISDIR(dir.st_mode)) {
			handledir(sock, path, port, base, args, sear, ohost);
			if(loglvl & DIRS)
				logentry(clienth, clientp, recvc,
							"dir listing");
			return;
		}

		dprintf(sock, err, recvc);
		if(loglvl & ERRORS)
			logentry(clienth, clientp, recvc, "not found");
	}

	return;
}

void
sighandler(int sig)
{
	switch(sig) {
	case SIGCHLD:
		while(waitpid(-1, NULL, WNOHANG) > 0);
		break;
	case SIGHUP:
	case SIGINT:
	case SIGQUIT:
	case SIGABRT:
	case SIGTERM:
	case SIGKILL:
		if(logfile != nil)
			stoplogging(glfd);
		if(listfd >= 0) {
			shutdown(listfd, SHUT_RDWR);
			close(listfd);
		}
		exit(0);
		break;
	default:
		break;
	}
}

void
initsignals(void)
{
	signal(SIGCHLD, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGKILL, sighandler);

	signal(SIGPIPE, SIG_IGN);
}

int
getlistenfd(struct addrinfo *hints, char *bindip, char *port)
{
	char addstr[INET6_ADDRSTRLEN];
	struct addrinfo *ai, *rp;
	void *sinaddr;
	int on, listfd;

	listfd = -1;

	if(getaddrinfo(bindip, port, hints, &ai))
		return -1;
	if(ai == nil)
		return -1;

	on = 1;
	for(rp = ai; rp != nil; rp = rp->ai_next) {
		listfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if(listfd < 0)
			continue;
		if(setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, &on,
					sizeof(on)) < 0) {
			break;
		}

		sinaddr = (rp->ai_family == AF_INET) ?
		          (void *)&((struct sockaddr_in *)rp->ai_addr)->sin_addr :
		          (void *)&((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;

		if(bind(listfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			if(loglvl & CONN && inet_ntop(rp->ai_family, sinaddr,
			   addstr, sizeof(addstr)))
				logentry(addstr, port, "-", "listening");
			break;
		}
		close(listfd);
		if(loglvl & CONN && inet_ntop(rp->ai_family, sinaddr,
		   addstr, sizeof(addstr)))
			logentry(addstr, port, "-", "could not bind");
	}
	if(rp == nil)
		return -1;
	freeaddrinfo(ai);

	return listfd;
}

void
usage(void)
{
	dprintf(2, "usage: %s [-4] [-6] [-d] [-l logfile] [-v loglvl] "
		   "[-b base] [-p port] [-o sport] [-u user] [-g group] "
		   "[-h host] [-i IP]\n",
		   argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct sockaddr_storage clt;
	socklen_t cltlen;
	int sock, dofork, v4, v6;
	char *port, *base, clienth[NI_MAXHOST], clientp[NI_MAXSERV];
	char *user, *group, *bindip, *ohost, *sport;
	struct passwd *us;
	struct group *gr;

	base = stdbase;
	port = stdport;
	dofork = 1;
	user = nil;
	group = nil;
	us = nil;
	gr = nil;
	bindip = nil;
	ohost = nil;
	sport = port;
	v4 = 1;
	v6 = 1;

	ARGBEGIN {
	case '4':
		v6 = 0;
		break;
	case '6':
		v4 = 0;
		break;
	case 'b':
		base = EARGF(usage());
		break;
	case 'p':
		port = EARGF(usage());
		break;
	case 'l':
		logfile = EARGF(usage());
		break;
	case 'd':
		dofork = 0;
		break;
	case 'v':
		loglvl = atoi(EARGF(usage()));
		break;
	case 'u':
		user = EARGF(usage());
		break;
	case 'g':
		group = EARGF(usage());
		break;
	case 'i':
		bindip = EARGF(usage());
		break;
	case 'h':
		ohost = EARGF(usage());
		break;
	case 'o':
		sport = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;

	if(ohost == nil) {
		ohost = xcalloc(1, 513);
		if(gethostname(ohost, 512) < 0) {
			perror("gethostname");
			free(ohost);
			return 1;
		}
	} else {
		ohost = xstrdup(ohost);
	}

	if(group != nil) {
		if((gr = getgrnam(group)) == nil) {
			perror("no such group");
			return 1;
		}
	}

	if(user != nil) {
		if((us = getpwnam(user)) == nil) {
			perror("no such user");
			return 1;
		}
	}

	if(dofork && fork() != 0)
		return 0;

	if(logfile != nil) {
		glfd = initlogging(logfile);
		if(glfd < 0) {
			perror("initlogging");
			return 1;
		}
	} else if(!dofork) {
		glfd = 1;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	if(bindip)
		hints.ai_flags |= AI_CANONNAME;

	listfd = -1;
	if(v6) {
		hints.ai_family = PF_INET6;
		listfd = getlistenfd(&hints, bindip, port);
		if(!v4 && listfd < 0) {
			perror("getlistenfd6");
			return 1;
		}
	}
	if(v4 && listfd < 0) {
		hints.ai_family = PF_INET;
		listfd = getlistenfd(&hints, bindip, port);
		if(listfd < 0) {
			perror("getlistenfd4");
			return 1;
		}
	}
	if(listfd < 0) {
		perror("You did not specify a TCP port.");
		return 1;
	}

	if(listen(listfd, 255)) {
		perror("listen");
		close(listfd);
		return 1;
	}

	if(dropprivileges(gr, us) < 0) {
		perror("dropprivileges");
		close(listfd);
		return 1;
	}

	initsignals();

	cltlen = sizeof(clt);
	while(running) {
		sock = accept(listfd, (struct sockaddr *)&clt, &cltlen);
		if(sock < 0) {
			switch(errno) {
			case ECONNABORTED:
			case EINTR:
				if (!running) {
					shutdown(listfd, SHUT_RDWR);
					close(listfd);
					return 0;
				}
				continue;
			default:
				perror("accept");
				close(listfd);
				return 1;
			}
		}

		getnameinfo((struct sockaddr *)&clt, cltlen, clienth,
				sizeof(clienth), clientp, sizeof(clientp),
				NI_NUMERICHOST|NI_NUMERICSERV);

		if(loglvl & CONN)
			logentry(clienth, clientp, "-", "connected");

		switch(fork()) {
		case -1:
			perror("fork");
			shutdown(sock, SHUT_RDWR);
			break;
		case 0:
			signal(SIGHUP, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			signal(SIGALRM, SIG_DFL);

			handlerequest(sock, base, ohost, sport, clienth,
						clientp);
			shutdown(sock, SHUT_RDWR);
			close(sock);
			return 0;
		default:
			break;
		}
		close(sock);
		if(loglvl & CONN)
			logentry(clienth, clientp, "-", "disconnected");
	}

	shutdown(listfd, SHUT_RDWR);
	close(listfd);
	if(logfile != nil)
		stoplogging(glfd);
	free(ohost);

	return 0;
}

