/* gfxio.c - Functions for loading and saving image data
 *
 * GFXIndex (c) 1999-2004 Fredrik Rambris <fredrik@rambris.com>.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gfxio.h"
#include "gfx.h"
#include "io_jpeg.h"
#include "io_png.h"
#include "io_dcraw.h"
#include <string.h>

List *ios=NULL;

int gfxio_init( void )
{
	ios=list_new();
#ifdef HAVE_LIBJPEG
	list_append( ios, (Node *)jpeg_init() );
#endif
#ifdef HAVE_LIBPNG
	list_append( ios, (Node *)png_init() );
#endif
#ifdef USE_DCRAW
#ifdef HAVE_LIBJPEG
	list_append( ios, (Node *)dcraw_init() );
#endif
#endif

	return( ERR_OK );
}

void gfxio_cleanup( void )
{
	Node *node, *next;
	struct imageio *iio;
	/* Empty IOS and tell I/O-libraries to clean up */
	if( !ios ) return;
	if( ios->head )
	{
		node=(Node *)ios->head;
		while( node )
		{
			next=node->next;
			iio=(struct imageio *)node;
			if( iio->io_cleanup ) iio->io_cleanup();
			node=next;
		}
	}
	free( ios );
//	list_free( ios, TRUE );
	ios=NULL;
}

void img_clean( struct image *img )
{
	if( img->im_pixels ) free( img->im_pixels );
	img->im_pixels=NULL;
//	if( img->im_alpha ) free( img->im_alpha );
//	img->im_alpha=NULL;
}

void fe_printioinfo( Node *node )
{
	printf( "%s (%s)\n", ((struct imageio *)node)->io_info, *(((struct imageio *)node)->io_extension) );
}

void printioinfo( void )
{
	printf( "I/O modules:\n" );
	if( !ios ) printf( "No modules\n" );
	list_foreach( ios, fe_printioinfo );
}

struct imageio *identify_file( FILE *file, char *filename, int *error )
{
	Node *node;
	struct imageio *io=NULL;
	BOOL found=FALSE;
	int count;
	if( !ios || !ios->head )
	{
		if( error ) *error=ERR_UNKNOWNFORMAT;
		return( NULL );
	}
	/* Guess what format the file might be of */
	node=(Node *)ios->head;
	while( node )
	{
		io=(struct imageio *)node;

		/* If we can we try to identify it by it's contents */
		if( io->io_identify && file )
		{
			if( io->io_identify( file ) ) break;
		}
		/* if not we try to guess by extension */
		else if( io->io_extension )
		{
			if( filename )
			{
				found=FALSE;
				for( count=0; io->io_extension[count]; count++ )
				{
					if( fastcasecompare( filename+strlen( filename )-strlen( io->io_extension[count] ), io->io_extension[count] ) )
					{
						found=TRUE;
						break;
					}
				}
				if( found ) break;
			}
		}
		io=NULL;
		node=node->next;
	}
	return( io );
}

struct image *gfx_load( char *filename, int *error, Tag tags, ... )
{
	FILE *fp=NULL;
	struct image *image=NULL;
	struct imageio *io=NULL;
	int err;

	if( !ios )
	{
		if( error ) *error=ERR_UNKNOWNFORMAT;
		return( NULL );
	}

	if( !( fp=fopen( filename, "rb" ) ) )
	{
		if( error ) *error=ERR_FILE;
		return( NULL );
	}

	io=identify_file( fp, filename, error );

	/* We have a possitive ID on the file */
	if( io )
	{
		if( !io->io_load )
		{
			fclose( fp );
			if( error ) *error=ERR_LOADNOTPOSSIBLE;
			return( NULL );
		}
		if( !( image=gfx_new0( struct image, 1 ) ) )
		{
			fclose( fp );
			if( error ) *error=ERR_MEM;
			return( NULL );
		}
		err=io->io_load( fp, image, (struct TagItem *)&tags );
		if( err )
		{
			free( image );
			fclose( fp );
			if( error ) *error=err;
			return( FALSE );
		}
	}
	else
	{
		if( error ) *error=ERR_UNKNOWNFORMAT;
	}
	
	fclose( fp );
	return( image );
}

int gfx_save( char *filename, struct image *image, Tag tags, ... )
{
	FILE *fp=NULL;
	struct imageio *io=NULL;
	int err;

	if( !filename ) return( ERR_NOFILE );
	if( !image ) return( ERR_UNKNOWN );
	if( !ios ) return( ERR_SAVENOTPOSSIBLE );
	
	io=identify_file( NULL, filename, NULL );
	if( io )
	{
		if( !io->io_save ) return( ERR_SAVENOTPOSSIBLE );
		if( !( fp=fopen( filename, "wb" ) ) ) return( ERR_FILE );
		err=io->io_save( fp, image, (struct TagItem *)&tags );
		fclose( fp );
		return( err );
	}
	else
	{
		return( ERR_UNKNOWNFORMAT );
	}
	return( ERR_UNKNOWN ); /* This should never be reached */
}

int gfx_getinfo( char *filename, struct image *image, Tag tags, ... )
{
	FILE *fp=NULL;
	struct imageio *io=NULL;
	int err=ERR_OK;

	if( !ios )
	{
		return( ERR_UNKNOWNFORMAT );
	}

	if( !( fp=fopen( filename, "rb" ) ) )
	{
		return( ERR_FILE );
	}

	io=identify_file( fp, filename, &err );

	/* We have a possitive ID on the file */
	if( io )
	{
		image->im_loadmodule=io->io_moduleid;
		if( !io->io_load )
		{
			fclose( fp );
			return( ERR_LOADNOTPOSSIBLE );
		}
		if( !io->io_getinfo )
		{
			fclose( fp );
			return ERR_LOADNOTPOSSIBLE;
		}
		err=io->io_getinfo( fp, image, (struct TagItem *)&tags );
		if( err )
		{
			fclose( fp );
			return( err );
		}
	}
	else
	{
		err=ERR_UNKNOWNFORMAT;
	}
	
	fclose( fp );
	return( err );
}

