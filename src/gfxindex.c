/* gfxindex.c - Main sceleton
 * GFXindex (c) 1999-2004 Fredrik Rambris <fredrik@rambris.com>.
 * All rights reserved.
 *
 * GFXindex is a tool that creates thumbnails and HTML-indexes of your images. 
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
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifndef __WIN32__
#include <libgen.h>
#endif
#include "global.h"
#include "io_jpeg.h"
#include "gfxio.h"
#include "gfx.h"
#include "exif.h"
#include "util.h"
#include "confargs.h"
#include "thumbdata.h"
#include "xml.h"

struct Backgrounds
{
	struct image *thumbbackground, *thumbalpha;
};

int traverse( char *dir, int level, ConfArg *confg, struct Picture *dirthumb );
void cleanup( void );
int scaleoriginal( char *imagefile, char *destdir, ConfArg *cfg, struct PictureNode *pn );
void gfxindex( ConfArg *local, char *dir, List *thumbs, int level );
char *strip_path( char *path, int number );
char *indexstr( int number );
void error( char *msg );
void status( int level, ConfArg *cfg, char *fmt, ... );
void navbar_new( char *str );
void navbar_add( ConfArg *local, char *str, char *newstr, ... );
void navbar_end( ConfArg *local, char *str );


void conf_check( ConfArg *ca )
{
	if( BCONF(ca,PREFS_THUMBBEVEL) )
	{
		CONF(ca,PREFS_PAD)=(void *)(1);
		CONF(ca,PREFS_SOFTPAD)=(void *)(0);
	}
}

/*
void about( void )
{
	printf( "GFXindex v" VERSION " (" __DATE__ "), Copyright (C) 1999-2003  Fredrik Rambris\n\n" );
	printf( "GFXindex comes with ABSOLUTELY NO WARRANTY; for details see the file 'COPYING'\n" );
	printf( "This is free software, and you are welcome to redistribute it under the terms\n" );
    printf( "of GNU General Public License. See http://www.gnu.org for more info\n\n" );
}
*/
int main( int argc, char **argv )
{
//	char file[1024];
	if( gfxio_init() ) error( "Couldn't initiate image I/O" );
//	printioinfo();
	if( !( global_confarg=confargs_new( config_definition, conf_check ) ) ) error( "Couldn't initalize preferences memory" );
	if( !confargs_commandline( global_confarg, argc, argv ) ) error( "Couldn't parse command line" );
	if( STR_ISSET(SCONF(global_confarg,PREFS_CONFIG) ) )
	{
		if( file_exist( SCONF(global_confarg,PREFS_CONFIG) ) )
		{
			confargs_load( global_confarg, SCONF(global_confarg,PREFS_CONFIG) );
		}
	}
	status( 1, global_confarg, "GFXindex v" VERSION " (c) 1999-2004 Fredrik Rambris. Built on " __DATE__ );
	if( STR_ISSET(SCONF(global_confarg,PREFS_SAVECONFIG) ) )
	{
		confargs_save( global_confarg, SCONF(global_confarg,PREFS_SAVECONFIG) );
		cleanup();
		return 0;
	}

	traverse( SCONF(global_confarg,PREFS_DIR), 0, global_confarg, NULL );
	cleanup();
	return 0;
}

/* A simple give a message and die */
void error( char *msg )
{
	if( msg ) fprintf(stderr, "*ERROR* %s\n", msg );
	cleanup();
	global_confarg=NULL;
	exit(1);
}

/* Report status to user.
 * Level 1: Basic info (i.e. which file that it's processing)
 * Level 2: More info (i.e. that it's loading, scaling, saving etc)
 *
 * It's written kinda stupid to be able to replace it with a GUI one
 */
void status( int level, ConfArg *cfg, char *fmt, ... )
{
	va_list ap;
	if( ICONF(cfg,PREFS_VERBOSE)>=level )
	{
		va_start( ap, fmt );
		vprintf( fmt, ap );
		printf( "\n" );
		va_end( ap );
	}
}

void cleanup( void )
{
	gfxio_cleanup();
	confargs_free( global_confarg );
}

void tprintf( char *buf, char *fmt, struct PictureNode *pn, struct Picture *pict, ConfArg *cfg, int page, int numpages )
{
	char *in=fmt, *out=buf;
	int ret;
	char tmp[1024], *ptr;
	struct tm ftm, ctm, *tm;
	time_t timep;
	time( &timep );
	if( ( tm=localtime( &timep ) ) )
		memcpy( &ctm, tm, sizeof( struct tm ) );
	struct stat statbuf;
	if( pn && pn->pn_original.p_path )
	{
		if( !stat( pn->pn_original.p_path, &statbuf ) )
		{
			if( ( tm=localtime( &(statbuf.st_mtime) ) ) )
				memcpy( &ftm, tm, sizeof( struct tm ) );
		}
	}
	while( *in )
	{
		if( *in=='%' )
		{
			switch( in[1] )
			{
				case '%':
					out[0]='%';
					in+=2;
					out++;
					out[0]='\0';
					break;
				case 'f': /* Filename */
					if( pn && pn->pn_original.p_path )
					{
						strcpy( tmp, pn->pn_original.p_path );
						ret=sprintf( out, "%s", basename( tmp ) );
						out+=ret;
						out[0]='\0';
					}
					in+=2;
				case 'F': /* Filename w/o extension */
					if( pn && pn->pn_original.p_path )
					{
						strcpy( tmp, pn->pn_original.p_path );
						if( ( ptr=strrchr( tmp, '.' ) ) ) ptr[0]='\0';
						ret=sprintf( out, "%s", basename( tmp ) );
						out+=ret;
						out[0]='\0';
					}
					in+=2;
				case 'y': /* Current year */
					ret=sprintf( out, "%d", 1900+ctm.tm_year );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'm': /* Current month */
					ret=sprintf( out, "%02d", ctm.tm_mon );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'd': /* Current day of month */
					ret=sprintf( out, "%02d", ctm.tm_mday );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'Y': /* File year */
					ret=sprintf( out, "%d", 1900+ftm.tm_year );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'M': /* File month */
					ret=sprintf( out, "%02d", ftm.tm_mon );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'D': /* File day of month */
					ret=sprintf( out, "%02d", ftm.tm_mday );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'p': /* Page number */
					ret=sprintf( out, "%d", page );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'P': /* Number of pages */
					ret=sprintf( out, "%d", numpages );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 't': /* Page title */
					ret=sprintf( out, "%s", SCONF(cfg,PREFS_TITLE) );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'T': /* Picture title */
					ret=sprintf( out, "%s", pn->pn_title );
					out+=ret;
					out[0]='\0';
					in+=2;
					break;
				case 'w': /* Picture width */
					if( pict )
					{
						ret=sprintf( out, "%d", pict->p_width );
						out+=ret;
						out[0]='\0';
					}
					in+=2;
					break;
				case 'h': /* Picture height */
					if( pict )
					{
						ret=sprintf( out, "%d", pict->p_height );
						out+=ret;
						out[0]='\0';
					}
					in+=2;
					break;
#if HAVE_LIBEXIF
				case 'e': /* Exif date */
					if( pn && pn->pn_exifinfo && pn->pn_exifinfo->ei_date )
					{
						ret=sprintf( out, "%s", pn->pn_exifinfo->ei_date );
						out+=ret;
						out[0]='\0';
					}
					in+=2;
					break;
#endif
				default:
					out[0]=in[0];
					in++;
					out++;
					out[0]='\0';
			}
		}
		else
		{
			out[0]=in[0];
			in++;
			out++;
			out[0]='\0';
		}
	}
}

int scaleoriginal( char *imagefile, char *destdir, ConfArg *cfg, struct PictureNode *pn )
{
	struct image *im=NULL, *thumb=NULL, info;
	int w, h;
#ifdef HAVE_LIBJPEG
	int scale=1;
#endif
	double factor;
	char *destfile;
	char *ptr;
	int err=0;
	int *size;
	int num;
	struct Picture **pict;
	if( !imagefile || !destdir || !cfg ) return 1;
	if( !CONF(cfg,PREFS_WIDTHS) ) return 1;
	if( !BCONF(cfg,PREFS_FLAT ) ) return 1;
	size=(int *)CONF(cfg,PREFS_WIDTHS);
	num=arrlen( size );
	if( pn->pn_pictures )
	{
		pict=pn->pn_pictures;
		while( *pict )
		{
			free( *pict );
			*pict=NULL;
			pict++;
		}
		free( pn->pn_pictures );
		pn->pn_pictures=NULL;
	}
	if( ! (pn->pn_pictures=gfx_new0( struct Picture *, num+1 ) ) ) return 1;
	pict=pn->pn_pictures;
	status( 3, cfg, "Loading image..." );
#ifdef HAVE_LIBJPEG
	if( gfx_getinfo( imagefile, &info, TAG_DONE )==ERR_OK )
	{
		for( scale=8;scale>1;scale/=2 )
		{
			if( info.im_width>info.im_height )
			{
				if( (*size)<=(info.im_width/scale) ) break;
			}
			else
			{
				if( (*size)<=(info.im_height/scale) ) break;
			}
		}
	}
	status( 3, cfg, "Using a load scale of 1:%d", scale );
#endif
	if( ( im=gfx_load( imagefile, NULL,
#ifdef HAVE_LIBJPEG
		GFXIO_JPEG_SCALE, scale,
#endif
		TAG_DONE ) ) )
	{
		if( pn->pn_rotate )
		{
			gfx_rotate( im, pn->pn_rotate );
		}
		if( ( destfile=gfx_new0( char, strlen( (char *)basename( imagefile ) )+strlen( destdir )+strlen( SCONF(cfg,PREFS_THUMBDIR) )+10 ) ) )
		{
			while( *size )
			{
				strcpy( destfile, destdir );
				tackon( destfile, SCONF(cfg,PREFS_THUMBDIR) );
				tackon( destfile, (char *)basename( imagefile ) );
				if( ( ptr=strrchr( destfile, '.' ) ) )
				{
					sprintf( ptr, "_%d.jpg", *size );
				}
				if( !file_exist( destfile ) || BCONF(cfg,PREFS_OVERWRITE) || BCONF(cfg,PREFS_REMAKEBIGS) )
				{
					/* Calculate scaling factor */
					if( im->im_width > im->im_height )
						factor=(double)im->im_width / (double)(*size);
					else
						factor=(double)im->im_height / (double)(*size);
	
					/* The width and height of the scaled down image */
					w=im->im_width/factor;
					h=im->im_height/factor;
					status( 1, cfg, "Creating downscaled image (%d x %d)...", w, h );
					if( ( (*pict)=gfx_new0( struct Picture, 1 ) ) )
					{
						(*pict)->p_width=w;
						(*pict)->p_height=h;
						(*pict)->p_path=setstr( (*pict)->p_path, destfile+strlen(destdir) );
					}
					if( w==im->im_width && h==im->im_height )
					{
						status( 3, cfg, "Saving image '%s'...", destfile );
						gfx_save( destfile, im, 0,
#ifdef HAVE_LIBJPEG
							GFXIO_JPEG_QUALITY, ICONF(cfg,PREFS_QUALITY),
#endif
							TAG_DONE );
					}
					else
					{
						if( ( thumb=gfx_allocimage( w, h, &err ) ) )
						{
							status( 3, cfg, "Scaling from %dx%d to %dx%d...", im->im_width, im->im_height, w, h );
							gfx_scaleimage( im, 0,0, im->im_width, im->im_height, thumb, 0, 0, thumb->im_width, thumb->im_height, ICONF(cfg,PREFS_SCALE), FALSE );
							status( 3, cfg, "Saving image '%s'...", destfile );
							gfx_save( destfile, thumb,
#ifdef HAVE_LIBJPEG
								GFXIO_JPEG_QUALITY, ICONF(cfg,PREFS_BIGQUALITY),
#endif
								TAG_DONE );
							gfx_freeimage( im, FALSE );
							im=thumb;
						}
					}
				}
				else if( gfx_getinfo( destfile, &info, TAG_DONE )==ERR_OK )
				{
					if( ( (*pict)=gfx_new0( struct Picture, 1 ) ) )
					{
						(*pict)->p_width=info.im_width;
						(*pict)->p_height=info.im_height;
						(*pict)->p_path=setstr( (*pict)->p_path, destfile+strlen(destdir) );
					}
				}
				size++;
				pict++;
			}
			free( destfile );
		}
		else err=ERR_MEM;

		gfx_freeimage( im, FALSE );
	}
	return( err );
}

int makethumbnail( char *imagefile, char *destdir, ConfArg *cfg, struct color *col, struct Picture *picture, struct PictureNode *pn, struct Backgrounds *backgrounds )
{
	struct image *im=NULL, *thumb=NULL;
	struct image info={NULL, 0, 0 };
#ifdef HAVE_LIBJPEG
	int scale=1;
#endif
	char *thumbfile=NULL, *str=NULL, *path;
	int w, h, x1, y1, x2, y2;
	double factor;

	/* All arguments must be supplied */
	if( !imagefile || !destdir || !cfg  ) return( 1 );

	/* Create filename */
	if( !( thumbfile = gfx_new0( char, strlen( (char *)basename( imagefile ) ) + strlen( destdir ) + strlen( SCONF(cfg,PREFS_THUMBDIR) ) + 5 ) ) )
	{
		return( 1 );
	}
	if( !BCONF(cfg,PREFS_FLAT ) )
	{
		strcpy( thumbfile, destdir );
		tackon( thumbfile, SCONF(cfg,PREFS_THUMBDIR ) );
		path=thumbfile+strlen(thumbfile)-strlen(SCONF(cfg,PREFS_THUMBDIR));
		tackon( thumbfile, (char *)basename( imagefile ) );

		/* Replace the extension with jpg */
		if( ( str=strrchr( thumbfile, '.' ) ) )
		{
			str[1]='\0';
			strcat( thumbfile, "jpg" );
		}
	}
	else
	{
		strcpy( thumbfile, destdir );
		path=thumbfile+strlen(thumbfile);
		tackon( thumbfile, (char *)basename( imagefile ) );
		if( ( str=strrchr( thumbfile, '.' ) ) )
		{
			str[0]='\0';
			strcat( thumbfile, "_tn.jpg" );
		}
	}

//	confargs_show( cfg );
	if( BCONF(cfg,PREFS_OVERWRITE) || !file_exist( thumbfile ) || BCONF(cfg,PREFS_REMAKETHUMBS) )
	{
		status( 1, cfg, "Creating thumbnail for '%s'...", imagefile );
		status( 2, cfg, "Loading imagefile" );
#ifdef HAVE_LIBJPEG
		if( gfx_getinfo( imagefile, &info, TAG_DONE )==ERR_OK )
		{
			for( scale=8;scale>1;scale/=2 )
			{
				if( ICONF(cfg,PREFS_THUMBWIDTH)<(info.im_width/scale) && ICONF(cfg,PREFS_THUMBHEIGHT)<(info.im_height/scale) ) break;
			}
		}
		status( 3, cfg, "Using a load scale of 1:%d", scale );
#endif
		if( ( im=gfx_load( imagefile, NULL,
#ifdef HAVE_LIBJPEG
						GFXIO_JPEG_SCALE, scale,
#endif
						TAG_DONE ) ) )
		{
			if( pn->pn_rotate )
			{
				gfx_rotate( im, pn->pn_rotate );
			}
			/* Calculate scaling factor */
			if( im->im_width > im->im_height )
				factor=(double)im->im_width / (double)ICONF(cfg,PREFS_THUMBWIDTH);
			else
				factor=(double)im->im_height / (double)ICONF(cfg,PREFS_THUMBHEIGHT);

			/* The width and height of the scaled down image */
			w=im->im_width/factor;
			h=im->im_height/factor;
			status( 3, cfg, "Allocating thumbnail" );
			/* Allocate our workspace */
			
			if( ( thumb=gfx_allocimage( (BCONF(cfg,PREFS_PAD)?ICONF(cfg,PREFS_THUMBWIDTH):w), (BCONF(cfg,PREFS_PAD)?ICONF(cfg,PREFS_THUMBHEIGHT):h), NULL ) ) )
			{
				if( picture )
				{
					picture->p_width=thumb->im_width;
					picture->p_height=thumb->im_height;
				}
				/* Create the actual thumbnail */
				/* First we clear the area */
				status( 3, cfg, "Clearing thumbnail" );
				gfx_rectfill( thumb, 0, 0, thumb->im_width, thumb->im_height, &col[0] );


				/* Then we stack on our background */
				if( backgrounds && backgrounds->thumbbackground )
				{
					gfx_stack( backgrounds->thumbbackground, thumb );
				}
				/* Draw outer bevel */
				if( BCONF(cfg,PREFS_THUMBBEVEL) )
				{
					status( 3, cfg, "Drawing bevel" );
					gfx_draw( thumb, 0, 0, ICONF(cfg,PREFS_THUMBWIDTH)-2, 0, &col[1] );
					gfx_draw( thumb, 0, 1, 0, ICONF(cfg,PREFS_THUMBHEIGHT)-2, &col[1] );
					gfx_draw( thumb, ICONF(cfg,PREFS_THUMBWIDTH)-1, 1, ICONF(cfg,PREFS_THUMBWIDTH)-1, ICONF(cfg,PREFS_THUMBHEIGHT)-1, &col[2] );
					gfx_draw( thumb, 0, ICONF(cfg,PREFS_THUMBHEIGHT)-1, ICONF(cfg,PREFS_THUMBWIDTH)-2, ICONF(cfg,PREFS_THUMBHEIGHT)-1, &col[2] );

					/* Draw inner bevel */
					if( BCONF(cfg,PREFS_INNERBEVEL) )
					{
						w-=8;
						h-=8;
						x1=((ICONF(cfg,PREFS_THUMBWIDTH)-w )/2)-1;
						y1=((ICONF(cfg,PREFS_THUMBHEIGHT)-h )/2)-1;
						x2=x1+(w+1);
						y2=y1+(h+1);
						gfx_draw( thumb, x1, y1, x2-1, y1, &col[2] );
						gfx_draw( thumb, x1, y1+1, x1, y2-1, &col[2] );
						gfx_draw( thumb, x2, y1+1, x2, y2, &col[1] );
						gfx_draw( thumb, x1+1,y2, x2-1, y2, &col[1] );
					}
					else
					{
						w-=4;
						h-=4;
					}
				}
				/* Replace the alpha */
				if( backgrounds && backgrounds->thumbalpha )
				{
					gfx_fixalpha( backgrounds->thumbalpha, im );
				}
				/* Scale and place the image in the thumbnail */
				status( 2, cfg, "Scaling image" );
				if( BCONF(cfg,PREFS_PAD) )
				{
					gfx_scaleimage( im, 0, 0, im->im_width, im->im_height, thumb, (ICONF(cfg,PREFS_THUMBWIDTH)-w)/2, (ICONF(cfg,PREFS_THUMBHEIGHT)-h)/2, w, h, ICONF(cfg,PREFS_SCALE), TRUE );
				}
				else
				{
					gfx_scaleimage( im, 0,0, im->im_width, im->im_height, thumb, 0, 0, thumb->im_width, thumb->im_height, ICONF(cfg,PREFS_SCALE), TRUE );
				}

				picture->p_path=setstr( picture->p_path, path );
				status( 2, cfg, "Saving image" );
				gfx_save( thumbfile, thumb,
#ifdef HAVE_LIBJPEG
					GFXIO_JPEG_QUALITY, ICONF(cfg,PREFS_QUALITY),
#endif
					TAG_DONE );

				gfx_freeimage( thumb, FALSE );
			}
			gfx_freeimage( im, FALSE );
		}
	}
	else if( picture )
	{
		status( 3, cfg, "Collecting info about thumbnail" );
		if( gfx_getinfo( thumbfile, &info, TAG_DONE )==ERR_OK )
		{
			picture->p_path=setstr( picture->p_path, path );
			picture->p_width=info.im_width;
			picture->p_height=info.im_height;
		}
	}

	free( thumbfile );
	return( 0 );
}

int traverse( char *dir, int level, ConfArg *config, struct Picture *dirthumb )
{
	ConfArg *cfg=config;
	DIR *dirhandle;
	struct dirent *de;
	struct stat buf;
	char *thumbdir=NULL;
	char *path, *file=NULL, *outpath=NULL, *outfile=NULL, *str=NULL;
	const char pwd[]="./";
	struct image image={NULL, 0, 0};
	List thumbdata, *album;
	struct PictureNode *pn, *apn;
	char *extention;
	struct Picture subdirthumb;
	int useful=0;
	int duseful;
	struct Backgrounds backgrounds;
	/* The directories we wont dive in to */
	char *disallowed[]=
	{
		".",
		"..",
		".xvpics",
		".thumbnails",
		"thumbnails",
		SCONF(cfg,PREFS_THUMBDIR), /* This array should be created after the config has been read in */
		"Thumbs.db",
		NULL
	};

	struct color col[3]=
	{
		{ 0xff, 0xff, 0xff, 0xff }, /* Background */ 
		{ 0xff, 0xff, 0xff, 0xff }, /* Light bevel */
		{ 0xff, 0xff, 0xff, 0xff }  /* Dark bevel */
	};
	cfg=confargs_copy( config );

	/* Read config etc etc*/
	/* ... **/
	memset( &backgrounds, '\0', sizeof( struct Backgrounds ) );
	if( STR_ISSET(SCONF(cfg,PREFS_THUMBBACKGROUND)) )
	{
		backgrounds.thumbbackground=gfx_load( SCONF(cfg,PREFS_THUMBBACKGROUND), NULL, TAG_DONE );
	}
	if( STR_ISSET(SCONF(cfg,PREFS_THUMBALPHA)) )
	{
		backgrounds.thumbalpha=gfx_load( SCONF(cfg,PREFS_THUMBALPHA), NULL, TAG_DONE );
	}
	memset( &thumbdata, '\0', sizeof( List ) );
	thumbdata.freenode=(free_picturenode);
	thumbdata.compare=(compare_picturenode);

	memset( &subdirthumb, '\0', sizeof( struct Picture ) );
	
	/* path is used like this:
	 * duplicate dir ([pictures/family\0............])
	 * add a trailing slash ([pictures/family/\0...........])
	 * set a pointer to the last char ([pictures/family/P...........])
	 * then just copy each filename to that pointer and we get the full path
	 */ 
	if( !( path=gfx_new( char, strlen( dir )+1024 ) ) )
	{
		confargs_free( cfg );
		return 0;
	}
	strcpy( path, dir );
	tackon( path, " " );
	file=path+strlen( path )-1;

	if( SCONF(cfg,PREFS_OUTDIR) )
	{
		if( !file_exist( SCONF(cfg,PREFS_OUTDIR) ) )
		{
#ifdef __WIN32__
			if( mkdir( SCONF(cfg,PREFS_OUTDIR) ) )
#else
			if( mkdir( SCONF(cfg,PREFS_OUTDIR), 0755 ) )
#endif
			{
				free( path );
				return 0;
			}
		}

		if( !( outpath=gfx_new( char, strlen( SCONF(cfg,PREFS_OUTDIR) )+1024 ) ) )
		{
			free( path );
			confargs_free( cfg );
			return 0;
		}
		strcpy( outpath, SCONF(cfg,PREFS_OUTDIR) );
		tackon( outpath, " " );
		outfile=outpath+strlen( outpath )-1;
		outfile[0]='\0';
		free( SCONF(cfg,PREFS_OUTDIR) );
		SCONF(cfg,PREFS_OUTDIR)=outpath;
	}
	else outpath=dir;
	
	if( ( dirhandle=opendir( strlen( dir )?dir:pwd ) ) )
	{
		if( ( thumbdir=gfx_new( char, strlen( outpath )+strlen( SCONF(cfg,PREFS_THUMBDIR) )+2 ) ) )
		{
			strcpy( file, "album.xml" );
			album=readAlbum( path, cfg );
			file[0]='\0';
			strcpy( thumbdir, outpath );
			tackon( thumbdir, SCONF(cfg,PREFS_THUMBDIR) );
			if( !BCONF(cfg,PREFS_FLAT ) )
			{
				if( !file_exist( thumbdir ) )
				{
#ifdef __WIN32__
					if( mkdir( thumbdir ) )
#else
					if( mkdir( thumbdir, 0755 ) )
#endif
					{
						free( thumbdir );
						closedir( dirhandle );
						free( path );
						return 0;
					}
				}
			}


			if( BCONF(cfg,PREFS_THUMBBEVEL) )
			{
				gfx_parsecolor( &col[0], SCONF(cfg,PREFS_BEVELBG) );
				gfx_parsecolor( &col[1], SCONF(cfg,PREFS_BEVELBRIGHT) );
				gfx_parsecolor( &col[2], SCONF(cfg,PREFS_BEVELDARK) );
			}
			else
			{
				gfx_parsecolor( &col[0], SCONF(cfg,PREFS_THUMBBGCOLOR) );
			}
			while( ( de=readdir( dirhandle ) ) )
			{
				strncpy( file, de->d_name, 1023 );
				if( stat( path, &buf )==0 )
				{
					if( S_ISDIR(buf.st_mode) && BCONF(cfg,PREFS_RECURSIVE) )
					{
						if( !match( disallowed, de->d_name ) )
						{
							if( SCONF(cfg,PREFS_OUTDIR) )
							{
								strncpy( outfile, de->d_name, 1023 );
							}
							status( 2, cfg, "Entering directory '%s'", path );
							duseful=traverse( path, level+1, cfg, &subdirthumb );
							useful+=duseful;
							status( 2, cfg, "Leaving directory %s (%d)", path, duseful );
							if( duseful )
							{

								if( ( pn=gfx_new0( struct PictureNode, 1 ) ) )
								{
									pn->pn_dir=setstr( pn->pn_dir, de->d_name );
									
									if( subdirthumb.p_path )
									{
										pn->pn_thumbnail.p_path=subdirthumb.p_path;
										subdirthumb.p_path=NULL;
										pn->pn_thumbnail.p_width=subdirthumb.p_width;
										pn->pn_thumbnail.p_height=subdirthumb.p_height;
									}
									/* It's much smarter to prepend than append. The dirs should be at the top anyway */
									list_prepend( &thumbdata, (Node*)pn );
									pn=NULL;
								}

							}
							if( subdirthumb.p_path )
							{
								free( subdirthumb.p_path );
								memset( &subdirthumb, '\0', sizeof( struct Picture ) );
							}
							if( SCONF(cfg,PREFS_OUTDIR ) ) outfile[0]='\0';
						}
					}
					else if( S_ISREG(buf.st_mode ) ) 
					{
						if( ( str=strrchr( path, '.' ) ) )
						{
							str-=3;
							if( str>path )
							{
								if( !strncmp( str, "_tn", 3 ) )
								{
									status( 2, cfg, "Skipping '%s'", path );
									continue;
								}
							}
						}

						apn=get_picturenode( album, path );
						if( apn && apn->pn_skip )
						{
							status( 2, cfg, "Skipping '%s'", path );
						}
						else
						{
							status( 3, cfg, "Examining '%s'", path );
							if( gfx_getinfo( path, &image, TAG_DONE )==ERR_OK )
							{
								useful++;
								if( ( pn=gfx_new0( struct PictureNode, 1 ) ) )
								{
									if( apn )
									{
										pn->pn_title=apn->pn_title;
										apn->pn_title=NULL;
										pn->pn_caption=apn->pn_caption;
										apn->pn_caption=NULL;
										pn->pn_rotate=apn->pn_rotate;
									}
#if HAVE_LIBEXIF
									status( 3, cfg, "Getting EXIF data if any" );
									if( ( pn->pn_exifinfo=gfx_exif_file( path ) ) )
									{
										/* If we haven't yet set a rotation and there is one in EXIF then use it.
										   Make sure your EXIF orientation is correct if you can set it. Otherwise set
										   rotation in album.xml to 360. That should do the trick. */
										if( !pn->pn_rotate && pn->pn_exifinfo->ei_rotate )
										{
											pn->pn_rotate=pn->pn_exifinfo->ei_rotate;
											status( 3, cfg, "Using rotation information from EXIF (%d degrees)", pn->pn_rotate );
										}
									}
#endif
									pn->pn_original.p_width=image.im_width;
									pn->pn_original.p_height=image.im_height;
									if( !BCONF(cfg,PREFS_THUMBS) || !makethumbnail( path, outpath, cfg, col, &pn->pn_thumbnail, pn, &backgrounds ) )
									{
										/* This is used when --dir is used to strip of the path. Seems to work ok. */
										int striplen=0;
										if( STR_ISSET(SCONF(cfg,PREFS_DIR)) )
										{
											striplen=strlen( SCONF(cfg,PREFS_DIR) );
											if( (SCONF(cfg,PREFS_DIR))[striplen-1]==PATH_DELIMITER ) striplen--;
											striplen++;
										}
										pn->pn_original.p_path=strdup( strip_path( path+striplen, level ) );
//										file=(char *)basename( path );
										if( !pn->pn_title )
										{
											pn->pn_title=strdup( file );
											if( !BCONF(cfg,PREFS_EXTENSIONS ) )
											{
												if( ( extention=strrchr( pn->pn_title, '.' ) ) ) extention[0]='\0';
											}
										}
										list_append( &thumbdata, (Node *)pn );
										if( dirthumb && !dirthumb->p_path )
										{
											dirthumb->p_path=setstr(dirthumb->p_path, pn->pn_thumbnail.p_path );
											dirthumb->p_width=pn->pn_thumbnail.p_width;
											dirthumb->p_height=pn->pn_thumbnail.p_height;
										}
									}
									else
									{
										free_picturenode( (Node *)pn );
										free( pn );
									}
									if( SCONF(cfg,PREFS_OUTDIR ) )
									{
										if( CONF(cfg,PREFS_WIDTHS) )
										{
											scaleoriginal( path, outpath, cfg, pn );
										}
										if( BCONF(cfg,PREFS_COPY) )
										{
											strcpy( outfile, file );
											if( !file_exist( outpath ) )
											{
												outfile[0]='\0';
												copyfile( path, SCONF(cfg,PREFS_OUTDIR ) );
											}
											else outfile[0]='\0';
										}
									}
									else
									{
										if( CONF(cfg,PREFS_WIDTHS) )
										{
											scaleoriginal( path, outpath, cfg, pn );
										}
									}
								}
							}
						}
					}
				}
				else
				{
					status( 3, cfg, "Couldn't stat" );
				}
			}
			list_sort( &thumbdata );
//			print_thumbdata( &thumbdata );

			if( BCONF(cfg,PREFS_INDEXES) ) gfxindex( cfg, outpath, &thumbdata, level );
			if( !BCONF(cfg,PREFS_FLAT ) )
			{
				if( STR_ISSET(SCONF(cfg,PREFS_OUTDIR)) )
				{
					strcpy( outfile, SCONF(cfg,PREFS_THUMBDIR) );
					tackon( outfile, "gfxindex.xml" );
					status( 2, cfg, "Writing cache to '%s'", outpath );
					writeThumbData( cfg, &thumbdata, outpath );
					outfile[0]='\0';
				}
				else
				{
					strcpy( file, SCONF(cfg,PREFS_THUMBDIR) );
					tackon( file, "gfxindex.xml" );
					status( 2, cfg, "Writing cache to '%s'", path );
					writeThumbData( cfg, &thumbdata, path );
					file[0]='\0';
				}
			}
			if( BCONF(cfg,PREFS_WRITEALBUM) )
			{
				file[0]='\0';
				writeAlbum( cfg, &thumbdata, path );
			}
			free( thumbdir );
			if( album ) list_free( album, TRUE );
			album=NULL;
		}
		closedir( dirhandle );
	}
	if( backgrounds.thumbbackground ) gfx_freeimage( backgrounds.thumbbackground, FALSE );
	if( backgrounds.thumbalpha ) gfx_freeimage( backgrounds.thumbalpha, FALSE );
	list_free( &thumbdata, FALSE );
	free( path );
	confargs_free( cfg );
	return useful;
}

/* Strip of yay many path segments from the left and return a pointer to the rest */
char *strip_path( char *path, int number )
{
	while( number-- )
	{
		if( ( path=strchr( path, PATH_DELIMITER ) ) )
		{
			path++;
		}
		else break;
	}
	return path;
}

void gfxindex( ConfArg *local, char *dir, List *thumbs, int level )
{
	int numpics=list_length( thumbs );
	int ppp=ICONF(local,PREFS_NUMX) * ICONF(local,PREFS_NUMY);
	int xcount=0, ycount=0, page=0, count;
	char path[1024];
	char index[1024];
	char thumbindex[1024];
	int numpages=numpics/ppp;
	int numscaled=0, bpict;
	int *size, numdefault=0;
	FILE *file=NULL, *thumbfile=NULL;
	struct PictureNode *pn=NULL;
	struct Picture *pict=NULL;
	char *tmpstr, *navstr;
	char tmpbuf[4096];
	Node *node;
	char space[32]="";
	char **css=NULL, **indexheader=NULL, **indexfooter=NULL, **pictureheader=NULL, **picturefooter=NULL;
	unsigned int css_numrows=0, indexheader_numrows=0, indexfooter_numrows=0, pictureheader_numrows=0, picturefooter_numrows=0;
	int hspace, vspace;
	if( (float)numpics/(float)ppp > numpages ) numpages++;

	if( !thumbs->head )
	{
		status( 3, local, "No thumbs to make index of dir %s", dir );
	}

	if( STR_ISSET( SCONF(local,PREFS_CSSFILE) ) )
	{
		if( ( css=readfile( SCONF(local,PREFS_CSSFILE), &css_numrows ) ) )
		{
			for( count=0; count<css_numrows; count++ )
			{
				stripws( css[count] );
			}
		}
	}

	if( STR_ISSET( SCONF(local,PREFS_INDEXHEADERFILE) ) )
	{
		if( ( indexheader=readfile( SCONF(local,PREFS_INDEXHEADERFILE), &indexheader_numrows ) ) )
		{
			for( count=0; count<indexheader_numrows; count++ )
			{
				stripws( indexheader[count] );
			}
		}
	}

	if( STR_ISSET( SCONF(local,PREFS_INDEXFOOTERFILE) ) )
	{
		if( ( indexfooter=readfile( SCONF(local,PREFS_INDEXFOOTERFILE), &indexfooter_numrows ) ) )
		{
			for( count=0; count<indexfooter_numrows; count++ )
			{
				stripws( indexfooter[count] );
			}
		}
	}

	if( STR_ISSET( SCONF(local,PREFS_PICTUREHEADERFILE) ) )
	{
		if( ( pictureheader=readfile( SCONF(local,PREFS_PICTUREHEADERFILE), &pictureheader_numrows ) ) )
		{
			for( count=0; count<pictureheader_numrows; count++ )
			{
				stripws( pictureheader[count] );
			}
		}
	}

	if( STR_ISSET( SCONF(local,PREFS_PICTUREFOOTERFILE) ) )
	{
		if( ( picturefooter=readfile( SCONF(local,PREFS_PICTUREFOOTERFILE), &picturefooter_numrows ) ) )
		{
			for( count=0; count<picturefooter_numrows; count++ )
			{
				stripws( picturefooter[count] );
			}
		}
	}

	size=(int *)CONF(local,PREFS_WIDTHS );
	if( size )
	{
		numscaled=arrlen( size );
		if( ICONF(local,PREFS_DEFWIDTH) )
		{
			for( bpict=0; bpict<numscaled; bpict++ )
			{
				if( size[bpict]==ICONF(local,PREFS_DEFWIDTH) )
				{
					numdefault=bpict;
					break;
				}
			}
		}
	}
	for( node=thumbs->head ; node ; node=node->next )
	{
		pn=(struct PictureNode *)node;
		for( bpict=-1; bpict<numscaled; bpict++ )
		{
			if( bpict==-1 )
			{
				if( !CONF(local,PREFS_OUTDIR) || BCONF(local,PREFS_COPY) ) pict=&(pn->pn_original);
				else pict=NULL;
			}
			else if( pn->pn_pictures )
			{
				pict=pn->pn_pictures[bpict];
			}
			else
			{
				pict=NULL;
			}
			if( pict && pict->p_path )
			{
				if( !BCONF(local,PREFS_FLAT ) )
				{
					strcpy( path, dir ); /* Start with the output dir */
					tackon( path, SCONF(local,PREFS_THUMBDIR) ); /* Add the thumbnail dir */
					tackon( path, (char *)basename( pict->p_path ) ); /* Add the name of the picture */
					strcat( path, ".html" ); /* And stick a .html after it */
					status( 3, local, "[1] Opening '%s'", path );
					if( !( thumbfile=fopen( path, "w" ) ) ) goto error;
					sprintf( path, "..%c%s", PATH_DELIMITER, indexstr( page ) );
					if( BCONF(local,PREFS_FULLDOC) )
					{
						if( STR_ISSET(SCONF(local,PREFS_PICTURETITLE)) )
							tprintf( tmpbuf, SCONF(local,PREFS_PICTURETITLE), pn, pict, local, page+1, numpages );
						else
							sprintf( tmpbuf, "%s%s%s ( %d x %d )", STR_ISSET(SCONF(local,PREFS_TITLE))?SCONF(local,PREFS_TITLE):"", STR_ISSET(SCONF(local,PREFS_TITLE))?" - ":"", (pn->pn_title?pn->pn_title:pn->pn_original.p_path), pict->p_width, pict->p_height );
						myfprintf( thumbfile, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n<HTML>\n <HEAD>\n  <TITLE>%s</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXindex v" VERSION " by Fredrik Rambris (http://fredrik.rambris.com)\">\n", tmpbuf );
						if( STR_ISSET(SCONF(local,PREFS_CSS)) )
						{
							myfprintf( thumbfile, "  <LINK REL=\"stylesheet\" HREF=\"" );
							for( count=0 ; count<=level ; count++ ) fprintf( thumbfile, "..%c", PATH_DELIMITER );
							myfprintf( thumbfile, "%s\" TYPE=\"text/css\">\n", SCONF(local,PREFS_CSS) );
						}
						if( css )
						{
							myfprintf( thumbfile, "  <STYLE><!--\n" );
							for( count=0; count<css_numrows; count++ )
							{
								myfprintf( thumbfile, "   %s\n", css[count] );
							}
							myfprintf( thumbfile, "  --></STYLE>\n" );
						}
						myfprintf( thumbfile, " </HEAD>\n <BODY%s%s>\n", STR_ISSET(SCONF(local,PREFS_BODYARGS))?" ":"", STR_ISSET(SCONF(local,PREFS_BODYARGS))?SCONF(local,PREFS_BODYARGS):"" );
	
						if( pictureheader || STR_ISSET(SCONF(local,PREFS_PICTUREHEADER)) ) myfprintf( thumbfile, "  <DIV CLASS=\"header\">\n" );
					}
				
					if( pictureheader )
					{
						for( count=0; count<pictureheader_numrows; count++ )
						{
							tprintf( tmpbuf, pictureheader[count], pn, pict, local, page+1, numpages );
							myfprintf( thumbfile, "   %s\n", tmpbuf );
						}
					}
					if( STR_ISSET(SCONF(local,PREFS_PICTUREHEADER)) )
					{
						tprintf( tmpbuf, SCONF(local,PREFS_PICTUREHEADER), pn, pict, local, page+1, numpages );
						myfprintf( thumbfile, "   %s\n", tmpbuf );
					}
					if( BCONF(local,PREFS_FULLDOC) )
					{
						if( pictureheader || STR_ISSET(SCONF(local,PREFS_PICTUREHEADER)) ) myfprintf( thumbfile, "  </DIV>\n" );
					}
					myfprintf( thumbfile, "  <DIV CLASS=\"picture\" ALIGN=\"center\">\n" );

					if( numpics>1 ) navbar_new( thumbindex );
					if( node->prev && !(((struct PictureNode *)(node->prev))->pn_dir) && ( STR_ISSET(SCONF(local,PREFS_PREV)) || ICONF(local,PREFS_NAVTHUMBS) || BCONF(local,PREFS_USETITLES) ) )
					{
						tmpstr=(bpict==-1?(char *)basename( ((struct PictureNode *)(node->prev))->pn_original.p_path ):(char *)basename( ((struct PictureNode *)(node->prev))->pn_pictures[bpict]->p_path ));
		
						if( ICONF(local,PREFS_NAVTHUMBS) )
						{
							navstr=BCONF(local,PREFS_USETITLES)?((struct PictureNode *)(node->prev))->pn_title:(STR_ISSET(SCONF(local,PREFS_PREV))?SCONF(local,PREFS_PREV):"");
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" ALT=\"%s\" TITLE=\"%s\" BORDER=\"0\"></A>", tmpstr, basename(((struct PictureNode *)(node->prev))->pn_thumbnail.p_path), (long)(((struct PictureNode *)(node->prev))->pn_thumbnail.p_width*ICONF(local,PREFS_NAVTHUMBS)/100), (long)(((struct PictureNode *)(node->prev))->pn_thumbnail.p_height*ICONF(local,PREFS_NAVTHUMBS)/100), navstr, navstr );
						}
						else if( BCONF(local,PREFS_USETITLES ) )
						{
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, ((struct PictureNode *)(node->prev))->pn_title );
						}
						else
						{
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, SCONF(local,PREFS_PREV) );
						}
					}
					if( SCONF(local,PREFS_INDEX) ) navbar_add( local, thumbindex, "<A HREF=\"%s\">%s</A>", path, SCONF(local,PREFS_INDEX) );
	
					if( node->next && !(((struct PictureNode *)(node->next))->pn_dir) && ( STR_ISSET(SCONF(local,PREFS_NEXT)) || ICONF(local,PREFS_NAVTHUMBS) || BCONF(local,PREFS_USETITLES ) ) )
					{
						tmpstr=(bpict==-1?(char *)basename( ((struct PictureNode *)(node->next))->pn_original.p_path ):(char *)basename( ((struct PictureNode *)(node->next))->pn_pictures[bpict]->p_path ));	
						navstr=BCONF(local,PREFS_USETITLES)?((struct PictureNode *)(node->next))->pn_title:(STR_ISSET(SCONF(local,PREFS_NEXT))?SCONF(local,PREFS_NEXT):"");
						if( ICONF(local,PREFS_NAVTHUMBS) )
						{
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" ALT=\"%s\" TITLE=\"%s\" BORDER=\"0\"></A>", tmpstr, basename(((struct PictureNode *)(node->next))->pn_thumbnail.p_path), (long)(((struct PictureNode *)(node->next))->pn_thumbnail.p_width*ICONF(local,PREFS_NAVTHUMBS)/100), (long)(((struct PictureNode *)(node->next))->pn_thumbnail.p_height*ICONF(local,PREFS_NAVTHUMBS)/100), navstr, navstr );
						}
						else if( BCONF(local,PREFS_USETITLES ) )
						{
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, ((struct PictureNode *)(node->next))->pn_title );
						}
						else
						{
							navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, SCONF(local,PREFS_NEXT) );
						}
					}


					if( numpics>1 ) navbar_end( local, thumbindex );
					if( numpics>1 ) myfprintf( thumbfile, "   <SPAN CLASS=\"navbar\">%s</SPAN><BR>\n", thumbindex );
//					myfprintf( thumbfile, "   <A HREF=\"%s\"><IMG SRC=\"../%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\" VSPACE=\"2\" ALT=\"%s\"></A><BR>\n", path, (STR_ISSET(SCONF(local,PREFS_OUTDIR))&&bpict>=0?pict->p_path+strlen(SCONF(local,PREFS_OUTDIR)):pict->p_path), pict->p_width, pict->p_height, pn->pn_title );
					myfprintf( thumbfile, "   <A HREF=\"%s\"><IMG SRC=\"../%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\" VSPACE=\"2\" ALT=\"%s\"></A><BR>\n", path, pict->p_path, pict->p_width, pict->p_height, pn->pn_title );
					if( BCONF(local,PREFS_CAPTIONS) && STR_ISSET(pn->pn_caption) ) myfprintf( thumbfile, "   <BR><SPAN CLASS=\"caption\">%s</SPAN><BR>\n", pn->pn_caption );
#if HAVE_LIBEXIF
					if( pn->pn_exifinfo && BCONF(local,PREFS_EXIF) )
					{
						ExifInfo *ei=pn->pn_exifinfo;
						myfprintf( thumbfile, "   <TABLE CLASS=\"exif\">\n" );
						if( STR_ISSET( ei->ei_date ) ) myfprintf( thumbfile, "    <TR><TH>Taken</TH><TD>%s</TD></TR>\n", ei->ei_date );
						if( STR_ISSET( ei->ei_make ) ) myfprintf( thumbfile, "    <TR><TH>Make</TH><TD>%s</TD></TR>\n", ei->ei_make );
						if( STR_ISSET( ei->ei_model ) ) myfprintf( thumbfile, "    <TR><TH>Model</TH><TD>%s</TD></TR>\n", ei->ei_model );
						if( STR_ISSET( ei->ei_exposure ) ) myfprintf( thumbfile, "    <TR><TH>Exposure time</TH><TD>%s</TD></TR>\n", ei->ei_exposure );
						if( STR_ISSET( ei->ei_aperture ) ) myfprintf( thumbfile, "    <TR><TH>Aperture</TH><TD>%s</TD></TR>\n", ei->ei_aperture );
						if( STR_ISSET( ei->ei_focal ) ) myfprintf( thumbfile, "    <TR><TH>Focal length</TH><TD>%s</TD></TR>\n", ei->ei_focal );
						myfprintf( thumbfile, "    <TR><TH>Flash</TH><TD>%s</TD></TR>\n", ei->ei_flash?"Yes":"No" );
						myfprintf( thumbfile, "   </TABLE>\n" );
					}
#endif
					if( numpics>1 ) myfprintf( thumbfile, "   <SPAN CLASS=\"navbar\">%s</SPAN>\n", thumbindex );
				
					myfprintf( thumbfile, "  </DIV>\n\n" );
					if( BCONF(local,PREFS_FULLDOC) )
					{
						if( picturefooter || STR_ISSET(SCONF(local,PREFS_PICTUREFOOTER)) ) myfprintf( thumbfile, "  <DIV CLASS=\"footer\">\n" );
					}
					if( picturefooter )
					{
						for( count=0; count<picturefooter_numrows; count++ )
						{
							tprintf( tmpbuf, picturefooter[count], pn, pict, local, page+1, numpages );
							myfprintf( thumbfile, "   %s\n", tmpbuf );
						}
					}
					if( STR_ISSET(SCONF(local,PREFS_PICTUREFOOTER)) )
					{
						tprintf( tmpbuf, SCONF(local,PREFS_PICTUREFOOTER), pn, pict, local, page+1, numpages );
						myfprintf( thumbfile, "   %s\n", tmpbuf );
					}
					if( BCONF(local,PREFS_FULLDOC) )
					{
						if( picturefooter || STR_ISSET(SCONF(local,PREFS_PICTUREFOOTER)) ) myfprintf( thumbfile, "  </DIV>\n" );
						myfprintf( thumbfile, " </BODY>\n</HTML>\n" );
					}
					fclose( thumbfile );
					thumbfile=NULL;
				}
			}
		}
		if( ycount+xcount==0 )
		{
			ycount=0;
			xcount=0;
			strcpy( path, dir );
			tackon( path, indexstr( page ) );
			status( 3, local,  "[2] Opening '%s'", path );
			if( !( file=fopen( path, "w" ) ) ) goto error;
			if( BCONF(local,PREFS_FULLDOC) )
			{
				if( STR_ISSET(SCONF(local,PREFS_INDEXTITLE)) )
					tprintf( tmpbuf, SCONF(local,PREFS_INDEXTITLE), pn, pict, local, page+1, numpages );
				else
					sprintf( tmpbuf, "%s%sPage %d / %d", STR_ISSET(SCONF(local,PREFS_TITLE))?SCONF(local,PREFS_TITLE):"", STR_ISSET(SCONF(local,PREFS_TITLE))?" - ":"", page+1, numpages );
				myfprintf( file, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n<HTML>\n <HEAD>\n  <TITLE>%s</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXindex v" VERSION " by Fredrik Rambris (http://fredrik.rambris.com)\">\n", tmpbuf );
				if( STR_ISSET(SCONF(local,PREFS_CSS)) )
				{
	
					myfprintf( file, "  <LINK REL=\"stylesheet\" HREF=\"" );
					for( count=0 ; count<level ; count ++ ) myfprintf( file, "..%c", PATH_DELIMITER );
					myfprintf( file, "%s\" TYPE=\"text/css\">\n", SCONF(local,PREFS_CSS) );
				}
				if( css )
				{
					myfprintf( file, "  <STYLE><!--\n" );
					for( count=0; count<css_numrows; count++ )
					{
						myfprintf( file, "   %s\n", css[count] );
					}
					myfprintf( file, "  --></STYLE>\n" );
				}
				myfprintf( file, " </HEAD>\n <BODY%s%s>\n", STR_ISSET(SCONF(local,PREFS_BODYARGS))?" ":"", STR_ISSET(SCONF(local,PREFS_BODYARGS))?SCONF(local,PREFS_BODYARGS):"" );
				if( indexheader || STR_ISSET(SCONF(local,PREFS_INDEXHEADER)) ) myfprintf( file, "  <DIV CLASS=\"header\">\n" );
			}
			if( indexheader )
			{
				for( count=0; count<indexheader_numrows; count++ )
				{
					tprintf( tmpbuf, indexheader[count], pn, pict, local, page+1, numpages );
					myfprintf( file, "   %s\n", tmpbuf );
				}
			}
			if( STR_ISSET(SCONF(local,PREFS_INDEXHEADER)) )
			{
				tprintf( tmpbuf, SCONF(local,PREFS_INDEXHEADER), pn, pict, local, page+1, numpages );
				myfprintf( file, "   %s\n", tmpbuf );
			}
			if( BCONF(local,PREFS_FULLDOC) )
			{
				if( indexheader || STR_ISSET(SCONF(local,PREFS_INDEXHEADER)) ) myfprintf( file, "  </DIV>\n" );
			}
			myfprintf( file, "  <DIV CLASS=\"thumbnails\" ALIGN=\"center\">\n" );
			index[0]='\0';
			if( numpages>1 || (STR_ISSET(SCONF(local,PREFS_PARENT)) && STR_ISSET(SCONF(local,PREFS_PARENTDOC))) ) navbar_new( index );
			if( numpages>1 )
			{
				if( page>0 && STR_ISSET(SCONF(local,PREFS_PREV)) ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page-1 ), SCONF(local,PREFS_PREV) );
			}
			if( STR_ISSET(SCONF(local,PREFS_PARENT)) && STR_ISSET(SCONF(local,PREFS_PARENTDOC)) ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", SCONF(local,PREFS_PARENTDOC), SCONF(local,PREFS_PARENT) );
			if( numpages>1 )
			{	
				if( BCONF(local,PREFS_NUMLINK) )
				{
					for( count=0 ; count<numpages ; count++ )
					{
						if( count!=page ) navbar_add( local, index, "<A HREF=\"%s\">", indexstr( count ) );
						else navbar_add( local, index, "<SPAN CLASS=\"current\">" );
						sprintf( index+strlen( index ), "%d", count+1 );
						strcat( index, "</A>" );
						if( count!=page ) strcat( index, "</A>" );
						else strcat( index, "</SPAN>" );
					}
				}
				if( page<(numpages-1) && STR_ISSET(SCONF(local,PREFS_NEXT)) ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page+1 ), SCONF(local,PREFS_NEXT) );
			}
			if( strlen( index ) )
			{
				navbar_end( local, index );
				myfprintf( file, "   <SPAN CLASS=\"navbar\">%s</SPAN><BR>\n", index );
			}
											
			myfprintf( file, "   <TABLE%s%s>\n", (STR_ISSET(SCONF(local,PREFS_TABLEARGS))?" ":""), (STR_ISSET(SCONF(local,PREFS_TABLEARGS))?SCONF(local,PREFS_TABLEARGS):"") );
		}
		if( xcount==0 ) myfprintf( file, "     <TR>\n" );
		space[0]='\0';
		if( pn->pn_dir )
		{
			myfprintf( file, "<TD CLASS=\"subdir\"%s%s><A HREF=\"%s/index.html\">", STR_ISSET(SCONF(local,PREFS_CELLARGS))?" ":"", STR_ISSET(SCONF(local,PREFS_CELLARGS))?SCONF(local,PREFS_CELLARGS):"", pn->pn_dir );
			if( pn->pn_thumbnail.p_path && !BCONF(local,PREFS_PAD) && BCONF(local,PREFS_SOFTPAD) )
			{
				hspace=0;
				vspace=0;
				if( pn->pn_thumbnail.p_width<ICONF(local,PREFS_THUMBWIDTH) )
				{
					hspace=(ICONF(local,PREFS_THUMBWIDTH)-pn->pn_thumbnail.p_width)/2;
				}
				if( pn->pn_thumbnail.p_height<ICONF(local,PREFS_THUMBHEIGHT) )
				{
					vspace=(ICONF(local,PREFS_THUMBHEIGHT)-pn->pn_thumbnail.p_height)/2;
				}
				if( hspace ) sprintf( space, " HSPACE=\"%d\"", hspace );
				if( vspace ) sprintf( space+strlen( space ), " VSPACE=\"%d\"", vspace );
				myfprintf( file, "<IMG SRC=\"%s/%s\" ALT=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\"%s>", pn->pn_dir, pn->pn_thumbnail.p_path, pn->pn_dir, pn->pn_thumbnail.p_width, pn->pn_thumbnail.p_height, space );
				if( BCONF(local,PREFS_TITLES) ) myfprintf( file, "<BR>%s (dir)", pn->pn_dir );
			}
			else myfprintf( file, "%s", pn->pn_dir );
			myfprintf( file, "</A></TD>" );
		}
		else
		{
			if( !BCONF(local,PREFS_PAD) && BCONF(local,PREFS_SOFTPAD) )
			{
				hspace=0;
				vspace=0;
				if( pn->pn_thumbnail.p_width<ICONF(local,PREFS_THUMBWIDTH) )
				{
					hspace=(ICONF(local,PREFS_THUMBWIDTH)-pn->pn_thumbnail.p_width)/2;
				}
				if( pn->pn_thumbnail.p_height<ICONF(local,PREFS_THUMBHEIGHT) )
				{
					vspace=(ICONF(local,PREFS_THUMBHEIGHT)-pn->pn_thumbnail.p_height)/2;
				}
				if( hspace ) sprintf( space, " HSPACE=\"%d\"", hspace );
				if( vspace ) sprintf( space+strlen( space ), " VSPACE=\"%d\"", vspace );
			}

			myfprintf( file, "     <TD%s%s>", SCONF(local,PREFS_CELLARGS)?" ":"", SCONF(local,PREFS_CELLARGS)?SCONF(local,PREFS_CELLARGS):"" );

			if( ICONF(local,PREFS_DEFWIDTH) && pn->pn_pictures && pn->pn_pictures[numdefault]->p_path ) pict=pn->pn_pictures[numdefault];
			else if( pn->pn_pictures && pn->pn_pictures[0]->p_path ) pict=pn->pn_pictures[0];
			else if( ( BCONF(local,PREFS_COPY) || !SCONF(local,PREFS_OUTDIR) ) && pn->pn_original.p_path ) pict=&pn->pn_original;
			else pict=NULL;
			if( pict )
			{
				if( !BCONF(local,PREFS_FLAT) )
				{
					strcpy( path, SCONF(local,PREFS_THUMBDIR) );
					tackon( path, (char *)basename( pict->p_path ) );
					myfprintf( file, "<A HREF=\"%s.html\">", path );
				}
				else
				{
					myfprintf( file, "<A HREF=\"%s\">", (char *)basename( pict->p_path ) );
				}
			}
			myfprintf( file, "<IMG ALT=\"%s\" SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\"%s>%s%s", pn->pn_title, pn->pn_thumbnail.p_path, pn->pn_thumbnail.p_width, pn->pn_thumbnail.p_height, space, BCONF(local,PREFS_TITLES)?"<BR>":"", BCONF(local,PREFS_TITLES)?pn->pn_title:"" );
			if( pict ) myfprintf( file, "</A>" );
			if( (pn->pn_pictures && numscaled>1) || ( BCONF(local,PREFS_COPY) || !SCONF(local,PREFS_OUTDIR) ) )
			{
				myfprintf( file, "<BR><SPAN CLASS=\"sizes\">" );
				hspace=0;
				for( bpict=-1; bpict<numscaled; bpict++ )
				{
					if( bpict==-1 && ( BCONF(local,PREFS_COPY) || !SCONF(local,PREFS_OUTDIR) ) && numscaled )
					{
						strcpy( path, SCONF(local,PREFS_THUMBDIR) );
						tackon( path, (char *)basename( pn->pn_original.p_path ) );
						myfprintf( file, "<A HREF=\"%s.html\">%d</A>", path, MAX(pn->pn_original.p_width,pn->pn_original.p_height) );
						hspace=1;
					}
					else if( numscaled && bpict>=0 )
					{
						strcpy( path, SCONF(local,PREFS_THUMBDIR) );
/*
 gfxindex -O /var/www/html/pics/paris -W1024,800 --defwidth=800 -v2
 segfaultar vid nästa rad... VARFÖR?!
 */
						tackon( path, (char *)basename( pn->pn_pictures[bpict]->p_path ) );
						if( hspace ) myfprintf( file, " " );
						myfprintf( file, "<A HREF=\"%s.html\">%d</A>", path, size[bpict] );
						hspace=1;
					}
				}
				myfprintf( file, "</SPAN>" );
			}
			myfprintf( file, "</TD>\n" );
		}
		xcount++;
		if( xcount>=ICONF(local,PREFS_NUMX) )
		{
			xcount=0;
			ycount++;
			myfprintf( file, "    </TR>\n" );
		}
		if( ycount>=ICONF(local,PREFS_NUMY) )
		{
			ycount=0;
			page++;
			myfprintf( file, "   </TABLE>\n" );
			if( BCONF(local,PREFS_CAPTIONS) && STR_ISSET(SCONF(local,PREFS_CAPTION)) ) myfprintf( file, "   <BR><SPAN CLASS=\"caption\">%s</SPAN>\n", SCONF(local,PREFS_CAPTION) );
			if( strlen( index ) ) myfprintf( file, "    <SPAN CLASS=\"navbar\">%s</SPAN>\n", index ); 

			myfprintf( file, "  </DIV>\n" );
			if( BCONF(local,PREFS_FULLDOC) )
			{
				if( indexfooter || STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) ) fprintf( file, "  <DIV CLASS=\"footer\">\n" );
			}
			if( indexfooter )
			{
				for( count=0; count<indexfooter_numrows; count++ )
				{
					tprintf( tmpbuf, indexfooter[count], pn, pict, local, page+1, numpages );
					myfprintf( file, "   %s\n", tmpbuf );
				}
			}
			if( STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) )
			{
				tprintf( tmpbuf, SCONF(local,PREFS_INDEXFOOTER), pn, pict, local, page+1, numpages );
				myfprintf( file, "   %s\n", tmpbuf );
			}
			if( BCONF(local,PREFS_FULLDOC) )
			{
				if( indexfooter || STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) ) myfprintf( file, "  </DIV>\n" );
				myfprintf( file, " </BODY>\n</HTML>\n" );
			}
			fclose( file );
			file=NULL;
		}
	}
	if( file )
	{
		myfprintf( file, "    </TR>\n   </TABLE>\n" );
		if( BCONF(local,PREFS_CAPTIONS) && STR_ISSET(SCONF(local,PREFS_CAPTION)) ) myfprintf( file, "   <BR><SPAN CLASS=\"caption\">%s</SPAN>\n", SCONF(local,PREFS_CAPTION) );
		if( strlen( index ) ) myfprintf( file, "    <SPAN CLASS=\"navbar\">%s</SPAN>\n", index ); 
		myfprintf( file, "  </DIV>\n" );
		if( BCONF(local,PREFS_FULLDOC) )
		{
			if( indexfooter || STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) ) myfprintf( file, "  <DIV CLASS=\"footer\">\n" );
		}
		if( indexfooter )
		{
			for( count=0; count<indexfooter_numrows; count++ )
			{
				tprintf( tmpbuf, indexfooter[count], pn, pict, local, page+1, numpages );
				myfprintf( file, "   %s\n", tmpbuf );
			}
		}
		if( STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) )
		{
			tprintf( tmpbuf, SCONF(local,PREFS_INDEXFOOTER), pn, pict, local, page+1, numpages );
			myfprintf( file, "   %s\n", tmpbuf );
		}
		if( BCONF(local,PREFS_FULLDOC) )
		{
			if( indexfooter || STR_ISSET(SCONF(local,PREFS_INDEXFOOTER)) ) myfprintf( file, "  </DIV>\n" );
			myfprintf( file, " </BODY>\n</HTML>\n" );
		}
		fclose( file );
		file=NULL;
	}
	error:
	if( file ) fclose( file );
	if( thumbfile ) fclose( thumbfile );
	if( css )
	{
		if( css[0] ) free( css[0] );
		free( css );
	}
	if( indexheader )
	{
		if( indexheader[0] ) free( indexheader[0] );
		free( indexheader );
	}
	if( indexfooter )
	{
		if( indexfooter[0] ) free( indexfooter[0] );
		free( indexfooter );
	}
	if( pictureheader )
	{
		if( pictureheader[0] ) free( pictureheader[0] );
		free( pictureheader );
	}
	if( picturefooter )
	{
		if( picturefooter[0] ) free( picturefooter[0] );
		free( picturefooter );
	}
}

char *indexstr( int number )
{
	static char buf[20];
	if( number>0 ) sprintf( buf, "index%d.html", number );
	else sprintf( buf, "index.html" );
	return( buf );
}

void navbar_new( char *str )
{
	str[0]='\0';
}

void navbar_add( ConfArg *local, char *str, char *newstr, ... )
{
	va_list ap;
	char buf[512];
	va_start( ap, newstr );
	vsnprintf( buf, 511, newstr, ap );
	if( str[0] ) { if( SCONF(local,PREFS_SPACE) ) strcat( str, SCONF(local,PREFS_SPACE) ); if( SCONF(local,PREFS_DIVIDER) ) strcat( str, SCONF(local,PREFS_DIVIDER) ); }
	else { if( SCONF(local,PREFS_LEFT) ) strcat( str, SCONF(local,PREFS_LEFT) ); }
	if( SCONF(local,PREFS_SPACE) ) strcat( str, SCONF(local,PREFS_SPACE) );
	strcat( str, buf );
	va_end( ap );
}

void navbar_end( ConfArg *local, char *str )
{
	if( SCONF(local,PREFS_SPACE) ) strcat( str, SCONF(local,PREFS_SPACE) );
	if( SCONF(local,PREFS_RIGHT) ) strcat( str, SCONF(local,PREFS_RIGHT) );
}

