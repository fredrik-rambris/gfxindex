#ifndef XML_H
#define XML_H
#if HAVE_LIBEXPAT

List *readAlbum( char *file, ConfArg *cfg );

#endif
void writeThumbData( ConfArg *cfg, List *thumbdata, char *file );
void writeAlbum( ConfArg *cfg, List *thumbdata, char *path );
#endif
