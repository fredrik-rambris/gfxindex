#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <glib.h>

#include "gfx.h"
#include "defaults.h"

struct ThumbData
{
	gchar *image;
	guint imagewidth, imageheight;
	gchar *thumb;
	guint thumbwidth, thumbheight;
	gchar *extra;
};

struct Global
{
	guint ThumbWidth, ThumbHeight;
	gchar *bgcolor, *bevelbright, *beveldark, *bevelback;
	gboolean bevel, recursive, overwrite, pad, quiet;
	gchar *dir;
	gchar *thumbdir;
	gboolean genindex, titles, numlink, showcredits, makethumbs, hideext;
	gchar *title;
	gint thumbscale;
	gint xstop, ystop;
	gchar *bodyargs;
	gchar *cellargs;
	gchar *css;
	gchar *left, *space, *divider, *right, *previous, *next, *index, *parent, *parentdoc;
	gint quality;
};

extern struct Global *global;

struct ProcessInfo
{
	struct color col[3];
	gboolean up;
};

void cleanup( void );
void makethumbs( gchar *dir, struct ProcessInfo *processinfo, gint level, struct Global *cfg );
void gfxindex( struct Global *local, gchar *dir, GList *thumbs, gint level );
gchar *indexstr( int number );
gchar *setstr( gchar *old, gchar *new );
void error( gchar *msg );
void navbar_new( GString *str );
void navbar_add( struct Global *local, GString *str, gchar *newstr, ... );
void navbar_end( struct Global *local, GString *str );
gboolean checkext( gchar *file );
gint dircomp( const struct dirent **a, const struct dirent **b );
gint thumbcomp( gpointer a, gpointer b );
void savethumblist( GList *thumbs, gchar *file );
GList *loadthumblist( GList *thumbs, gchar *file );
void freenode( gpointer data, gpointer user_data );
gboolean fastcompare( gchar *a, gchar *b );
GList *removenode( GList *thumbs, gchar *imagename );
struct ThumbData *findnode( GList *thumbs, gchar *imagename );
gboolean file_exist( gchar *filename );

#endif /* GLOBAL_H */
