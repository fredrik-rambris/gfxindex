/* io_jpeg.c - Functions for loading and saving JPEG files
 *
 * GFXIndex (c) 1999-2003 Fredrik Rambris <fredrik@rambris.com>.
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

BOOL jpeg_identify( FILE *file );
int jpeg_load( FILE *file, struct image *img, struct TagItem *tags );
int jpeg_save( FILE *file, struct image *img, struct TagItem *tags );
int jpeg_getinfo( FILE *file, struct image *img, struct TagItem *tags );
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

int jpeg_error( int ret, struct image *img, void *cinfo, unsigned char *pixels )
{
	jpeg_destroy( (j_common_ptr)cinfo );
	if( img ) img_clean( img );
	if( pixels )
	{
		fprintf( stdout, "jpeg_error Free: 0x%08x\n", (int)pixels );
		free( pixels );
		if( img ) img->im_pixels=NULL;
	}
	return( ret );
}

/*******************************/

static struct imageio iio=
{
	{ NULL, NULL },
	jpeg_identify,
	jpeg_load,
	jpeg_save,
	jpeg_getinfo,
	NULL,
	"io_jpeg v0.2 by Fredrik Rambris.",
	"jpg"
};

struct imageio *jpeg_init( void )
{
	return( &iio );
}

BOOL jpeg_identify( FILE *file )
{
	unsigned char buf[8];
	if( !file ) return( FALSE );
	fread( buf, 2, 1, file );
	rewind( file );
	if( ( buf[0]==0xff ) && ( buf[1]==0xd8 ) ) return( TRUE );
	return( FALSE );
}

int jpeg_getinfo( FILE *file, struct image *img, struct TagItem *tags )
{
	int err=ERR_UNKNOWN;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	if( !file ) return( ERR_NOFILE );
	
	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	if( setjmp( jerr.setjmp_buffer ) )
	{
		return( jpeg_error( err, img, &cinfo, NULL ) );
	}
	
	jpeg_create_decompress( &cinfo );
	jpeg_stdio_src( &cinfo, file );

	/* Read some header information */
	if( !(jpeg_read_header( &cinfo, TRUE ) ) ) return( jpeg_error( ERR_HEADER, img, &cinfo, NULL ) );

	img->im_width=cinfo.image_width;
	img->im_height=cinfo.image_height;
	
	/* We're done */
	jpeg_destroy_decompress( &cinfo );
	return( ERR_OK );
}

int jpeg_load( FILE *file, struct image *img, struct TagItem *tags )
{
	int err=ERR_UNKNOWN;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];
	unsigned char *row_pixels=NULL;
	int row_stride, row, col;
	Pixel *pixel;

	if( !file ) return( ERR_NOFILE );
	
	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	if( setjmp( jerr.setjmp_buffer ) )
	{
		return( jpeg_error( err, img, &cinfo, row_pixels ) );
	}
	
	jpeg_create_decompress( &cinfo );
	jpeg_stdio_src( &cinfo, file );

	/* Read some header information */
	if( !(jpeg_read_header( &cinfo, TRUE ) ) ) return( jpeg_error( ERR_HEADER, img, &cinfo, row_pixels ) );

	/* We only work with RGB in memory, never grayscale or something else */
	cinfo.out_color_space=JCS_RGB;

	cinfo.scale_denom=GetTagData( GFXIO_JPEG_SCALE, 1, tags );

	if( !(jpeg_start_decompress( &cinfo ) ) ) return( jpeg_error( ERR_UNKNOWN, img, &cinfo, row_pixels ) );

	row_stride=cinfo.output_width * cinfo.output_components;
	
	/* Free the pixels */
	img_clean( img );

	/* Allocate new fresh ones */
	if( !(img->im_pixels=gfx_new( Pixel, cinfo.output_width*cinfo.output_height ) ) ) return( jpeg_error( ERR_MEM, img, &cinfo, row_pixels ) );

	img->im_width=cinfo.output_width;
	img->im_height=cinfo.output_height;
	row_pixels=gfx_new0( unsigned char, row_stride );
	row_pointer[0]=row_pixels;
	row=0;
	err=ERR_IO;
	/* Now read the image data */
	while( cinfo.output_scanline < cinfo.output_height )
	{
		if( !(jpeg_read_scanlines( &cinfo, row_pointer, 1 ) ) ) return( jpeg_error( ERR_IO, NULL, &cinfo, row_pixels ) );
		for( col=0; col<img->im_width; col++ )
		{
			pixel=&(img->im_pixels[col+(img->im_width*row)]);
			pixel->r=row_pixels[col*3];
			pixel->g=row_pixels[(col*3)+1];
			pixel->b=row_pixels[(col*3)+2];
			pixel->a=255;
		}
		row++;
	}
	/* We're done */
	free( row_pixels );
	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	return( ERR_OK );
}

int jpeg_save( FILE *file, struct image *img, struct TagItem *tags )
{
	int err=ERR_UNKNOWN;
	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;

	JSAMPROW row_pointer[1];
	int row_stride=img->im_width*3;

	unsigned char *row_pixels=NULL;
	int row, col;
	Pixel *pixel;

	if( !file ) return( ERR_NOFILE );

	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	if( setjmp( jerr.setjmp_buffer ) )
	{
		return( jpeg_error( err, NULL, &cinfo, row_pixels ) );
	}	
	
	jpeg_create_compress( &cinfo );
	jpeg_stdio_dest( &cinfo, file );

	cinfo.image_width = img->im_width;
	cinfo.image_height = img->im_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults( &cinfo );
	jpeg_set_quality( &cinfo, (int)GetTagData( GFXIO_JPEG_QUALITY, 90, tags ), FALSE );

	/* These slow down encoding but speed up downloading */
	cinfo.optimize_coding=TRUE;
	jpeg_simple_progression( &cinfo );

	jpeg_start_compress( &cinfo, TRUE );

	row_pixels=gfx_new( unsigned char, row_stride );
	row_pointer[0]=row_pixels;
	row=0;

	err=ERR_IO;
	/* Now save the image data */
	while( cinfo.next_scanline < cinfo.image_height )
	{
		for( col=0; col<img->im_width; col++ )
		{
			pixel=&(img->im_pixels[col+(img->im_width*row)]);
			row_pixels[col*3]=pixel->r;
			row_pixels[(col*3)+1]=pixel->g;
			row_pixels[(col*3)+2]=pixel->b;
		}
		if( !(jpeg_write_scanlines( &cinfo, row_pointer, 1 ) ) ) return( jpeg_error( ERR_IO, NULL, &cinfo, row_pixels ) );
		row++;
	}

	/* We're done */
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	return( ERR_OK );
}

#endif
