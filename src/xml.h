#ifndef XML_H
#define XML_H

List *readAlbum( char *file, ConfArg *cfg );
void writeThumbData( ConfArg *cfg, List *thumbdata, char *file );
void writeAlbum( ConfArg *cfg, List *thumbdata, char *path );
#endif
