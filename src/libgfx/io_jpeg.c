/* io_jpeg.c - Functions for loading and saving JPEG files
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

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#ifdef HAVE_LIBJPEG

#include "gfxio.h"
#include "io_jpeg.h"
#include <setjmp.h>
#include <jpeglib.h>

gboolean jpeg_identify( FILE *file );
gint jpeg_load( FILE *file, struct image *img, struct TagItem *tags );
gint jpeg_save( FILE *file, struct image *img, struct TagItem *tags );
void jpeg_cleanup( void );

/***** JPEG ERROR HANDLING *****/

struct my_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

void my_error_exit( j_common_ptr cinfo )
{
	struct my_error_mgr *myerr=(struct my_error_mgr *)cinfo->err;
	(*cinfo->err->output_message)( cinfo );
	longjmp( myerr->setjmp_buffer, 1 );
}

gint jpeg_error( gint ret, struct image *img, gpointer cinfo )
{
	jpeg_destroy( (j_common_ptr)cinfo );
	if( img ) img_clean( img );
	return( ret );
}

/*******************************/

struct imageio iio=
{
	jpeg_identify,
	jpeg_load,
	jpeg_save,
	NULL,
	"io_jpeg v0.2 by Fredrik Rambris.",
	"jpg"
};

struct imageio *jpeg_init( void )
{
	return( &iio );
}

gboolean jpeg_identify( FILE *file )
{
	guchar buf[8];
	if( !file ) return( FALSE );
	fread( buf, 2, 1, file );
	rewind( file );
	if( ( buf[0]==0xff ) && ( buf[1]==0xd8 ) ) return( TRUE );
	return( FALSE );
}

gint jpeg_load( FILE *file, struct image *img, struct TagItem *tags )
{
	gint err=ERR_UNKNOWN;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];
	gint row_stride;

	if( !file ) return( ERR_NOFILE );
	
	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	if( setjmp( jerr.setjmp_buffer ) )
	{
		return( jpeg_error( err, img, &cinfo ) );
	}
	
	jpeg_create_decompress( &cinfo );
	jpeg_stdio_src( &cinfo, file );

	/* Read some header information */
	if( !(jpeg_read_header( &cinfo, TRUE ) ) ) return( jpeg_error( ERR_HEADER, img, &cinfo ) );

	/* We only work with RGB in memory, never grayscale or something else */
	cinfo.out_color_space=JCS_RGB;

	if( !(jpeg_start_decompress( &cinfo ) ) ) return( jpeg_error( ERR_UNKNOWN, img, &cinfo ) );

	row_stride=cinfo.output_width * cinfo.output_components;
	
	/* Free the pixels */
	img_clean( img );

	/* Allocate new fresh ones */
	if( !(img->im_pixels=g_malloc0( row_stride * cinfo.output_height ) ) ) return( jpeg_error( ERR_MEM, img, &cinfo ) );

	img->im_width=cinfo.output_width;
	img->im_height=cinfo.output_height;
	
	row_pointer[0]=img->im_pixels;
	err=ERR_IO;
	/* Now read the image data */
	while( cinfo.output_scanline < cinfo.output_height )
	{
		if( !(jpeg_read_scanlines( &cinfo, row_pointer, 1 ) ) ) return( jpeg_error( ERR_IO, NULL, &cinfo ) );
		row_pointer[0]+=row_stride;
	}
	/* We're done */
	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	return( ERR_OK );
}

gint jpeg_save( FILE *file, struct image *img, struct TagItem *tags )
{
	gint err=ERR_UNKNOWN;
	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;

	JSAMPROW row_pointer[1];
	gint row_stride=img->im_width*3;

	if( !file ) return( ERR_NOFILE );

	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	if( setjmp( jerr.setjmp_buffer ) )
	{
		return( jpeg_error( err, NULL, &cinfo ) );
	}	
	
	jpeg_create_compress( &cinfo );
	jpeg_stdio_dest( &cinfo, file );

	cinfo.image_width = img->im_width;
	cinfo.image_height = img->im_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults( &cinfo );
	jpeg_set_quality( &cinfo, (int)GetTagData( GFXIO_JPEG_QUALITY, 90, tags ), FALSE );
	jpeg_start_compress( &cinfo, TRUE );
	
	row_pointer[0]=img->im_pixels;

	err=ERR_IO;
	/* Now save the image data */
	while( cinfo.next_scanline < cinfo.image_height )
	{
		if( !(jpeg_write_scanlines( &cinfo, row_pointer, 1 ) ) ) return( jpeg_error( ERR_IO, NULL, &cinfo ) );
		row_pointer[0]+=row_stride;
	}

	/* We're done */
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	return( ERR_OK );
}

#endif
