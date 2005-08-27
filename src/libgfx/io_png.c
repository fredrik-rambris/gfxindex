/* io_png.c - Functions for loading and saving PNG files
 * Haven't implemented it totally yet. Please contribute if you want PNG
 * support.
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

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#ifdef HAVE_LIBPNG

#include "gfxio.h"
#include "io_png.h"
#include <png.h>
#include <setjmp.h>

BOOL png_identify( FILE *file );
int png_load( FILE *file, struct image *img, struct TagItem *tags );
int png_save( FILE *file, struct image *img, struct TagItem *tags );
int png_getinfo( FILE *file, struct image *img, struct TagItem *tags );
void png_cleanup( void );

	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL, end_info=NULL;
	int color_type, bit_depth, i;
	png_bytep *row_pointers=NULL;

static int gfx_png_error( int ret, struct image *img, png_structp png_ptr, png_infop info_ptr, png_infop end_info, png_bytep *row_pointers )
{
	if( img ) img_clean( img );
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	if( row_pointers ) free( row_pointers );

	return( ret );
}

const char *png_extensions[]=
{
	"png",
	NULL
};

static struct imageio iio=
{
	{ NULL, NULL },
	GFXIO_PNG,
	png_identify,
	png_load,
	NULL, //	png_save,
	png_getinfo,
	NULL,
	"io_png v0.1 by Fredrik Rambris.",
	png_extensions
};

struct imageio *png_init( void )
{
	return( &iio );
}

BOOL png_identify( FILE *file )
{
	char buf[8];
	int number;
	if( !file ) return FALSE;
	if( (number=fread( buf, 1, 8, file ) ) )
	{
		rewind( file ); /* We rewind the file. We could just give the first 8 bytes to every identify function but probably some will require more and therefor we let every library do it's own reading and checking */
		return !png_sig_cmp( buf, 0, number );
	}
	rewind( file );
	return FALSE;
}

int png_getinfo( FILE *file, struct image *img, struct TagItem *tags )
{
	int err=ERR_MEM;
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	png_bytep *row_pointers=NULL;

	if( !file ) return( ERR_NOFILE );

	if( !(png_ptr=png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL )) )
	{
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}

    if (setjmp(png_jmpbuf(png_ptr)))
    {
		return gfx_png_error( err, img, png_ptr, info_ptr, NULL, NULL );
	}

	if( !( info_ptr=png_create_info_struct( png_ptr ) ) )
	{
		png_destroy_read_struct( &png_ptr, (png_infopp)NULL, (png_infopp)NULL );
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}

	png_init_io( png_ptr, file );
	err=ERR_IO;
	png_read_info( png_ptr, info_ptr );
	img->im_width=png_get_image_width( png_ptr, info_ptr );
	img->im_height=png_get_image_height( png_ptr, info_ptr );
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return ERR_OK;
}

int png_load( FILE *file, struct image *img, struct TagItem *tags )
{
	int err=ERR_MEM;
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL, end_info=NULL;
	int color_type, bit_depth, i;
	png_bytep *row_pointers=NULL;

	if( !file ) return( ERR_NOFILE );

	if( !(png_ptr=png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL )) )
	{
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}

    if (setjmp(png_jmpbuf(png_ptr)))
    {
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}

	
	if( !( info_ptr=png_create_info_struct( png_ptr ) ) )
	{
		png_destroy_read_struct( &png_ptr, (png_infopp)NULL, (png_infopp)NULL );
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}

	if( !( end_info=png_create_info_struct( png_ptr ) ) )
	{
		png_destroy_read_struct( &png_ptr, &info_ptr, (png_infopp)NULL );
		return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	}
	png_init_io( png_ptr, file );

	/* Free the pixels */
	img_clean( img );
	
	err=ERR_IO;
	png_read_info( png_ptr, info_ptr );
	img->im_width=png_get_image_width( png_ptr, info_ptr );
	img->im_height=png_get_image_height( png_ptr, info_ptr );
	color_type=png_get_color_type( png_ptr, info_ptr );
	bit_depth=png_get_bit_depth( png_ptr, info_ptr );
	
	/* We ensure that we get RGBA pixels */
    if( color_type == PNG_COLOR_TYPE_PALETTE ) png_set_palette_to_rgb(png_ptr);
    if( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 ) png_set_gray_1_2_4_to_8(png_ptr);
    if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ) png_set_tRNS_to_alpha(png_ptr);
	if( bit_depth==16 ) png_set_strip_16( png_ptr );
	if( color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA ) png_set_gray_to_rgb( png_ptr );
	if( color_type == PNG_COLOR_TYPE_RGB ) png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	png_read_update_info(png_ptr, info_ptr);

	err=ERR_MEM;
	/* Allocate new fresh pixels */
	if( !(img->im_pixels=gfx_new( Pixel, img->im_width*img->im_height ) ) ) return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );

	if( !(row_pointers = gfx_new( png_bytep, img->im_height ) ) ) return gfx_png_error( err, img, png_ptr, info_ptr, end_info, row_pointers );
	for( i=0; i<img->im_height; i++ ) row_pointers[i]=(png_bytep)(&img->im_pixels[i*img->im_width]);
	err=ERR_IO;
	png_read_image( png_ptr, row_pointers );
	png_read_end(png_ptr, end_info);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	free( row_pointers );
	/* I don't want to handle */
	return ERR_OK;
}

int png_save( FILE *file, struct image *img, struct TagItem *tags )
{
	return ERR_OK;
}

#endif
