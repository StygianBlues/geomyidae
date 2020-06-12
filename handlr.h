/*
 * Copy me if you can.
 * by 20h
 */

#ifndef HANDLR_H
#define HANDLR_H

/*
 * Handler API definition
 *
 * path .... absolute path to the script
 * port .... port which the script should use when defining menu items
 *	     (See -o and -p in geomyidae(8))
 * base .... base path of geomyidae ("" in case of chroot)
 * args .... query string parsed after »script?query«
 * sear .... search part of request (»selector\tsearch\r\n«)
 * ohost ... host of geomiydae (See -h in geomyidae(8))
 * chost ... IP of the client sending a request
 * istls ... set to 1, if TLS was used for thr request
 */

void handledir(int sock, char *path, char *port, char *base, char *args,
			char *sear, char *ohost, char *chost, int istls);
void handlegph(int sock, char *file, char *port, char *base, char *args,
			char *sear, char *ohost, char *chost, int istls);
void handlebin(int sock, char *file, char *port, char *base, char *args,
			char *sear, char *ohost, char *chost, int istls);
void handlecgi(int sock, char *file, char *port, char *base, char *args,
			char *sear, char *ohost, char *chost, int istls);
void handledcgi(int sock, char *file, char *port, char *base, char *args,
			char *sear, char *ohost, char *chost, int istls);

#endif
