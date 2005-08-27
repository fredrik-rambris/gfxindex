/* io_dcraw.c - Functions for loading RAW photos
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

#ifdef USE_DCRAW
#ifdef HAVE_LIBJPEG

#include "gfxio.h"
#include "io_dcraw.h"
#include "libdcraw.h"
#include "../exif.h"
#include <math.h>

BOOL dcraw_identify( FILE *file );
int dcraw_getinfo( FILE *file, struct image *img, struct TagItem *tags );
int dcraw_load( FILE *file, struct image *img, struct TagItem *tags );
static void write_image(dcrawhandle *handle, struct image *img);

const char *dcraw_extensions[]=
{
	"crw", /* Canon */
	"cr2", /* Canon */
	"nef", /* Nikon */
	"pef", /* Pentax */
	"raf", /* Fuji */
	"x3f", /* Foveon/Sigma */
	"bay", /* Casio */
	"orf", /* Olympus */
	"mrw", /* Minolta */
	"raw", /* Leica */
	"srf", /* Sony */
	"tif", /* Canon EOS1D	*/
	NULL
};

static struct imageio iio=
{
	{ NULL, NULL },
	GFXIO_DCRAW,
	dcraw_identify,
	dcraw_load,
	NULL,
	dcraw_getinfo,
	NULL,
	"io_dcraw v0.1 by Fredrik Rambris. Using code from David Coffin.",
	dcraw_extensions
};

struct imageio *dcraw_init( void )
{
	return( &iio );
}

BOOL dcraw_identify( FILE *file )
{
	dcrawhandle handle;
	if( !file ) return( ERR_NOFILE );
	memset( (void *)&handle, 0, sizeof( dcrawhandle ) );
	dcraw_inithandle( &handle );
	rewind( file );
	handle.ifp=file;
	return (identify(&handle)==0);
}

int dcraw_getinfo( FILE *file, struct image *img, struct TagItem *tags )
{
	dcrawhandle handle;
	if( !file ) return( ERR_NOFILE );
	if( !img ) return( ERR_UNKNOWN );
	memset( (void *)&handle, 0, sizeof( dcrawhandle ) );
	dcraw_inithandle( &handle );
	rewind( file );
	handle.ifp=file;
	if( identify(&handle)==0 )
	{
		img->im_width=handle.width;
		img->im_height=handle.height;
		return ERR_OK;
	}
	return ERR_UNKNOWN;
}

int dcraw_load( FILE *file, struct image *img, struct TagItem *tags )
{
	dcrawhandle handle;
	if( !file ) return( ERR_NOFILE );

	memset( (void *)&handle, 0, sizeof( dcrawhandle ) );
	dcraw_inithandle( &handle );
	rewind( file );
	handle.ifp=file;
	if( identify(&handle)!=0 ) return ERR_UNKNOWNFORMAT;
	
	/* Free the pixels */
	img_clean( img );

	/* Allocate new fresh ones */
	if( !(img->im_pixels=gfx_new( Pixel, handle.width*handle.height ) ) ) return( ERR_MEM );

	handle.quick_interpolate=(int)GetTagData( GFXIO_DCRAW_QUICK, 0, tags );
	handle.use_auto_wb=(int)GetTagData( GFXIO_DCRAW_AUTOWB, 0, tags );
	handle.use_camera_wb=(int)GetTagData( GFXIO_DCRAW_CAMWB, 1, tags );
	handle.shrink = (int)GetTagData( GFXIO_DCRAW_HALFSIZE, 0, tags ) && handle.filters;
	handle.iheight=(handle.height + handle.shrink) >> handle.shrink;
    handle.iwidth=(handle.width + handle.shrink) >> handle.shrink;
	handle.image=calloc( handle.iheight * handle.iwidth, sizeof *(handle.image) );
	if( !handle.image )
	{
		img_clean( img );
		return ERR_MEM;
	}
	(*handle.load_raw)( &handle );

	handle.height = handle.iheight;
	handle.width  = handle.iwidth;

	if( handle.is_foveon ) foveon_interpolate( &handle );
	else scale_colors(&handle);

	if( handle.shrink ) handle.filters=0;
    handle.trim=0;
    if (handle.filters && !handle.document_mode)
	{
		handle.trim=1;
		vng_interpolate( &handle );
	}
	convert_to_rgb( &handle );

	/* Do the transfer into img->pixels */
	write_image( &handle, img );
	
	free( handle.image );
	return ERR_OK;
}

static void write_image(dcrawhandle *handle, struct image *img)
{
  int row, col, i, val, total;
  Pixel *pixel;
  float max, mul, scale[0x10000];
  ushort *rgb;
  if( !img ) return;
/*
   Set the white point to the 99th percentile
 */
  i = handle->width * handle->height * (strcmp(handle->make,"FUJIFILM") ? 0.01 : 0.005);
  for (val=0x2000, total=0; --val; )
    if ((total += handle->histogram[val]) > i) break;
  max = val << 4;

  mul = handle->bright * 442 / max;
  scale[0] = 0;
  for (i=1; i < 0x10000; i++)
    scale[i] = mul * pow (i*2/max, handle->gamma_val-1);

  img->im_width=handle->width;
  img->im_height=handle->height;
  
  for (row=handle->trim; row < handle->height-handle->trim; row++)
  {
    for (col=handle->trim; col < handle->width-handle->trim; col++)
	{
		pixel=&(img->im_pixels[col+(img->im_width*row)]);
		rgb = handle->image[row*handle->width+col];
		val=rgb[0] * scale[rgb[3]];
		pixel->r=val>255?255:val;
		val=rgb[1] * scale[rgb[3]];
		pixel->g=val>255?255:val;
		val=rgb[2] * scale[rgb[3]];
		pixel->b=val>255?255:val;
		pixel->a=255;
    }
  }
}

ExifInfo *dcraw_getexif( char *file )
{
	dcrawhandle handle;
	ExifInfo *ei=NULL;
	if( !file ) return NULL;
	memset( (void *)&handle, 0, sizeof( dcrawhandle ) );
	dcraw_inithandle( &handle );
	if( !(handle.ifp=fopen( file, "rb" ) ) ) return NULL;
	if( identify(&handle)==0 )
	{
		if( ( ei=gfx_new0( ExifInfo, 1 ) ) )
		{
			ei->ei_make=setstr( ei->ei_make, handle.make );
			ei->ei_model=setstr( ei->ei_model, handle.model );
/*
			ei->ei_flash=get_exif_flash( ed );
			ei->ei_exposure=setstr( ei->ei_exposure, get_exif_value( ed, EXIF_TAG_EXPOSURE_TIME ) );
			ei->ei_aperture=setstr( ei->ei_aperture, get_exif_value( ed, EXIF_TAG_FNUMBER ) );
			ei->ei_rotate=get_exif_rotate( ed );
			ei->ei_date=setstr( ei->ei_date, get_exif_value( ed, EXIF_TAG_DATE_TIME ) );
			ei->ei_focal=setstr( ei->ei_focal, get_exif_value( ed, EXIF_TAG_FOCAL_LENGTH ) );
*/
			/* If we have no info then no deal */
			if( !ei->ei_make && !ei->ei_model && !ei->ei_exposure && !ei->ei_aperture && !ei->ei_date && !ei->ei_focal )
			{
				gfx_exif_free( ei, TRUE );
				ei=NULL;
			}
		}
	}
	if( handle.ifp ) fclose( handle.ifp );
	handle.ifp=NULL;
	return ei;
}
#else
ExifInfo *dcraw_getexif( char *file )
{
	return NULL;
}
#endif
#else
ExifInfo *dcraw_getexif( char *file )
{
	return NULL;
}
#endif
