# geomyidae - a tiny, standalone gopherd written in C
# See LICENSE file for copyright and license details.
.POSIX:

NAME = geomyidae
VERSION = 0.34

PREFIX = /usr/local
BINDIR = ${PREFIX}/sbin
MANDIR = ${PREFIX}/share/man/man8

# Comment to disable TLS support
TLS_CFLAGS = -DENABLE_TLS
TLS_LDFLAGS = -ltls 

GEOM_CFLAGS = -D_DEFAULT_SOURCE -I. -I/usr/include ${TLS_CFLAGS} ${CFLAGS}
GEOM_LDFLAGS = -L/usr/lib -L. ${TLS_LDFLAGS} ${LDFLAGS}

SRC = main.c ind.c handlr.c
OBJ = ${SRC:.c=.o}

all: ${NAME}

.c.o:
	${CC} ${GEOM_CFLAGS} -c $<

${OBJ}:

${NAME}: ${OBJ}
	${CC} -o $@ ${OBJ} ${GEOM_LDFLAGS}

clean:
	rm -f ${NAME} ${OBJ} ${NAME}-${VERSION}.tar.gz

install: all
	mkdir -p "${DESTDIR}${BINDIR}"
	cp -f ${NAME} "${DESTDIR}${BINDIR}"
	chmod 755 "${DESTDIR}${BINDIR}/${NAME}"
	mkdir -p "${DESTDIR}${MANDIR}"
	cp -f ${NAME}.8 "${DESTDIR}${MANDIR}"
	chmod 644 "${DESTDIR}${MANDIR}/${NAME}.8"

uninstall:
	rm -f "${DESTDIR}${BINDIR}/${NAME}"
	rm -f "${DESTDIR}${MANDIR}/${NAME}.8"

dist: clean
	mkdir -p ${NAME}-${VERSION}
	cp -R rc.d CGI README LICENSE index.gph Makefile ${NAME}.8 \
	       	*.c *.h ${NAME}-${VERSION}
	tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	gzip ${NAME}-${VERSION}.tar
	mv ${NAME}-${VERSION}.tar.gz ${NAME}-${VERSION}.tgz
	rm -rf "${NAME}-${VERSION}"

.PHONY: all clean dist install uninstall
