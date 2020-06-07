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
#include <sys/select.h>
#include <sys/time.h>
#include <tls.h>

#include "ind.h"
#include "handlr.h"
#include "arg.h"

enum {
	NOLOG	= 0,
	FILES	= 1,
	DIRS	= 2,
	HTTP	= 4,
	ERRORS	= 8,
	CONN	= 16,
	GPLUS	= 32
};

int glfd = -1;
int loglvl = 47;
int *listfds = NULL;
int nlistfds = 0;
int revlookup = 1;
char *logfile = NULL;

char *argv0;
char stdbase[] = "/var/gopher";
char *stdport = "70";
char *indexf[] = {"/index.gph", "/index.cgi", "/index.dcgi", "/index.bin"};
char *nocgierr = "3Sorry, execution of the token '%s' was requested, but this "
	    "is disabled in the server configuration.\tErr"
	    "\tlocalhost\t70\r\n";
char *notfounderr = "3Sorry, but the requested token '%s' could not be found.\tErr"
	    "\tlocalhost\t70\r\n";
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
char *selinval ="3Happy helping ☃ here: "
		"Sorry, your selector does not start with / or contains '..'. "
		"That's illegal here.\tErr\tlocalhost\t70\r\n.\r\n\r\n";

int
dropprivileges(struct group *gr, struct passwd *pw)
{
	if (gr != NULL)
		if (setgroups(1, &gr->gr_gid) != 0 || setgid(gr->gr_gid) != 0)
			return -1;
	if (pw != NULL) {
		if (gr == NULL) {
			if (setgroups(1, &pw->pw_gid) != 0 ||
			    setgid(pw->pw_gid) != 0)
				return -1;
		}
		if (setuid(pw->pw_uid) != 0)
			return -1;
	}

	return 0;
}

void
logentry(char *host, char *port, char *qry, char *status)
{
	time_t tim;
	struct tm *ptr;
	char timstr[128], *ahost;

        if (glfd >= 0) {
		tim = time(0);
		ptr = gmtime(&tim);

		ahost = revlookup ? reverselookup(host) : host;
		strftime(timstr, sizeof(timstr), "%F %T %z", ptr);

		dprintf(glfd, "[%s|%s|%s|%s] %s\n",
			timstr, ahost, port, status, qry);
		if (revlookup)
			free(ahost);
        }

	return;
}

void
handlerequest(int sock, char *req, int rlen, char *base, char *ohost,
	      char *port, char *clienth, char *clientp, int nocgi)
{
	struct stat dir;
	char recvc[1025], recvb[1025], path[1025], *args = NULL, *sear, *c;
	int len = 0, fd, i, maxrecv;
	filetype *type;

	memset(&dir, 0, sizeof(dir));
	memset(recvb, 0, sizeof(recvb));
	memset(recvc, 0, sizeof(recvc));

	maxrecv = sizeof(recvb) - 1;
	if (rlen > maxrecv || rlen < 0)
		return;
	memcpy(recvb, req, rlen);

	c = strchr(recvb, '\r');
	if (c)
		c[0] = '\0';
	c = strchr(recvb, '\n');
	if (c)
		c[0] = '\0';
	sear = strchr(recvb, '\t');
	if (sear != NULL) {
		*sear++ = '\0';

		/*
		 * This is a compatibility layer to geomyidae for users using
		 * the original gopher(1) client. Gopher+ is by default
		 * requesting the metadata. We are using a trick in the
		 * gopher(1) parsing code to jump back to gopher compatibility
		 * mode. DO NOT ADD ANY OTHER GOPHER+ SUPPORT. GOPHER+ IS
		 * CRAP.
		 */
		if (*sear == '+' || *sear == '$' || *sear == '!' || *sear == '\0') {
			if (loglvl & GPLUS)
				logentry(clienth, clientp, recvb, "gopher+ redirect");
			dprintf(sock, "+-2\r\n");
			dprintf(sock, "+INFO: 1gopher+\t\t%s\t%s\r\n",
					ohost, port);
			dprintf(sock, "+ADMIN:\r\n Admin: Me\r\n");
			return;
		}
	}

	memmove(recvc, recvb, rlen+1);

	if (!strncmp(recvb, "URL:", 4)) {
		len = snprintf(path, sizeof(path), htredir,
				recvb + 4, recvb + 4, recvb + 4);
		if (len > sizeof(path))
			len = sizeof(path);
		write(sock, path, len);
		if (loglvl & HTTP)
			logentry(clienth, clientp, recvc, "HTTP redirect");
		return;
	}

	/*
	 * Valid cases in gopher we overwrite here, but could be used for
	 * other geomyidae features:
	 *
	 *	request string = "?..." -> "/?..."
	 *	request string = "" -> "/"
	 *	request string = "somestring" -> "/somestring"
	 *
	 * Be careful, when you consider those special cases to be used
	 * for some feature. You can easily do good and bad.
	 */

	args = strchr(recvb, '?');
	if (args != NULL)
		*args++ = '\0';

	if (recvb[0] == '\0') {
		recvb[0] = '/';
		recvb[1] = '\0';
	}

	if (recvb[0] != '/' || strstr(recvb, "..")){
		dprintf(sock, selinval);
		return;
	}

	snprintf(path, sizeof(path), "%s%s", base, recvb);

	fd = -1;
	if (stat(path, &dir) != -1 && S_ISDIR(dir.st_mode)) {
		for (i = 0; i < sizeof(indexf)/sizeof(indexf[0]); i++) {
			if (strlen(path) + strlen(indexf[i]) >= sizeof(path)) {
				if (loglvl & ERRORS)
					logentry(clienth, clientp, recvc,
					         "path truncation occurred");
				return;
			}
			strncat(path, indexf[i], sizeof(path) - strlen(path) - 1);
			fd = open(path, O_RDONLY);
			if (fd >= 0)
				break;
			path[strlen(path)-strlen(indexf[i])] = '\0';
		}
	} else {
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			dprintf(sock, notfounderr, recvc);
			if (loglvl & ERRORS)
				logentry(clienth, clientp, recvc, strerror(errno));
			return;
		}
	}

	if (fd >= 0) {
		close(fd);
		if (loglvl & FILES)
			logentry(clienth, clientp, recvc, "serving");

		c = strrchr(path, '/');
		if (c == NULL)
			c = path;
		type = gettype(c);
		if (nocgi && (type->f == handledcgi || type->f == handlecgi)) {
			dprintf(sock, nocgierr, recvc);
			if (loglvl & ERRORS)
				logentry(clienth, clientp, recvc, "nocgi error");
		} else {
			type->f(sock, path, port, base, args, sear, ohost,
				clienth);
		}
	} else {
		if (S_ISDIR(dir.st_mode)) {
			handledir(sock, path, port, base, args, sear, ohost,
				clienth);
			if (loglvl & DIRS) {
				logentry(clienth, clientp, recvc,
							"dir listing");
			}
			return;
		}

		dprintf(sock, notfounderr, recvc);
		if (loglvl & ERRORS)
			logentry(clienth, clientp, recvc, "not found");
	}

	return;
}

void
sighandler(int sig)
{
	int i;

	switch (sig) {
	case SIGCHLD:
		while (waitpid(-1, NULL, WNOHANG) > 0);
		break;
	case SIGINT:
	case SIGQUIT:
	case SIGABRT:
	case SIGTERM:
	case SIGKILL:
		if (logfile != NULL && glfd != -1) {
			close(glfd);
			glfd = -1;
		}

		for (i = 0; i < nlistfds; i++) {
			shutdown(listfds[i], SHUT_RDWR);
			close(listfds[i]);
		}
		free(listfds);
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

/*
 * TODO: Move Linux and BSD to Plan 9 socket and bind handling, so we do not
 *       need the inconsistent return and exit on getaddrinfo.
 */
int *
getlistenfd(struct addrinfo *hints, char *bindip, char *port, int *rlfdnum)
{
	char addstr[INET6_ADDRSTRLEN];
	struct addrinfo *ai, *rp;
	void *sinaddr;
	int on, *listenfds, *listenfd, aierr, errno_save;

	if ((aierr = getaddrinfo(bindip, port, hints, &ai)) || ai == NULL) {
		fprintf(stderr, "getaddrinfo (%s:%s): %s\n", bindip, port,
			gai_strerror(aierr));
		exit(1);
	}

	*rlfdnum = 0;
	listenfds = NULL;
	on = 1;
	for (rp = ai; rp != NULL; rp = rp->ai_next) {
		listenfds = xrealloc(listenfds,
				sizeof(*listenfds) * (++*rlfdnum));
		listenfd = &listenfds[*rlfdnum-1];

		*listenfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (*listenfd < 0)
			continue;
		if (setsockopt(*listenfd, SOL_SOCKET, SO_REUSEADDR, &on,
				sizeof(on)) < 0) {
			close(*listenfd);
			(*rlfdnum)--;
			continue;
		}

		if (rp->ai_family == AF_INET6 && (setsockopt(*listenfd,
				IPPROTO_IPV6, IPV6_V6ONLY, &on,
				sizeof(on)) < 0)) {
			close(*listenfd);
			(*rlfdnum)--;
			continue;
		}

		sinaddr = (rp->ai_family == AF_INET) ?
		          (void *)&((struct sockaddr_in *)rp->ai_addr)->sin_addr :
		          (void *)&((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;

		if (bind(*listenfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			if (loglvl & CONN && inet_ntop(rp->ai_family, sinaddr,
					addstr, sizeof(addstr))) {
				/* Do not revlookup here. */
				on = revlookup;
				revlookup = 0;
				logentry(addstr, port, "-", "listening");
				revlookup = on;
			}
			continue;
		}

		/* Save errno, because fprintf in logentry overwrites it. */
		errno_save = errno;
		close(*listenfd);
		(*rlfdnum)--;
		if (loglvl & CONN && inet_ntop(rp->ai_family, sinaddr,
				addstr, sizeof(addstr))) {
			/* Do not revlookup here. */
			on = revlookup;
			revlookup = 0;
			logentry(addstr, port, "-", "could not bind");
			revlookup = on;
		}
		errno = errno_save;
	}
	freeaddrinfo(ai);
	if (*rlfdnum < 1) {
		free(listenfds);
		return NULL;
	}

	return listenfds;
}

void
usage(void)
{
	dprintf(2, "usage: %s [-46cden] [-l logfile] "
		   "[-t keyfile certfile] "
	           "[-v loglvl] [-b base] [-p port] [-o sport] "
	           "[-u user] [-g group] [-h host] [-i interface ...]\n",
		   argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct addrinfo hints;
	struct sockaddr_storage clt;
	socklen_t cltlen;
	int sock, dofork = 1, inetf = AF_UNSPEC, usechroot = 0,
	    nocgi = 0, errno_save, nbindips = 0, i, j,
	    nlfdret, *lfdret, listfd, maxlfd, dotls = 0, istls = 0,
	    shuflen, wlen, shufpos, tlspipe[2], maxrecv, retl,
	    rlen = 0;
	fd_set rfd;
	char *port, *base, clienth[NI_MAXHOST], clientp[NI_MAXSERV],
	     *user = NULL, *group = NULL, **bindips = NULL,
	     *ohost = NULL, *sport = NULL, *p, *certfile = NULL,
	     *keyfile = NULL, shufbuf[1025], byte0, recvb[1025];
	struct passwd *us = NULL;
	struct group *gr = NULL;
	struct tls_config *tlsconfig = NULL;
	struct tls *tlsctx = NULL, *tlsclientctx;

	base = stdbase;
	port = stdport;

	ARGBEGIN {
	case '4':
		inetf = AF_INET;
		break;
	case '6':
		inetf = AF_INET6;
		break;
	case 'b':
		base = EARGF(usage());
		break;
	case 'c':
		usechroot = 1;
		break;
	case 'p':
		port = EARGF(usage());
		if (sport == NULL)
			sport = port;
		break;
	case 'l':
		logfile = EARGF(usage());
		break;
	case 'd':
		dofork = 0;
		break;
	case 'e':
		nocgi = 1;
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
		bindips = xrealloc(bindips, sizeof(*bindips) * (++nbindips));
		bindips[nbindips-1] = EARGF(usage());
		break;
	case 'h':
		ohost = EARGF(usage());
		break;
	case 'o':
		sport = EARGF(usage());
		break;
	case 'n':
		revlookup = 0;
		break;
	case 't':
		dotls = 1;
		keyfile = EARGF(usage());
		certfile = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;

	if (sport == NULL)
		sport = port;

	if (argc != 0)
		usage();

	if (dotls) {
		if (tls_init() < 0) {
			perror("tls_init");
			return 1;
		}
		if ((tlsconfig = tls_config_new()) == NULL) {
			perror("tls_config_new");
			return 1;
		}
		if ((tlsctx = tls_server()) == NULL) {
			perror("tls_server");
			return 1;
		}
		if (tls_config_set_key_file(tlsconfig, keyfile) < 0) {
			perror("tls_config_set_key_file");
			return 1;
		}
		if (tls_config_set_cert_file(tlsconfig, certfile) < 0) {
			perror("tls_config_set_cert_file");
			return 1;
		}
		if (tls_configure(tlsctx, tlsconfig) < 0) {
			perror("tls_configure");
			return 1;
		}
	}

	if (ohost == NULL) {
		/* Do not use HOST_NAME_MAX, it is not defined on NetBSD. */
		ohost = xcalloc(1, 256+1);
		if (gethostname(ohost, 256) < 0) {
			perror("gethostname");
			free(ohost);
			return 1;
		}
	} else {
		ohost = xstrdup(ohost);
	}

	if (group != NULL) {
		errno = 0;
		if ((gr = getgrnam(group)) == NULL) {
			if (errno == 0) {
				fprintf(stderr, "no such group '%s'\n", group);
			} else {
				perror("getgrnam");
			}
			return 1;
		}
	}

	if (user != NULL) {
		errno = 0;
		if ((us = getpwnam(user)) == NULL) {
			if (errno == 0) {
				fprintf(stderr, "no such user '%s'\n", user);
			} else {
				perror("getpwnam");
			}
			return 1;
		}
	}

	if (dofork) {
		switch (fork()) {
		case -1:
			perror("fork");
			return 1;
		case 0:
			break;
		default:
			return 0;
		}
	}

	if (logfile != NULL) {
		glfd = open(logfile, O_APPEND | O_WRONLY | O_CREAT, 0644);
		if (glfd < 0) {
			perror("log");
			return 1;
		}
	} else if (!dofork) {
		glfd = 1;
	}

	if (bindips == NULL) {
		if (inetf == AF_INET || inetf == AF_UNSPEC) {
			bindips = xrealloc(bindips, sizeof(*bindips) * (++nbindips));
			bindips[nbindips-1] = "0.0.0.0";
		}
		if (inetf == AF_INET6 || inetf == AF_UNSPEC) {
			bindips = xrealloc(bindips, sizeof(*bindips) * (++nbindips));
			bindips[nbindips-1] = "::";
		}
	}

	for (i = 0; i < nbindips; i++) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = inetf;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		if (bindips[i])
			hints.ai_flags |= AI_CANONNAME;

		nlfdret = 0;
		lfdret = getlistenfd(&hints, bindips[i], port, &nlfdret);
		if (nlfdret < 1) {
			errno_save = errno;
			fprintf(stderr, "Unable to get a binding socket for "
					"%s:%s\n", bindips[i], port);
			errno = errno_save;
			perror("getlistenfd");
		}

		for (j = 0; j < nlfdret; j++) {
			if (listen(lfdret[j], 255) < 0) {
				perror("listen");
				close(lfdret[j]);
				continue;
			}
			listfds = xrealloc(listfds,
					sizeof(*listfds) * ++nlistfds);
			listfds[nlistfds-1] = lfdret[j];
		}
		free(lfdret);
	}
	free(bindips);

	if (nlistfds < 1)
		return 1;

	if (usechroot) {
		if (chdir(base) < 0) {
			perror("chdir");
			return 1;
		}
		base = "";
		if (chroot(".") < 0) {
			perror("chroot");
			return 1;
		}
	} else if (*base != '/' && !(base = realpath(base, NULL))) {
		perror("realpath");
		return 1;
	}

	/* strip / at the end, except if it is "/" */
	for (p = base + strlen(base); p > base + 1 && p[-1] == '/'; --p)
		p[-1] = '\0';

	if (dropprivileges(gr, us) < 0) {
		perror("dropprivileges");

		for (i = 0; i < nlistfds; i++) {
			shutdown(listfds[i], SHUT_RDWR);
			close(listfds[i]);
		}
		free(listfds);
		return 1;
	}

	initsignals();

#ifdef __OpenBSD__
	char promises[31]; /* check the size needed in the fork too */
	snprintf(promises, sizeof(promises), "rpath inet stdio proc exec %s",
	         revlookup ? "dns" : "");
	if (pledge(promises, NULL) == -1) {
		perror("pledge");
		exit(1);
	}
#endif /* __OpenBSD__ */

	while (1) {
		FD_ZERO(&rfd);
		maxlfd = 0;
		for (i = 0; i < nlistfds; i++) {
			FD_SET(listfds[i], &rfd);
			if (listfds[i] > maxlfd)
				maxlfd = listfds[i];
		}

		if (pselect(maxlfd+1, &rfd, NULL, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			perror("pselect");
			break;
		}

		listfd = -1;
		for (i = 0; i < nlistfds; i++) {
			if (FD_ISSET(listfds[i], &rfd)) {
				listfd = listfds[i];
				break;
			}
		}
		if (listfd < 0)
			continue;

		cltlen = sizeof(clt);
		sock = accept(listfd, (struct sockaddr *)&clt, &cltlen);
		if (sock < 0) {
			switch (errno) {
			case ECONNABORTED:
			case EINTR:
				continue;
			default:
				perror("accept");
				close(listfd);
				return 1;
			}
		}

		if (getnameinfo((struct sockaddr *)&clt, cltlen, clienth,
				sizeof(clienth), clientp, sizeof(clientp),
				NI_NUMERICHOST|NI_NUMERICSERV)) {
			clienth[0] = clientp[0] = '\0';
		}

		if (!strncmp(clienth, "::ffff:", 7))
			memmove(clienth, clienth+7, strlen(clienth)-6);

		if (loglvl & CONN)
			logentry(clienth, clientp, "-", "connected");

		switch (fork()) {
		case -1:
			perror("fork");
			shutdown(sock, SHUT_RDWR);
			break;
		case 0:
			close(listfd);

			signal(SIGHUP, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			signal(SIGALRM, SIG_DFL);

#ifdef __OpenBSD__
			snprintf(promises, sizeof(promises),
			         "rpath inet stdio %s %s",
			         nocgi     ? ""    : "proc exec",
			         revlookup ? "dns" : "");
			if (pledge(promises, NULL) == -1) {
				perror("pledge");
				exit(1);
			}
#endif /* __OpenBSD__ */

			if (recv(sock, &byte0, 1, MSG_PEEK) < 1)
				return 1;

			/*
			 * First byte is 0x16 == 22, which is the TLS
			 * Handshake first byte.
			 */
			istls = 0;
			if (byte0 == 0x16 && dotls) {
				istls = 1;
				if (tls_accept_socket(tlsctx, &tlsclientctx, sock) < 0)
					return 1;
				if (tls_handshake(tlsclientctx) < 0)
					return 1;
			}

			maxrecv = sizeof(recvb) - 1;
			do {
				if (istls) {
					retl = tls_read(tlsclientctx,
						recvb+rlen, sizeof(recvb)-1-rlen);
					if (retl < 0)
						fprintf(stderr, "tls_read failed: %s\n", tls_error(tlsclientctx));
				} else {
					retl = read(sock, recvb+rlen,
						sizeof(recvb)-1-rlen);
					if (retl < 0)
						perror("read");
				}
				if (retl <= 0)
					break;
				rlen += retl;
			} while (recvb[rlen-1] != '\n'
					&& --maxrecv > 0);
			if (rlen <= 0)
				return 1;

			if (istls) {
				if (pipe(tlspipe) < 0) {
					perror("tls_pipe");
					return 1;
				}

				switch(fork()) {
				case 0:
					sock = tlspipe[1];
					close(tlspipe[0]);
					break;
				case -1:
					perror("fork");
					return 1;
				default:
					close(tlspipe[1]);
					do {
						shuflen = read(tlspipe[0], shufbuf, sizeof(shufbuf)-1);
						if (shuflen == -1 && errno == EINTR)
							continue;
						for (shufpos = 0; shufpos < shuflen; shufpos += wlen) {
							wlen = tls_write(tlsclientctx, shufbuf+shufpos, shuflen-shufpos);
							if (wlen < 0) {
								fprintf(stderr, "tls_write failed: %s\n", tls_error(tlsclientctx));
								return 1;
							}
						}
					} while(shuflen > 0);

					tls_close(tlsclientctx);
					tls_free(tlsclientctx);
					close(tlspipe[0]);

					waitforpendingbytes(sock);
					shutdown(sock, SHUT_RDWR);
					close(sock);
					return 0;
				}
			}

			handlerequest(sock, recvb, rlen, base,
					ohost, sport, clienth,
					clientp, nocgi);

			if (!istls) {
				waitforpendingbytes(sock);
				shutdown(sock, SHUT_RDWR);
				close(sock);
			}
			close(sock);

			if (loglvl & CONN) {
				logentry(clienth, clientp, "-",
						"disconnected");
			}

			return 0;
		default:
			break;
		}
		close(sock);
	}

	if (logfile != NULL && glfd != -1) {
		close(glfd);
		glfd = -1;
	}
	free(ohost);

	for (i = 0; i < nlistfds; i++) {
		shutdown(listfds[i], SHUT_RDWR);
		close(listfds[i]);
	}
	free(listfds);

	if (dotls) {
		tls_close(tlsctx);
		tls_free(tlsctx);
		tls_config_free(tlsconfig);
	}

	return 0;
}

