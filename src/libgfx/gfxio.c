/* gfxio.c - Functions for loading and saving image data
 *
 * GFXIndex (c) 1999-2000 Fredrik Rambris <fredrik@rambris.com>.
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
#include <string.h>

GList *ios=NULL;

gint gfxio_init( void )
{
#ifdef HAVE_LIBJPEG
	ios=g_list_append( ios, (gpointer)jpeg_init() );
#endif
	return( ERR_OK );
}

void gfxio_cleanup( void )
{
	GList *node=ios;
	struct imageio *iio;
	/* Empty IOS and tell I/O-libraries to clean up */
	while( node )
	{
		iio=(struct imageio *)node->data;
		if( iio->io_cleanup ) iio->io_cleanup();
		node->data=ios=NULL;
		node=g_list_next( node );
	}
	g_list_free( ios );
	ios=NULL;
}

void img_clean( struct image *img )
{
	if( img->im_pixels ) g_free( img->im_pixels );
	img->im_pixels=NULL;
	if( img->im_alpha ) g_free( img->im_alpha );
	img->im_alpha=NULL;
}

void fe_printioinfo( gpointer data, gpointer user_data )
{
	printf( "%s\n", ((struct imageio *)data)->io_info );
}

void printioinfo( void )
{
	printf( "I/O modules:\n" );
	if( !ios ) printf( "No modules\n" );
	g_list_foreach( ios, fe_printioinfo, NULL );
}

struct imageio *identify_file( FILE *file, gchar *filename, gint *error )
{
	GList *node=ios;
	struct imageio *io=NULL;
	if( !ios )
	{
		if( error ) *error=ERR_UNKNOWNFORMAT;
		return( NULL );
	}
	/* Guess what format the file might be of */
	while( node )
	{
		io=(struct imageio *)node->data;

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
				if( !g_strcasecmp( filename+strlen( filename )-strlen( io->io_extension ), io->io_extension ) ) break;
			}
		}
		io=NULL;
		node=g_list_next( node );
	}
	return( io );
}

struct image *gfx_load( gchar *filename, gint *error, Tag tags )
{
	FILE *fp=NULL;
	struct image *image=NULL;
	struct imageio *io=NULL;
	gint err;

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
		if( !( image=g_new0( struct image, 1 ) ) )
		{
			fclose( fp );
			if( error ) *error=ERR_MEM;
			return( NULL );
		}
		err=io->io_load( fp, image, (struct TagItem *)&tags );
		if( err )
		{
			g_free( image );
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

gint gfx_save( gchar *filename, struct image *image, Tag tags, ... )
{
	FILE *fp=NULL;
	struct imageio *io=NULL;
	gint err;

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
