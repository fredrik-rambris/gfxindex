/* args.c - Argument and config handling
 * GFXIndex (c) 1999-2001 Fredrik Rambris <fredrik@rambris.com>.
 * All rights reserved.
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
 */


#include "global.h"
#include "args.h"

gint args_init( int argc, char **argv )
{
	if( !( global=g_new0( struct Global, 1 ) ) ) error( "Couldn't allocate memory" );
	setglobaldefaults();
	readconf( "/etc/gfxindex", global );
	readconf( "~/.gfxindex", global );
	if( !handleargs( argc, argv, global ) ) error( NULL );
	free_empties( global );
	return( 0 );
}

gint handleargs( int argc, char **argv, struct Global *cfg )
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

	optCon = poptGetContext( NULL, argc, (const char **)argv, optionsTable, 0 );
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
				cfg->quality=(gint)atoi( (char *)poptGetOptArg( optCon ) );
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
				if( strlen( poptGetOptArg( optCon ) ) ) cfg->bgcolor=setstr( cfg->bgcolor, (char *)poptGetOptArg( optCon ) );
				else cfg->bgcolor=setstr( cfg->bgcolor, NULL );
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
				printf( "thumbwidth       %d\n", DEFAULT_THUMBWIDTH );
				printf( "thumbheight      %d\n", DEFAULT_THUMBHEIGHT );
				printf( "quality          %d\n", DEFAULT_QUALITY );
				printf( "thumbbgcolor     %s\n", DEFAULT_BGCOLOR );
				printf( "thumbbevel       %s\n", DEFAULT_BEVEL?"Yes":"No" );
				printf( "bevelbg          %s\n", DEFAULT_BEVELBACK );
				printf( "bevelbright      %s\n", DEFAULT_BEVELBRIGHT );
				printf( "beveldark        %s\n", DEFAULT_BEVELDARK );
				printf( "Create indexes?  %s\n", DEFAULT_GENINDEX?"Yes":"No" );
				printf( "Show filenames?  %s\n", DEFAULT_TITLES?"Yes":"No" );
				printf( "Use NavThumbs?   %s (%d %%)\n", DEFAULT_THUMBSCALE?"Yes":"No", DEFAULT_THUMBSCALE );
				printf( "numx             %d\n", DEFAULT_XSTOP );
				printf( "numy             %d\n", DEFAULT_YSTOP );
				printf( "bodyargs         %s\n", DEFAULT_BODYARGS );
				printf( "cellargs         %s\n", DEFAULT_CELLARGS );
				printf( "css              %s\n", DEFAULT_CSS );
				return( FALSE );
				break;
		}
	}
      	cfg->title=setstr( cfg->title, (char *)poptGetArg( optCon ) );

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
		{ "hideext", CA_ARG_BOOL, &cfg->hideext },
		{ NULL, 0, NULL }
	};

	if( ( fp=fopen( filename, "r" ) ) )
	{
		while( fgets( buf, 1023, fp ) )
		{
			/* First strip off comments */
			if( ( s=strstr( buf, "//" ) ) )
			{
				s[0]='\0';
			}
			/* Strip of whitespaces */
			if( ( s=strchr( buf, '=' ) ) )
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
							if( ( a=strchr( answers, s[0] ) ) )
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

void freeconf( struct Global *cfg )
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
		if( ( old=(gchar *)g_malloc( strlen( new )+1 ) ) )
		{
			strcpy( old, new );
		}
	}
	return( old );
}
