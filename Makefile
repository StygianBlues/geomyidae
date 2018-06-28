# geomyidae - a tiny, standalone gopherd written in C
# See LICENSE file for copyright and license details.
.POSIX:

NAME = geomyidae
VERSION = 0.32.1

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin
MANDIR = ${PREFIX}/share/man/man8

CFLAGS = -O2 -Wall
GEOM_CFLAGS = -D_DEFAULT_SOURCE -I. -I/usr/include ${CFLAGS}
GEOM_LDFLAGS = -L/usr/lib -L. ${LDFLAGS}

SRC = main.c ind.c handlr.c
OBJ = ${SRC:.c=.o}

all: options ${NAME}

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${GEOM_CFLAGS}"
	@echo "LDFLAGS  = ${GEOM_LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} ${GEOM_CFLAGS} -c $<

${OBJ}:

${NAME}: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${GEOM_LDFLAGS}

clean:
	@echo cleaning
	@rm -f ${NAME} ${OBJ} ${NAME}-${VERSION}.tar.gz

install: all
	@echo installing executable to ${DESTDIR}${BINDIR}
	@mkdir -p "${DESTDIR}${BINDIR}"
	@cp -f ${NAME} "${DESTDIR}${BINDIR}"
	@chmod 755 "${DESTDIR}${BINDIR}/${NAME}"
	@echo installing manpage to "${DESTDIR}${MANDIR}"
	@mkdir -p "${DESTDIR}${MANDIR}"
	@cp -f ${NAME}.8 "${DESTDIR}${MANDIR}"
	@chmod 644 "${DESTDIR}${MANDIR}/${NAME}.8"

uninstall:
	@echo removing executable file from ${DESTDIR}${BINDIR}
	@rm -f "${DESTDIR}${BINDIR}/${NAME}"
	@echo removing manpage from "${DESTDIR}${MANDIR}"
	@rm -f "${DESTDIR}${MANDIR}/${NAME}.8"

dist: clean
	@echo creating dist tarball
	@mkdir -p ${NAME}-${VERSION}
	@cp -R rc.d CGI README LICENSE index.gph Makefile ${NAME}.8 \
	       	*.c *.h ${NAME}-${VERSION}
	@tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	@gzip ${NAME}-${VERSION}.tar
	@mv ${NAME}-${VERSION}.tar.gz ${NAME}-${VERSION}.tgz
	@rm -rf "${NAME}-${VERSION}"

.PHONY: all options clean dist install uninstall
