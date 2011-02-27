#ifndef ARG_H
#define ARG_H

#define USED(x) ((void)(x))

extern char *argv0;

#define	ARGBEGIN	for(argv0 = *argv, argv++, argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char _argc;\
				_argc = argv[0][1];\
				switch(_argc)
#define	ARGEND		USED(_argc);} USED(argv);USED(argc);
#define	EARGF(x)	((argv[1] == nil)? ((x), abort(), (char *)0) :\
			(argc--, argv++, argv[0]))

#endif

