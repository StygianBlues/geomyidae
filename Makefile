PROGRAM = geomyidae
VERSION = 0.18

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/man/man8

#CPPFLAGS += -D_BSD_SOURCE
CFLAGS += -O2 -Wall -I. -I/usr/include 
LDFLAGS += -L/usr/lib -L. -lc

CFILES = main.c ind.c handlr.c 

OBJECTS = ${CFILES:.c=.o}

all: $(PROGRAM)

${PROGRAM}: ${OBJECTS}
	${CC} ${LDFLAGS} -o ${PROGRAM} ${OBJECTS}

.SUFFIXES : .c .h

.c.o :
	${CC} ${CFLAGS} ${CPPFLAGS} -c $<
.c :
	${CC} ${CFLAGS} ${CPPFLAGS} -c $<

clean :
	@rm -f *.o ${PROGRAM} core *~

install: $(PROGRAM)
	@mkdir -p ${DESTDIR}${BINDIR}
	@cp -f ${PROGRAM} ${DESTDIR}${BINDIR}
	@strip ${DESTDIR}${BINDIR}/${PROGRAM}
	@chmod 755 ${DESTDIR}${BINDIR}/${PROGRAM}
	@mkdir -p ${DESTDIR}${MANDIR}
	@cp -f geomyidae.8 ${DESTDIR}${MANDIR}
	@chmod 644 ${DESTDIR}${MANDIR}/${PROGRAM}.8

uninstall:
	@rm -f ${DESTDIR}${BINDIR}/${PROGRAM}
	@rm -f ${DESTDIR}${MANDIR}/${PROGRAM}.8

dist: clean
	@mkdir -p "${PROGRAM}-${VERSION}"
	@cp -r rc.d README LICENSE index.gph Makefile geomyidae.8 \
	       	*.c *.h "${PROGRAM}-${VERSION}"
	@chmod 755 "${PROGRAM}-${VERSION}"
	@chmod 744 "${PROGRAM}-${VERSION}"/*
	@tar -cf "${PROGRAM}-${VERSION}.tar" "${PROGRAM}-${VERSION}"
	@gzip "${PROGRAM}-${VERSION}.tar"
	@mv "${PROGRAM}-${VERSION}.tar.gz" "${PROGRAM}-${VERSION}.tgz"
	@rm -rf "${PROGRAM}-${VERSION}"

.PHONY: all clean dist install uninstall

