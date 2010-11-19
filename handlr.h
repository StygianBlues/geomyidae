/*
 * Copy me if you can. 
 * by 20h
 */

#ifndef HANDLR_H
#define HANDLR_H

#define nil NULL

void handledir(int sock, char *path, char *port, char *base, char *args,
			char *sear);
void handlegph(int sock, char *file, char *port, char *base, char *args,
			char *sear);
void handlebin(int sock, char *file, char *port, char *base, char *args,
			char *sear);
void handlecgi(int sock, char *file, char *port, char *base, char *args,
			char *sear);

#endif
