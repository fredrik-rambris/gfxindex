/*
 * GFXIndex (c) 1999-2000 Fredrik Rambris <fredrik@rambris.com>. All rights reserved.
 *
 * GFXIndex is a tool that creates thumbnails and HTML-indexes of your images. 
 *
 * This is licensed under GNU GPL.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define VERSION "1.1"
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk_imlib.h>
#include <popt.h>

#include "config.h"

struct ThumbData
{
	gchar *image;
	guint imagewidth, imageheight;
	gchar *thumb;
	guint thumbwidth, thumbheight;
	gchar *extra;
};

/* Our global variable space */
struct Global
{
	guint ThumbWidth, ThumbHeight;
	gchar *bgcolor, *bevelbright, *beveldark, *bevelback;
	gboolean bevel, recursive, overwrite, pad, quiet;
	gchar *dir;
	gchar *thumbdir;
	gboolean genindex, titles, numlink, showcredits, makethumbs;
	gchar *title;
	gint thumbscale;
	gint xstop, ystop;
	gchar *bodyargs;
	gchar *cellargs;
	gchar *css;
	gchar *left, *space, *divider, *right, *previous, *next, *index, *parent, *parentdoc;
	gint quality;
};
struct Global *global=NULL;

struct ProcessInfo
{
	GdkPixmap *thumb;
	GdkImlibImage *im, *thumbim;
	GdkGC *gc;
	GdkColor col[3];
	gboolean up;
};

/* Prototypes */

char *stripws( char *str );
void readconf( gchar *filename, struct Global *cfg );
gint handleargs( gint argc, gchar **argv, struct Global *cfg );
void free_empties( struct Global *cfg );
void cleanup( struct Global *cfg );
struct Global *dupconf( struct Global *cfg );
void makethumbs( gchar *dir, struct ProcessInfo *processinfo, gint level, struct Global *cfg );
void gfxindex( struct Global *local, gchar *dir, GList *thumbs, gint level );
gchar *indexstr( int number );
gchar *setstr( gchar *old, gchar *new );
void error( gchar *msg );
void navbar_new( GString *str );
void navbar_add( struct Global *local, GString *str, gchar *newstr, ... );
void navbar_end( struct Global *local, GString *str );
void setglobaldefaults( void );
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

gint main( gint argc, gchar **argv )
{
	gdk_init( &argc, &argv );
	gdk_imlib_init();
	if( !( global=g_new0( struct Global, 1 ) ) ) error( "Couldn't allocate memory" );
	setglobaldefaults();
	readconf( "/etc/gfxindex", global );
	readconf( "~/.gfxindex", global );
	if( !handleargs( argc, argv, global ) ) error( NULL );
	free_empties( global );
	if( !global->quiet ) printf( "GFXIndex v" VERSION " (c) Fredrik Rambris 1999-2000.\n" );

	makethumbs( global->dir, NULL, 0, global );
	cleanup(global);
	return( 0 );
}

gint handleargs( gint argc, gchar **argv, struct Global *cfg )
{
	gchar c;
	GString *tempstr=g_string_new( NULL );
	gint ret=TRUE;
	poptContext optCon;

	struct poptOption optionsTable[] =
	{
		{ "quiet", 'q', POPT_ARG_NONE, NULL, 'q', "don't print any progress information", NULL },
		{ "dir", 'd', POPT_ARG_STRING, NULL, 'd', "dir to start with", "DIR" },
		{ "overwrite", 'o', POPT_ARG_NONE, NULL, 'o', "overwrite thumbnails", NULL },
		{ "recursive", 'r', POPT_ARG_NONE, NULL, 'r', "traverse dirs", NULL },
#if DEFAULT_GENINDEX
		{ "nothumbs", 'T', POPT_ARG_NONE, NULL, 'T', "don't make thumbnails", NULL },
#else
		{ "thumbs", 'T', POPT_ARG_NONE, NULL, 'T', "make thumbnails", NULL },
#endif
#if DEFAULT_PAD
		{ "nopad", 'p', POPT_ARG_NONE, NULL, 'p', "don't pad thumbnails for equal size", NULL },
#else
		{ "pad", 'p', POPT_ARG_NONE, NULL, 'p', "pad thumbnails for equal size", NULL },
#endif
		{ "thumbwidth", 'w', POPT_ARG_INT, &cfg->ThumbWidth, 0, "width of thumbnails", "PIXELS" },
		{ "thumbheight", 'h', POPT_ARG_INT, &cfg->ThumbHeight, 0, "height of thumbnails", "PIXELS" },
		{ "quality", 'Q', POPT_ARG_INT, NULL, 'Q', "JPEG quality of thumbs. 0 crap, 100 good", "NUM" },
		{ "thumbbgcolor", 'c', POPT_ARG_STRING, NULL, 'c', "backgroundcolor of thumbnails", "COLORSPEC" },
		{ "thumbbevel", 'b', POPT_ARG_NONE, NULL, 'b', "beveled thumbnails", NULL },
		{ "bevelbg", 'g', POPT_ARG_STRING, NULL, 'g', "backgroundcolor in beveled state", "COLORSPEC" },
		{ "bevelbright", 'l', POPT_ARG_STRING, NULL, 'l', "color of bright edges in bevel", "COLORSPEC" },
		{ "beveldark", 'm', POPT_ARG_STRING, NULL, 'm', "color of dark edges in bevel\n", "COLORSPEC" },
#if DEFAULT_GENINDEX
		{ "noindexes", 'i', POPT_ARG_NONE, NULL, 'i', "don't create HTML indexes", NULL },
#else
		{ "indexes", 'i', POPT_ARG_NONE, NULL, 'i', "create HTML indexes", NULL },
#endif
#if DEFAULT_SHOWCREDITS
		{ "nocredits", 'C', POPT_ARG_NONE, NULL, 'C', "don't show credits", NULL },
#else
		{ "credits", 'C', POPT_ARG_NONE, NULL, 'C', "show credits", NULL },
#endif
#if DEFAULT_TITLES
		{ "notitles", 't', POPT_ARG_NONE, NULL, 't', "don't show filenames", NULL },
#else
		{ "titles", 't', POPT_ARG_NONE, NULL, 't', "show filenames", NULL },
#endif
#if DEFAULT_NUMLINK
		{ "nonumlink", 'n', POPT_ARG_NONE, NULL, 'n', "don't use numeric links to individial pages", NULL },
#else
		{ "numlink", 'n', POPT_ARG_NONE, NULL, 'n', "use numeric links to individial pages", NULL },
#endif
		{ "navthumbs", 'u', POPT_ARG_INT, &cfg->thumbscale, 0, "use thumbnails for Prev and Next (set scale)", "PERCENT" },
		{ "numx", 'x', POPT_ARG_INT, &cfg->xstop, 0, "number of thumbnails per row", "NUM" },
		{ "numy", 'y', POPT_ARG_INT, &cfg->ystop, 0, "number of rows per page", "NUM" },
		{ "bodyargs", 'a', POPT_ARG_STRING, NULL, 'a', "statements inserted in the BODY tag", "STRING" },
		{ "cellargs", 'e', POPT_ARG_STRING, NULL, 'e', "statements inserted in the TD tag", "STRING" },
		{ "css", 's', POPT_ARG_STRING, NULL, 's', "relative path to Cascading StyleSheets\n", "PATH" },
		{ "parentdoc", 0, POPT_ARG_STRING, NULL, '.', "document linked to on Parent links", "PATH" },
		{ "left", 0, POPT_ARG_STRING, NULL, '[', NULL, "STRING" },
		{ "space", 0, POPT_ARG_STRING, NULL, ' ', NULL, "STRING" },
		{ "divider", 0, POPT_ARG_STRING, NULL, '|', NULL, "STRING" },
		{ "right", 0, POPT_ARG_STRING, NULL, ']', NULL, "STRING" },
		{ "prev", 0, POPT_ARG_STRING, NULL, '<', NULL, "STRING" },
		{ "next", 0, POPT_ARG_STRING, NULL, '>', NULL, "STRING" },
		{ "index", 0, POPT_ARG_STRING, NULL, '^', NULL, "STRING" },
		{ "parent", 0, POPT_ARG_STRING, NULL, '/', "\n", "STRING" },
		{ "defaults", 0, POPT_ARG_NONE, NULL, 'z', "show the defaults and quit", NULL },
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};

	optCon = poptGetContext( NULL, argc, argv, optionsTable, 0 );
	poptSetOtherOptionHelp( optCon, "<title>" );

	/* Now do options processing, get portname */
	while( ( c=poptGetNextOpt( optCon ) )>=0 )
	{
		switch( c )
		{
			case 'q':
				cfg->quiet=TRUE;
				break;
			case 'Q':
				cfg->quality=atoi( (char *)poptGetOptArg( optCon ) );
				if( cfg->quality > 100 ) cfg->quality=100;
				if( cfg->quality < 0 ) cfg->quality=0;
				cfg->quality=(guint)((gfloat)cfg->quality*2.559);
				break;
			case 'd':
				if( strlen( poptGetOptArg( optCon ) ) )
				{
					g_string_assign( tempstr, (gchar *)poptGetOptArg( optCon ) );
					if( tempstr->str[strlen(tempstr->str)-1]!='/' ) g_string_append( tempstr, "/" );
					cfg->dir=setstr( cfg->dir, tempstr->str );
				} else setstr( cfg->dir, NULL );
				break;
			case 'T':
#if DEFAULT_MAKETHUMBS
				cfg->makethumbs=FALSE;
#else
				cfg->makethumbs=TRUE;
#endif
				break;
			case 'c':
				cfg->bgcolor=setstr( cfg->bgcolor, strlen(poptGetOptArg( optCon ))?poptGetOptArg( optCon ):NULL );
				break;
			case 'b':
#if DEFAULT_BEVEL
				cfg->bevel=FALSE;
#else
				cfg->bevel=TRUE;
				cfg->pad=TRUE;
#endif
				break;
			case 'o':
				cfg->overwrite=TRUE;
				break;
			case 'r':
				cfg->recursive=TRUE;
				break;
			case 'p':
#if DEFAULT_PAD
				cfg->pad=FALSE;
				cfg->bevel=FALSE;
#else
				cfg->pad=TRUE;
#endif
				break;
			case 'g':
				cfg->bevelback=setstr( cfg->bevelback, (gchar *)poptGetOptArg( optCon ) );
				break;
			case 'l':
				cfg->bevelbright=setstr( cfg->bevelbright, (gchar *)poptGetOptArg( optCon ) );
				break;
			case 'm':
				cfg->beveldark=setstr( cfg->beveldark, (gchar *)poptGetOptArg( optCon ) );
				break;
			case 'i':
#if DEFAULT_GENINDEX
				cfg->genindex=FALSE;
#else
				cfg->genindex=TRUE;
#endif
				break;
			case 'C':
#if DEFAULT_SHOWCREDITS
				cfg->showcredits=FALSE;
#else
				cfg->showcredits=TRUE;
#endif
				break;
			case 't':
#if DEFAULT_TITLES
				cfg->titles=FALSE;
#else
				cfg->titles=TRUE;
#endif
				break;
			case 'n':
#if DEFAULT_NUMLINK
				cfg->numlink=FALSE;
#else
				cfg->numlink=TRUE;
#endif
				break;
			case 'a':
				cfg->bodyargs=setstr( cfg->bodyargs, (gchar *)poptGetOptArg( optCon ) );
				break;
			case 'e':
				cfg->cellargs=setstr( cfg->cellargs, (gchar *)poptGetOptArg( optCon ) );
				break;
			case 's':
				cfg->css=setstr( cfg->css, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '.':
				cfg->parentdoc=setstr( cfg->parentdoc, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '[':
				cfg->left=setstr( cfg->left, (gchar *)poptGetOptArg( optCon ) );
				break;
			case ' ':
				cfg->space=setstr( cfg->space, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '|':
				cfg->divider=setstr( cfg->divider, (gchar *)poptGetOptArg( optCon ) );
				break;
			case ']':
				cfg->right=setstr( cfg->right, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '<':
				cfg->previous=setstr( cfg->previous, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '>':
				cfg->next=setstr( cfg->next, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '^':
				cfg->index=setstr( cfg->index, (gchar *)poptGetOptArg( optCon ) );
				break;
			case '/':
				cfg->parent=setstr( cfg->parent, (gchar *)poptGetOptArg( optCon )  );
				break;

			case 'z':
				poptFreeContext(optCon);
				g_string_free( tempstr, TRUE );
				printf( "dir              %s\n", DEFAULT_DIR );
				printf( "Make thumbnails? %s\n", DEFAULT_MAKETHUMBS?"Yes":"No" );
				printf( "Show credits     %s\n", DEFAULT_SHOWCREDITS?"Yes":"No" );
				printf( "pad              %s\n", DEFAULT_PAD?"Yes":"No" );
				printf( "thumbwidth       %ld\n", DEFAULT_THUMBWIDTH );
				printf( "thumbheight      %ld\n", DEFAULT_THUMBHEIGHT );
				printf( "quality          %ld\n", DEFAULT_QUALITY );
				printf( "thumbbgcolor     %s\n", DEFAULT_BGCOLOR );
				printf( "thumbbevel       %s\n", DEFAULT_BEVEL?"Yes":"No" );
				printf( "bevelbg          %s\n", DEFAULT_BEVELBACK );
				printf( "bevelbright      %s\n", DEFAULT_BEVELBRIGHT );
				printf( "beveldark        %s\n", DEFAULT_BEVELDARK );
				printf( "Create indexes?  %s\n", DEFAULT_GENINDEX?"Yes":"No" );
				printf( "Show filenames?  %s\n", DEFAULT_TITLES?"Yes":"No" );
				printf( "Use NavThumbs?   %s (%ld %%)\n", DEFAULT_THUMBSCALE?"Yes":"No", DEFAULT_THUMBSCALE );
				printf( "numx             %ld\n", DEFAULT_XSTOP );
				printf( "numy             %ld\n", DEFAULT_YSTOP );
				printf( "bodyargs         %s\n", DEFAULT_BODYARGS );
				printf( "cellargs         %s\n", DEFAULT_CELLARGS );
				printf( "css              %s\n", DEFAULT_CSS );
				return( FALSE );
				break;
		}
	}
      	cfg->title=setstr( cfg->title, poptGetArg( optCon ) );

	if (c < -1)
	{
		/* an error occurred during option processing */
		fprintf( stderr, "%s: %s\n",
		poptBadOption( optCon, POPT_BADOPTION_NOALIAS ),
		poptStrerror(c) );
	}

	poptFreeContext(optCon);
	g_string_free( tempstr, TRUE );
	if( cfg->xstop<1 ) cfg->xstop=DEFAULT_XSTOP;
	if( cfg->ystop<1 ) cfg->ystop=DEFAULT_YSTOP;
	return ret;
}

char *stripws( char *str )
{
	int start, end;
	if( !str ) return( NULL );
	for( start=0 ; str[start]==' ' || str[start]=='\n' || str[start]=='\r' ; start++ );
	if( str[start]=='\0' )
	{
		str[0]='\0';
		return( str );
	}
	for( end=strlen( str ) ; ((str[end]==' ') || (str[end]=='\0') || (str[end]=='\n') || (str[end]=='\r')) && end>-1 ; end-- ) str[end]='\0';
	if( end<1 ) return( str );
	memmove( str, str+start, end-start+1 );
	str[ end-start+1 ]='\0';
	return( str );
}

void readconf( gchar *filename, struct Global *cfg  )
{
	gchar buf[1024], *s, *a;
	FILE *fp=NULL;
	gint count;
	gchar *answers="01NYFTnyft";
	enum
	{
		CA_ARG_INT=1,
		CA_ARG_STR,
		CA_ARG_BOOL
	};
	struct ConfArgs
	{
		gchar *name;
		gint argtype;
		void *arg;
	};

	struct ConfArgs confargs[]=
	{
		{ "quiet", CA_ARG_BOOL, &cfg->quiet },
		{ "dir" , CA_ARG_STR, &cfg->dir },
		{ "makethumbs", CA_ARG_BOOL, &cfg->makethumbs },
		{ "overwrite", CA_ARG_BOOL, &cfg->overwrite },
		{ "recursive", CA_ARG_BOOL, &cfg->recursive },
		{ "pad", CA_ARG_BOOL, &cfg->pad },
		{ "thumbwidth", CA_ARG_INT, &cfg->ThumbWidth },
		{ "thumbheight", CA_ARG_INT, &cfg->ThumbHeight },
		{ "quality", CA_ARG_INT, &cfg->quality },
		{ "thumbbgcolor", CA_ARG_STR, &cfg->bgcolor },
		{ "thumbbevel", CA_ARG_BOOL, &cfg->bevel },
		{ "bevelbg", CA_ARG_STR, &cfg->bevelback },
		{ "bevelbright", CA_ARG_STR, &cfg->bevelbright },
		{ "beveldark", CA_ARG_STR, &cfg->beveldark },
		{ "indexes", CA_ARG_BOOL, &cfg->genindex },
		{ "credits", CA_ARG_BOOL, &cfg->showcredits },
		{ "titles", CA_ARG_BOOL, &cfg->titles },
		{ "numlink", CA_ARG_BOOL, &cfg->numlink },
		{ "navthumbs", CA_ARG_INT, &cfg->thumbscale },
		{ "numx", CA_ARG_INT, &cfg->xstop },
		{ "numy", CA_ARG_INT, &cfg->ystop },
		{ "bodyargs", CA_ARG_STR, &cfg->bodyargs },
		{ "cellargs", CA_ARG_STR, &cfg->cellargs },
		{ "css", CA_ARG_STR, &cfg->css },
		{ "parentdoc", CA_ARG_STR, &cfg->parentdoc },
		{ "left", CA_ARG_STR, &cfg->left },
		{ "space", CA_ARG_STR, &cfg->space },
		{ "divider", CA_ARG_STR, &cfg->divider },
		{ "right", CA_ARG_STR, &cfg->right },
		{ "prev", CA_ARG_STR, &cfg->previous },
		{ "next", CA_ARG_STR, &cfg->next },
		{ "index", CA_ARG_STR, &cfg->index },
		{ "parent", CA_ARG_STR, &cfg->parent },
		{ "title", CA_ARG_STR, &cfg->title },
		{ NULL, 0, NULL }
	};

	if( fp=fopen( filename, "r" ) )
	{
		while( fgets( buf, 1023, fp ) )
		{
			/* First strip off comments */
			if( s=strstr( buf, "//" ) )
			{
				s[0]='\0';
			}
			/* Strip of whitespaces */
			if( s=strchr( buf, '=' ) )
			{
				s[0]='\0';
				s++;
				stripws( buf );
				stripws( s );
				g_strdown( buf );
				for( count=0 ; confargs[count].name ; count++ )
				{
					if( fastcompare( confargs[count].name, buf ) )
					{
						if( confargs[count].argtype==CA_ARG_INT )
						{
							*(gint *)confargs[count].arg=atoi( s );
							if( fastcompare( confargs[count].name, "quality" ) )
							{
								if( cfg->quality > 100 ) cfg->quality=100;
								if( cfg->quality < 0 ) cfg->quality=0;
								cfg->quality=(guint)((gfloat)cfg->quality*2.559);
							}
						}
						else if( confargs[count].argtype==CA_ARG_STR )
						{
							*(gchar **)confargs[count].arg=setstr( *(gchar **)confargs[count].arg, s );
						}
						else if( confargs[count].argtype==CA_ARG_BOOL )
						{
							if( a=strchr( answers, s[0] ) )
							{
								*(gboolean *)confargs[count].arg=((((gint)(a-answers))%2)==1);
							}
						}
					}
				}
			}
		}
		fclose( fp );
	}	
}

#define free_empty( s ) if( s && !s[0] ) s=setstr(s,NULL)

void free_empties( struct Global *cfg )
{
	free_empty( cfg->bgcolor );
	free_empty( cfg->bevelbright );
	free_empty( cfg->beveldark );
	free_empty( cfg->bevelback );
	free_empty( cfg->title );
	free_empty( cfg->bodyargs );
	free_empty( cfg->cellargs );
	free_empty( cfg->css );
	free_empty( cfg->left );
	free_empty( cfg->space );
	free_empty( cfg->divider );
	free_empty( cfg->right );
	free_empty( cfg->previous );
	free_empty( cfg->next );
	free_empty( cfg->index );
	free_empty( cfg->parent );
	free_empty( cfg->parentdoc );
}

void cleanup( struct Global *cfg )
{
	if( cfg )
	{
		if( cfg->bgcolor ) g_free( cfg->bgcolor );
		if( cfg->bevelback ) g_free( cfg->bevelback );
		if( cfg->bevelbright ) g_free( cfg->bevelbright );
		if( cfg->beveldark ) g_free( cfg->beveldark );
		if( cfg->dir ) g_free( cfg->dir );
		if( cfg->thumbdir ) g_free( cfg->thumbdir );
		if( cfg->title ) g_free( cfg->title );
		if( cfg->bodyargs ) g_free( cfg->bodyargs );
		if( cfg->cellargs ) g_free( cfg->cellargs );
		if( cfg->css ) g_free( cfg->css );
		if( cfg->parentdoc ) g_free( cfg->parentdoc );
		if( cfg->left ) g_free( cfg->left );
		if( cfg->space ) g_free( cfg->space );
		if( cfg->divider ) g_free( cfg->divider );
		if( cfg->right ) g_free( cfg->right );
		if( cfg->previous ) g_free( cfg->previous );
		if( cfg->next ) g_free( cfg->next );
		if( cfg->index ) g_free( cfg->index );
		if( cfg->parent ) g_free( cfg->parent );
		g_free( cfg );
	}
}

struct Global *dupconf( struct Global *cfg )
{
	struct Global *local;
	if( !cfg ) return( NULL );
	if( !( local=g_new0( struct Global, 1 ) ) ) return( NULL );
	memcpy( local, cfg, sizeof( struct Global ) );
	if( cfg->bgcolor ) local->bgcolor=g_strdup( cfg->bgcolor );
	if( cfg->bevelback ) local->bevelback=g_strdup( cfg->bevelback );
	if( cfg->bevelbright ) local->bevelbright=g_strdup( cfg->bevelbright );
	if( cfg->beveldark ) local->beveldark=g_strdup( cfg->beveldark );
	if( cfg->dir ) local->dir=g_strdup( cfg->dir );
	if( cfg->thumbdir ) local->thumbdir=g_strdup( cfg->thumbdir );
	if( cfg->title ) local->title=g_strdup( cfg->title );
	if( cfg->bodyargs ) local->bodyargs=g_strdup( cfg->bodyargs );
	if( cfg->cellargs ) local->cellargs=g_strdup( cfg->cellargs );
	if( cfg->css ) local->css=g_strdup( cfg->css );
	if( cfg->parentdoc ) local->parentdoc=g_strdup( cfg->parentdoc );
	if( cfg->left ) local->left=g_strdup( cfg->left );
	if( cfg->space ) local->space=g_strdup( cfg->space );
	if( cfg->divider ) local->divider=g_strdup( cfg->divider );
	if( cfg->right ) local->right=g_strdup( cfg->right );
	if( cfg->previous ) local->previous=g_strdup( cfg->previous );
	if( cfg->next ) local->next=g_strdup( cfg->next );
	if( cfg->index ) local->index=g_strdup( cfg->index );
	if( cfg->parent ) local->parent=g_strdup( cfg->parent );
	return( local );
}


void error( gchar *msg )
{
	if( msg ) fprintf(stderr, "*ERROR* %s\n", msg );
	cleanup(global);
	global=NULL;
	exit(1);
}

void setglobaldefaults( void )
{
	global->quiet=DEFAULT_QUIET;
	global->showcredits=DEFAULT_SHOWCREDITS;
	global->makethumbs=DEFAULT_MAKETHUMBS;
	global->ThumbWidth=DEFAULT_THUMBWIDTH;
	global->ThumbHeight=DEFAULT_THUMBHEIGHT;
	global->quality=(guint)((gfloat)DEFAULT_QUALITY*2.559);
	global->thumbscale=DEFAULT_THUMBSCALE;								
	global->bgcolor=setstr( global->bgcolor, DEFAULT_BGCOLOR );
	global->bevelback=setstr( global->bevelback, DEFAULT_BEVELBACK );
	global->bevelbright=setstr( global->bevelbright, DEFAULT_BEVELBRIGHT );
	global->beveldark=setstr( global->beveldark, DEFAULT_BEVELDARK );
	global->bevel=DEFAULT_BEVEL;
	global->recursive=DEFAULT_RECURSIVE;
	global->overwrite=DEFAULT_OVERWRITE;
	global->pad=DEFAULT_PAD;
	global->dir=setstr( global->dir, DEFAULT_DIR );
	global->thumbdir=setstr( global->thumbdir, DEFAULT_THUMBDIR );
	global->genindex=DEFAULT_GENINDEX;
	global->titles=DEFAULT_TITLES;
	global->numlink=DEFAULT_NUMLINK;
	global->xstop=DEFAULT_XSTOP;
	global->ystop=DEFAULT_YSTOP;
	global->bodyargs=setstr( global->bodyargs, DEFAULT_BODYARGS );
	global->cellargs=setstr( global->cellargs, DEFAULT_CELLARGS );
	global->css=setstr( global->css, DEFAULT_CSS );

	global->parentdoc=setstr( global->parentdoc, DEFAULT_PARENTDOC );
	global->left=setstr( global->left, DEFAULT_STR_LEFT );
	global->space=setstr( global->space, DEFAULT_STR_SPACE );
	global->divider=setstr( global->divider, DEFAULT_STR_DIVIDER );
	global->right=setstr( global->right, DEFAULT_STR_RIGHT );
	global->previous=setstr( global->previous, DEFAULT_STR_PREVIOUS );
	global->next=setstr( global->next, DEFAULT_STR_NEXT );
	global->index=setstr( global->index, DEFAULT_STR_INDEX );
	global->parent=setstr( global->parent, DEFAULT_STR_PARENT );
}

void makethumbs( gchar *dir, struct ProcessInfo *processinfo, gint level, struct Global *cfg )
{
	DIR *dirhandle;
	gchar *str;
	struct dirent *de;
	struct stat buf;
	GString *path=g_string_new(NULL);
	GString *thumbpath=g_string_new(NULL);
	gboolean slash=(dir[strlen(dir)-1]!='/'), thumbslash=(global->thumbdir[strlen(global->thumbdir)-1]!='/');

	gint w, h, x1, y1, x2, y2;
	gdouble factor;
	GList *thumbs = NULL;
	struct ThumbData *td;
	struct ProcessInfo *pi;
	GdkImlibSaveInfo si={ 0, 0, 0, 0, 0, 0 };
	struct Global *local;
	local=dupconf( cfg );
	g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", global->thumbdir, thumbslash?"/":"", "gfxindex" );
	readconf( thumbpath->str, local );
	g_string_sprintf( thumbpath, "%s%s%s", dir, slash?"/":"", ".gfxindex" );
	readconf( thumbpath->str, local );
	free_empties( local );
	si.quality=local->quality;
	//printf("%s\n", dir );
	//if( processinfo ) processinfo=FALSE;

	if( processinfo ) pi=processinfo;
	else
	{
		GdkColor col[3]=
		{
			0, 0x0000, 0x0000, 0x0000, /* black */
			0, 0xffff, 0xffff, 0xffff, /* White */
			0, 0x0000, 0x0000, 0x0000  /* Black */
		};

		if( !( pi=g_new0( struct ProcessInfo, 1 ) ) )
		{
			error( "Couldn't allocate memory" );
		}

		memcpy( col, pi->col, sizeof( GdkColor )*3 );
		pi->up=FALSE;
	}
	/* Initialize our workspace */
	pi->thumb=gdk_pixmap_new( NULL, local->ThumbWidth, local->ThumbWidth, 24 );
	pi->gc=gdk_gc_new( pi->thumb );
	if( local->bevel )
	{
		if( !gdk_color_parse( local->bevelback, &pi->col[0] ) )
		{
			pi->col[0].red=pi->col[0].green=pi->col[0].blue=0xc0c0;
			g_warning( "Couldn't parse color" );
		}
		gdk_imlib_best_color_get( &pi->col[0] );
		if( !gdk_color_parse( local->bevelbright, &pi->col[1] ) ) g_warning( "Couldn't parse color" );
		gdk_imlib_best_color_get( &pi->col[1] );
		if( !gdk_color_parse( local->beveldark, &pi->col[2] ) ) g_warning( "Couldn't parse color" );
		gdk_imlib_best_color_get( &pi->col[2] );
	}
	else
	{
		if( !gdk_color_parse( local->bgcolor, &pi->col[0] ) ) g_warning( "Couldn't parse color" );
		gdk_imlib_best_color_get( &pi->col[0] );
	}

	slash=(dir[strlen(dir)-1]!='/');
	thumbslash=(local->thumbdir[strlen(local->thumbdir)-1]!='/');
	if( dirhandle=opendir( dir ) )
	{
		/* First we see to it that the thumbdir exist */
		g_string_sprintf( thumbpath, "%s%s%s", dir, slash?"/":"", local->thumbdir );
		if( !file_exist( thumbpath->str ) ) mkdir( thumbpath->str, 0755 );

		/* If we don't want to overwrite everything. Load the current thumblist */
		g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", "files.db" );
		if( !local->overwrite ) thumbs=loadthumblist( thumbs, thumbpath->str );

		while( de=readdir( dirhandle ) )
		{
			/* Skip entries with leading dot */
			if( de->d_name[0]!='.' )
			{
				/* Create whole path to the file */
				g_string_sprintf( path, "%s%s%s", dir, slash?"/":"", de->d_name );
				if( stat( path->str, &buf ) ) perror( path->str );
				else
				{
					if( S_ISDIR(buf.st_mode) && local->recursive )
					{
						makethumbs( path->str, pi, level+1, local );
					}
					else if( S_ISREG(buf.st_mode) && local->makethumbs )
					{
						if( checkext( path->str ) )
						{
							g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", de->d_name );

							/* Replace the extension with jpg */
							if( str=strrchr( thumbpath->str, '.' ) )
							{
								g_string_truncate( thumbpath, str-thumbpath->str+1 );
								g_string_append( thumbpath, "jpg" );
							}

							if( local->overwrite || !file_exist( thumbpath->str ) )
							{
								/* Create the actual thumbnail */
								/* First we clear the area */
								gdk_gc_set_foreground( pi->gc, &pi->col[0] );
								gdk_draw_rectangle( pi->thumb, pi->gc, TRUE, 0, 0, local->ThumbWidth, local->ThumbHeight );

								/* Draw outer bevel */
								if( local->bevel )
								{
									gdk_gc_set_foreground( pi->gc, &pi->col[1] );
									gdk_draw_line( pi->thumb, pi->gc, 0, 0, local->ThumbWidth-2, 0 );
									gdk_draw_line( pi->thumb, pi->gc, 0, 1, 0, local->ThumbHeight-2 );
									gdk_gc_set_foreground( pi->gc, &pi->col[2] );
									gdk_draw_line( pi->thumb, pi->gc, local->ThumbWidth-1, 1, local->ThumbWidth-1, local->ThumbHeight-1 );
									gdk_draw_line( pi->thumb, pi->gc, 0, local->ThumbHeight-1, local->ThumbWidth-2, local->ThumbHeight-1 );
								}

								/* Then we load the original image */
								if( pi->im=gdk_imlib_load_image( path->str ) )
								{
									if( !local->quiet )
									{
										if( pi->up ) printf( "\x1bM\x1b[K" );
										printf( "Creating thumbnail for %s...\n", path->str );
										pi->up=TRUE;
									}
									/* Calculate scaling factor */
									if( pi->im->rgb_width > pi->im->rgb_height )
										factor=(gdouble)pi->im->rgb_width / (gdouble)local->ThumbWidth;
									else
										factor=(gdouble)pi->im->rgb_height / (gdouble)local->ThumbHeight;

									/* The width and height of the scaled down image */
									w=pi->im->rgb_width/factor;
									h=pi->im->rgb_height/factor;

									/* Draw inner bevel */
									if( local->bevel )
									{
										w-=8;
										h-=8;
										x1=((local->ThumbWidth-w )/2)-1;
										y1=((local->ThumbHeight-h )/2)-1;
										x2=x1+(w+1);
										y2=y1+(h+1);

										gdk_gc_set_foreground( pi->gc, &pi->col[2] );
										gdk_draw_line( pi->thumb, pi->gc, x1, y1, x2-1, y1 );
										gdk_draw_line( pi->thumb, pi->gc, x1, y1+1, x1, y2-1 );
										gdk_gc_set_foreground( pi->gc, &pi->col[1] );
										gdk_draw_line( pi->thumb, pi->gc, x2, y1+1, x2, y2 );
										gdk_draw_line( pi->thumb, pi->gc, x1+1,y2, x2-1, y2 );
									}

									/* Scale and place the image in the thumbnail */
									if( local->pad )
									{
										gdk_imlib_paste_image( pi->im, pi->thumb, (local->ThumbWidth-w)/2, (local->ThumbHeight-h)/2, w, h);
										pi->thumbim=gdk_imlib_create_image_from_drawable( pi->thumb, NULL, 0, 0, local->ThumbWidth, local->ThumbHeight );
									}
									else
									{
										gdk_imlib_paste_image( pi->im, pi->thumb, 0, 0, w, h);
										pi->thumbim=gdk_imlib_create_image_from_drawable( pi->thumb, NULL, 0, 0, w, h );
									}
									str=thumbpath->str;
									str+=strlen( dir );
									if( slash ) str++;
									if( td=findnode( thumbs, de->d_name ) )
									{
										td->imagewidth=pi->im->rgb_width;
										td->imageheight=pi->im->rgb_height;
										td->thumbwidth=pi->thumbim->rgb_width;
										td->thumbheight=pi->thumbim->rgb_height;
										td->image=setstr( NULL, de->d_name );
										td->thumb=setstr( NULL, str );
									}
									else if( td=g_new0( struct ThumbData, 1 ) )
									{
										td->imagewidth=pi->im->rgb_width;
										td->imageheight=pi->im->rgb_height;
										td->thumbwidth=pi->thumbim->rgb_width;
										td->thumbheight=pi->thumbim->rgb_height;
										td->image=setstr( NULL, de->d_name );
										td->thumb=setstr( NULL, str );
										thumbs=g_list_prepend( thumbs, td );
									} else g_warning( "Couldn't allocate memory for thumbnail-list node. Creating thumbnails anyway." );
									gdk_imlib_save_image( pi->thumbim, thumbpath->str, &si);
									gdk_imlib_kill_image( pi->thumbim );
									gdk_imlib_kill_image( pi->im );
								}
							}
						}
					}
				}
			}
		}
		if( local->makethumbs )
		{
			g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", "files.db" );
			thumbs=g_list_sort( thumbs, (GCompareFunc)thumbcomp );
			savethumblist( thumbs, thumbpath->str );
		}
		closedir( dirhandle );
		if( local->genindex ) gfxindex( local, dir, thumbs, level );
	}
	else perror( "opendir" );
	if( path ) g_string_free( path, TRUE );
	if( thumbpath ) g_string_free( thumbpath, TRUE );
	g_list_foreach( thumbs, freenode, NULL );
	g_list_free( thumbs );
	if( !processinfo )
	{
		gdk_pixmap_unref( pi->thumb );
		g_free( pi );
	}
	cleanup( local );
	local=NULL;
}

void gfxindex( struct Global *local, gchar *dir, GList *thumbs, gint level )
{
	guint numpics=g_list_length( thumbs );
	guint ppp=local->xstop * local->ystop;
	guint xcount=0, ycount=0, page=0, count;
	GString *path=g_string_new(NULL);
	GString *index=g_string_new(NULL);
	GString *thumbindex=g_string_new(NULL);
	gboolean slash=(dir[strlen(dir)-1]!='/'), thumbslash=(local->thumbdir[strlen(local->thumbdir)-1]!='/');
	guint numpages=numpics/ppp;
	FILE *file=NULL, *thumbfile=NULL;
	struct ThumbData *td;
	char *tmpstr;
	GList *node;
	if( (gfloat)numpics/(gfloat)ppp > numpages ) numpages++;
	for( node=thumbs ; node ; node=node->next )
	{
		td=(struct ThumbData *)node->data;
		g_string_sprintf( path, "%s%s%s.html", dir, slash?"/":"", td->thumb );
		if( !( thumbfile=fopen( path->str, "w" ) ) ) goto error;
		g_string_sprintf( path, "../%s", indexstr( page ) );
		fprintf( thumbfile, "<HTML>\n <HEAD>\n  <TITLE>%s%s%s ( %ld x %ld )</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " by Fredrik Rambris (fredrik.rambris@amiga.nu)\">\n", local->title?local->title:"", local->title?" - ":"", td->image, td->imagewidth, td->imageheight );
		if( local->css )
		{
			fprintf( thumbfile, "  <LINK REL=\"stylesheet\" HREF=\"" );
			for( count=0 ; count<=level ; count ++ ) fprintf( thumbfile, "../" );
			fprintf( thumbfile, "%s\" TYPE=\"text/css\">\n", local->css );
		}
		fprintf( thumbfile, " </HEAD>\n <BODY%s%s>\n  <DIV ALIGN=\"center\">\n", local->bodyargs?" ":"", local->bodyargs?local->bodyargs:"" );
		if( numpics>1 ) navbar_new( thumbindex );
		if( node->prev && ( local->previous || local->thumbscale ) )
		{
			if( !( tmpstr=strrchr( ((struct ThumbData *)node->prev->data)->thumb, '/' ) ) ) tmpstr=((struct ThumbData *)node->prev->data)->thumb;
			else tmpstr++;

			if( local->thumbscale )
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" ALT=\"%s\" BORDER=\"0\"></A>", tmpstr, tmpstr, (long)(((struct ThumbData *)node->prev->data)->thumbwidth*local->thumbscale/100), (long)(((struct ThumbData *)node->prev->data)->thumbheight*local->thumbscale/100), local->previous );
			}
			else
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, local->previous );
			}
		}
		if( local->index ) navbar_add( local, thumbindex, "<A HREF=\"%s\">%s</A>", path->str, local->index );
		if( node->next && ( local->next || local->thumbscale ) )
		{
			if( !( tmpstr=strrchr( ((struct ThumbData *)node->next->data)->thumb, '/' ) ) ) tmpstr=((struct ThumbData *)node->next->data)->thumb;
			else tmpstr++;

			if( local->thumbscale )
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" ALT=\"%s\" BORDER=\"0\"></A>", tmpstr, tmpstr, (long)(((struct ThumbData *)node->next->data)->thumbwidth*local->thumbscale/100), (long)(((struct ThumbData *)node->next->data)->thumbheight*local->thumbscale/100), local->next );
			}
			else
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, local->next );
			}
		}
		if( numpics>1 ) navbar_end( local, thumbindex );
		if( numpics>1 ) fprintf( thumbfile, "   <SPAN ID=\"navbar\">%s</SPAN><BR>\n", thumbindex->str );
		fprintf( thumbfile, "   <A HREF=\"%s\"><IMG SRC=\"../%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\" VSPACE=\"2\"></A><BR>\n", path->str, td->image, td->imagewidth, td->imageheight );
		if( numpics>1 ) fprintf( thumbfile, "   <SPAN ID=\"navbar\">%s</SPAN>\n", thumbindex->str );
		if( local->showcredits ) fprintf( thumbfile, "   <HR>\n   <SPAN ID=\"credits\">Created using <A HREF=\"http://boost.linux.kz/gfxindex/\">GFXIndex</A> v" VERSION " by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></SPAN>\n" );
		fprintf( thumbfile, "  </DIV>\n </BODY>\n</HTML>\n" );
		fclose( thumbfile );
		thumbfile=NULL;
		if( ycount+xcount==0 )

		{
			ycount=0;
			xcount=0;
			g_string_sprintf( path, "%s%s%s", dir, slash?"/":"", indexstr( page ) );
			if( !( file=fopen( path->str, "w" ) ) ) goto error;
			fprintf( file, "<HTML>\n <HEAD>\n  <TITLE>%s%sPage %ld / %ld</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " by Fredrik Rambris (fredrik.rambris@amiga.nu)\">\n", local->title?local->title:"", local->title?" - ":"", page+1, numpages );
			if( local->css )
			{
				fprintf( file, "  <LINK REL=\"stylesheet\" HREF=\"" );
				for( count=0 ; count<level ; count ++ ) fprintf( file, "../" );
				fprintf( file, "%s\" TYPE=\"text/css\">\n", local->css );
			}
			fprintf( file, " </HEAD>\n <BODY%s%s>\n  <DIV ALIGN=\"center\">\n",  local->bodyargs?" ":"", local->bodyargs?local->bodyargs:"" );

			if( numpages>1 )
			{
				navbar_new( index );
				if( page>0 && local->previous ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page-1 ), local->previous );

				if( local->parent && local->parentdoc ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", local->parentdoc, local->parent );
				if( local->numlink )
				{
					for( count=0 ; count<numpages ; count++ )
					{
						if( count!=page ) navbar_add( local, index, "<A HREF=\"%s\">", indexstr( count ) );
						else navbar_add( local, index, "<SPAN ID=\"current\">" );
						g_string_sprintfa( index, "%ld", count+1 );
						if( count!=page ) g_string_sprintfa( index, "</A>" );
						else g_string_sprintfa( index, "</SPAN>" );
					}
				}
				if( page<(numpages-1) && local->next ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page+1 ), local->next );
				navbar_end( local, index );
				fprintf( file, "   <SPAN ID=\"navbar\">%s</SPAN><BR>\n", index->str );
			} else g_string_truncate( index, 0 );
			fprintf( file, "   <TABLE BORDER=\"0\" CELLPADDING=\"2\" CELLSPACING=\"0\">\n" );
		}
		if( xcount==0 ) fprintf( file, "     <TR>\n" );
		fprintf( file, "     <TD%s%s><A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\">%s%s</A></TD>\n", local->cellargs?" ":"", local->cellargs?local->cellargs:"", td->thumb, td->thumb, td->thumbwidth, td->thumbheight, local->titles?"<BR>":"", local->titles?td->image:"" );
		xcount++;
		if( xcount>=local->xstop )
		{
			xcount=0;
			ycount++;
			fprintf( file, "    </TR>\n" );
		}
		if( ycount>=local->ystop )
		{
			ycount=0;
			page++;
			fprintf( file, "   </TABLE>\n" );
			if( numpages>1 ) fprintf( file, "    <SPAN ID=\"navbar\">%s</SPAN>\n", index->str ); 
			if( local->showcredits ) fprintf( file, "   <HR>\n   <SPAN ID=\"credits\">Created using <A HREF=\"http://boost.linux.kz/gfxindex/\">GFXIndex</A> v" VERSION " by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></SPAN>\n" );
			fprintf( file, "  </DIV>\n </BODY>\n</HTML>\n" );
			fclose( file );
			file=NULL;
		}
	}
	if( file )
	{
		fprintf( file, "    </TR>\n   </TABLE>\n" );
		if( numpages>1 ) fprintf( file, "    <SPAN ID=\"navbar\">%s</SPAN>\n", index->str ); 
		if( local->showcredits ) fprintf( file, "   <HR>\n   <SPAN ID=\"credits\">Created using <A HREF=\"http://boost.linux.kz/gfxindex/\">GFXIndex</A> v" VERSION " by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></SPAN>\n" );
		fprintf( file, "  </DIV>\n </BODY>\n</HTML>\n" );
		fclose( file );
		file=NULL;
	}
	error:
	if( file ) fclose( file );
	if( thumbfile ) fclose( thumbfile );
	g_string_free( path, TRUE );
	g_string_free( index, TRUE );
	g_string_free( thumbindex, TRUE );
}

gchar *indexstr( int number )
{
	static gchar buf[20];
	if( number>0 ) sprintf( buf, "index%ld.html", number );
	else sprintf( buf, "index.html" );
	return( buf );
}

void navbar_new( GString *str )
{
	g_string_truncate( str, 0 );
}

void navbar_add( struct Global *local, GString *str, gchar *newstr, ... )
{
	va_list ap;
	gchar buf[512];
	va_start( ap, newstr );
	vsnprintf( buf, 511, newstr, ap );
	if( str->str[0] ) { if( local->space ) g_string_append( str, local->space ); if( local->divider ) g_string_append( str, local->divider ); }
	else { if( local->left ) g_string_append( str, local->left ); }
	if( local->space ) g_string_append( str, local->space );
	g_string_append( str, buf );
	va_end( ap );
}

void navbar_end( struct Global *local, GString *str )
{
	if( local->space ) g_string_append( str, local->space );
	if( local->right ) g_string_append( str, local->right );
}

gchar *setstr( gchar *old, gchar *new )
{
	if( old )
	{
		g_mem_check( old );
		g_free( old );
		old=NULL;
	}

	if( new )
	{
		if( old=(gchar *)g_malloc( strlen( new )+1 ) )
		{
			strcpy( old, new );
		}
	}
	return( old );
}

gboolean checkext( gchar *file )
{
	gchar *str;
	/* Extract the extension */
	if( !( str=strrchr( file, '.') ) ) return( FALSE );
	str++;

	/* These are hardcoded for now... will be replaced by some smarter thing later on */
	if( !strcasecmp( str, "jpg") ) return( TRUE );
	if( !strcasecmp( str, "jpeg") ) return( TRUE );
	if( !strcasecmp( str, "gif") ) return( TRUE );
	if( !strcasecmp( str, "png") ) return( TRUE );
	/* I've commented these out because webbrowsers generally doesn't support them anyway */
	//if( !strcasecmp( str, "tif") ) return( TRUE );
	//if( !strcasecmp( str, "xpm") ) return( TRUE );
	//if( !strcasecmp( str, "tga") ) return( TRUE );

	return( FALSE );
}

gint dircomp( const struct dirent **a, const struct dirent **b )
{
	return( g_strcasecmp( b[0]->d_name, a[0]->d_name ) );
}

gint thumbcomp( gpointer a, gpointer b )
{
	struct ThumbData *tda=a, *tdb=b;
	return( g_strcasecmp( tda->image, tdb->image ) );
}

void savethumblist( GList *thumbs, gchar *file )
{
	FILE *fp;
	struct ThumbData *td;
	if( !thumbs || !file ) return;
	if( fp=fopen( file, "w+" ) )
	{
		for( ; thumbs ; thumbs=thumbs->next )
		{
			td=(struct ThumbData *)thumbs->data;
			fprintf( fp, "%s;%ld;%ld;%s;%ld;%ld%s%s\n", td->image, td->imagewidth, td->imageheight, td->thumb, td->thumbwidth, td->thumbheight, td->extra?";":"", td->extra?td->extra:"" );
		}
		fclose( fp );
	}
}

GList *loadthumblist( GList *thumbs, gchar *file )
{
	FILE *fp;
	struct ThumbData *td;
	static gchar buf[1024];
	gchar **line;
	if( !file ) return( thumbs );
	if( !file_exist( file ) ) return( thumbs );
	if( fp=fopen( file, "r" ) )
	{
		while( fgets( buf, 1023, fp ) )
		{
			g_strstrip( buf );
			if( line=g_strsplit( buf, ";", 6 ) )
			{
				if( td=g_new0( struct ThumbData, 1 ) )
				{
					td->image=setstr( NULL, line[0] );
					td->thumb=setstr( NULL, line[3] );
					td->imagewidth=atoi( line[1] );
					td->imageheight=atoi( line[2] );
					td->thumbwidth=atoi( line[4] );
					td->thumbheight=atoi( line[5] );
					td->extra=setstr( NULL, line[6] );
					thumbs=g_list_prepend( thumbs, td );
				} else g_warning( "Couldn't allocate memory for node" );
				g_strfreev( line );
			} else g_warning( "Couldn't split up line" );
		}
		thumbs=g_list_sort( thumbs, (GCompareFunc)thumbcomp );
	}
	else g_warning( "Couldn't load thumbnail list" );
	return( thumbs );
}

void freenode( gpointer data, gpointer user_data )
{
	struct ThumbData *td=data;
	td->image=setstr( td->image, NULL );
	td->thumb=setstr( td->thumb, NULL );
	g_free( td );
}

gboolean fastcompare( gchar *a, gchar *b )
{
	for( ; a[0] && b[0] ; a++, b++ )
	{
		if( a[0]!=b[0] ) return( FALSE );
	}
	if( a[0]!=b[0] ) return( FALSE );
	return( TRUE );
}

GList *removenode( GList *thumbs, gchar *imagename )
{
	GList *node, *next;
	struct ThumbData *td;
	for( node=thumbs; node ; node=next )
	{
		next=node->next;
		td=(struct ThumbData *)node->data;
		if( fastcompare( td->image, imagename ) )
		{
			thumbs=g_list_remove_link( thumbs, node );
			freenode( node->data, NULL );
			g_list_free_1( node );
		}
	}
	return( thumbs );
}

struct ThumbData *findnode( GList *thumbs, gchar *imagename )
{
	GList *node;
	struct ThumbData *td;
	for( node=thumbs; node ; node=node->next )
	{
		td=(struct ThumbData *)node->data;
		if( fastcompare( td->image, imagename ) )
		{
			return( td );
		}
	}
	return( NULL );
}

gboolean file_exist( gchar *filename )
{
	struct stat buf;
	return( stat( filename, &buf )==0 );
}
