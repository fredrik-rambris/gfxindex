#ifndef WIN32_H
#define WIN32_H

#ifdef WIN32

char *basename( char *str );
int myfprintf(FILE *stream, const char *format, ...);

#else

#define myfprintf fprintf

#endif

#endif
