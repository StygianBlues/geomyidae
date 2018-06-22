#ifndef ARG_H
#define ARG_H

#define USED(x) ((void)(x))

extern char *argv0;

#define ARGBEGIN	for(argv0 = *argv, argv++, argc--;\
					argv[0] && argv[0][0] == '-'\
					&& argv[0][1];\
					argc--, argv++) {\
				char _argc;\
				char **_argv;\
				if(argv[0][1] == '-' && argv[0][2] == '\0') {\
					argv++;\
					argc--;\
					break;\
				}\
				int i_;\
				for(i_ = 1, _argv = argv; argv[0][i_];\
						i_++) {\
					if(_argv != argv)\
						break;\
					_argc = argv[0][i_];\
					switch(_argc)

#define ARGEND			}\
				USED(_argc);\
			}\
			USED(argv);\
			USED(argc);

#define	EARGF(x)	((argv[1] == NULL)? ((x), abort(), (char *)0) :\
			(argc--, argv++, argv[0]))

#endif

