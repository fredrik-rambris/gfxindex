/* gfx.c - Functions for manipulating image data
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

#include "gfx.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#define xytoff(img,x,y) (((y)*img->im_width)+(x))

#include "gd_rescale.h"

char *gfx_errors[12]=
{
	(char *)"Everything is fine",
	(char *)"Unknown error",
	(char *)"The FILE-handle was NULL or no filename specified",
	(char *)"Error while reading/writing headers",
	(char *)"Couldn't allocate memory",
	(char *)"Could not read/write data",
	(char *)"File doesn't exist or couldn't open file",
	(char *)"The file format isn't known",
	(char *)"Loading is not supported with this format",
	(char *)"Saving is not supported with this format",
	(char *)"Couldn't parse color",
	(char *)"Option not supported by this particular function"
};

struct image *gfx_allocimage( int width, int height, int *err )
{
	struct image *img=NULL;
//	int count;

	if( !( img=gfx_new0( struct image, 1 ) ) ) { if( err ) *err=ERR_MEM; return( NULL ); }
	if( (width*height)>0 )
	{
		img->im_width=width;
		img->im_height=height;
		if( !( img->im_pixels=gfx_new0( Pixel, ((width*(height+1))+1) ) ) ) { if( err ) *err=ERR_MEM; gfx_freeimage( img, FALSE ); return( NULL ); }
		if( err ) *err=ERR_OK;
		return( img );	
	}
	if( err ) *err=ERR_UNKNOWN;
	return( NULL );
}

void fe_free_extension( struct TagItem *ti, void *user_data )
{
	if( !ti ) return;
	if( !ti->ti_Data ) return;
	free( (void *)ti->ti_Data );
	ti->ti_Data=(ULONG)0L;
}

void gfx_freeimage( struct image *img, BOOL only_pixels )
{
	if( !img ) return;
	if( img->im_pixels )
	{
		free( img->im_pixels );
	}
	img->im_pixels=NULL;
	img->im_width=img->im_height=0;
	//ForeachMask( (struct TagItem *)img->extension->data, TAG_CONTAINS_POINTER, fe_free_extension, NULL, FALSE );
	if( !only_pixels )
	{
		free( img );
	}
}

int gfx_parsecolor( struct color *col, char *colstr )
{
	char *ptr, conv[5]="0xff";
//	fprintf( stderr, "col: 0x%08x, '%s'\n", col, colstr );
	if( !col ) return( ERR_COLPARSEERROR );
	if( !colstr ) return( ERR_COLPARSEERROR );
	if( strlen( colstr )<3 ) return( ERR_COLPARSEERROR );
	strtolower( colstr );
	if( !strncmp( colstr, "0x", 2 ) ) ptr=colstr+2;
	else if( !strncmp( colstr, "#", 1 ) ) ptr=colstr+1;
	else if( !strncmp( colstr, "$", 1 ) ) ptr=colstr+1;
	else ptr=colstr;
	conv[2]=ptr[0];
	conv[3]=ptr[1];
	col->r=strtol( conv, (char **)NULL, 16 );
	conv[2]=ptr[2];
	conv[3]=ptr[3];
	col->g=strtol( conv, (char **)NULL, 16 );
	conv[2]=ptr[4];
	conv[3]=ptr[5];
	col->b=strtol( conv, (char **)NULL, 16 );
	col->opacity=255;
	return( ERR_OK );
}

BOOL gfx_withinbounds( struct image *img, int x, int y )
{
	if( !img ) return( FALSE );
	if( x>-1 && y>-1 && x<img->im_width && y<img->im_height ) return( TRUE );
	return( FALSE );
}

static  void gfx_putpixel( Pixel *pixel, struct color *col )
{
	pixel->r=col->r;
	pixel->g=col->g;
	pixel->b=col->b;
	pixel->a=col->opacity;
}

static  void gfx_getpixel( Pixel *pixel, struct color *col )
{
	col->r=pixel->r;
	col->g=pixel->g;
	col->b=pixel->b;
	col->opacity=pixel->a;
}

static  void gfx_pixelcpy( Pixel *dst, Pixel *src )
{
	if( dst!=src ) memcpy( dst, src, sizeof( Pixel ) );
}

void gfx_writepixel( struct image *img, int x, int y, struct color *col )
{
	Pixel *pixel;
	if( !img ) return;
	if( !img->im_pixels ) return;
	if( !gfx_withinbounds( img, x, y ) ) return;
	pixel=&(img->im_pixels[xytoff(img,x,y)]);
	gfx_putpixel( pixel, col );
}

struct color *gfx_readpixel( struct image *img, int x, int y, struct color *col )
{
	Pixel *pixel;
	if( !img ) return( NULL );
	if( !img->im_pixels ) return( NULL );
	if( !gfx_withinbounds( img, x, y ) ) return( NULL );
	pixel=&(img->im_pixels[xytoff(img,x,y)]);
	gfx_getpixel( pixel, col );
	return( col );
}

void gfx_rectfill( struct image *img, int x1, int y1, int x2, int y2, struct color *col )
{
	int row;
		//, rgb;
	long off, end;
	Pixel *pixel;
	if( !img ) return;
	if( !img->im_pixels ) return;

	for( row=y1 ; row<y2 ; row++ )
	{
		off=(row*img->im_width)+x1;
		end=off+x2-x1;
		for( ; off<end; off++ )
		{
			pixel=&(img->im_pixels[off]);
			gfx_putpixel( pixel, col );
		}
	}
}

void gfx_draw( struct image *img, int x1, int y1, int x2, int y2, struct color *col )
{
	int dy, dx, incr1, incr2, d, x, y, xend, yend, xdirflag, ydirflag;
	dx=abs( x2-x1 );
	dy=abs( y2-y1 );
	if( dy<=dx )
	{
		d=2*dy-dx;
		incr1=2*dy;
		incr2=2*( dy-dx );
		if( x1>x2 )
		{
			x=x2;
			y=y2;
			ydirflag=(-1);
			xend=x1;
		}
		else
		{
			x=x1;
			y=y1;
			ydirflag=1;
			xend=x2;
		}
		gfx_writepixel( img, x, y, col );
		if( ( ( y2-y1 )*ydirflag )>0 )
		{
			while( x<xend )
			{
				x++;
				if( d<0 )
				{
					d+=incr1;
				}
				else
				{
					y++;
					d+=incr2;
				}
				gfx_writepixel( img, x, y, col );
			}
		}
		else
		{
			while( x<xend )
			{
				x++;
				if( d<0 )
				{
					d+=incr1;
				}
				else
				{
					y--;
					d+=incr2;
				}
				gfx_writepixel( img, x, y, col );
			}
		}		
	}
	else
	{
		d=2*dx-dy;
		incr1=2*dx;
		incr2=2*( dx-dy );
		if( y1>y2 )
		{
			y=y2;
			x=x2;
			yend=y1;
			xdirflag=(-1);
		}
		else
		{
			y=y1;
			x=x1;
			yend=y2;
			xdirflag=1;
		}
		gfx_writepixel( img, x, y, col );
		if( ( ( x2-x1 )*xdirflag )>0 )
		{
			while( y<yend )
			{
				y++;
				if( d<0 )
				{
					d+=incr1;
				}
				else
				{
					x++;
					d+=incr2;
				}
				gfx_writepixel( img, x, y, col );
			}
		}
		else
		{
			while( y<yend )
			{
				y++;
				if( d<0 )
				{
					d+=incr1;
				}
				else
				{
					x--;
					d+=incr2;
				}
				gfx_writepixel( img, x, y, col );
			}
		}
	}
}

void gfx_scaleimage( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height, int scale_type, BOOL apply_alpha )
{
	switch( scale_type )
	{
		case SCALE_NEAREST:
		gfx_scale_nearest( src_img, src_x, src_y, src_width, src_height, dst_img, dst_x, dst_y, dst_width, dst_height, apply_alpha );
		break;
/*
		case SCALE_FINE:
		gfx_scale_fine( src_img, src_x, src_y, src_width, src_height, dst_img, dst_x, dst_y, dst_width, dst_height );
		break;
*/
		case SCALE_SLOW:
		gdImageCopyResampled( dst_img, src_img, dst_x, dst_y, src_x, src_y, dst_width, dst_height, src_width, src_height, apply_alpha );
		break;
		
		default:
		gfx_scale_nearest( src_img, src_x, src_y, src_width, src_height, dst_img, dst_x, dst_y, dst_width, dst_height, apply_alpha );
		break;
	}
}

void gfx_scale_nearest( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height, BOOL apply_alpha )
{
	int x, y;
	struct color col;
	float xratio, yratio;
	if( !( src_img && dst_img && src_img->im_pixels && dst_img->im_pixels ) ) return;
	col.opacity=255;
	xratio=(float)dst_width/(float)src_width;
	yratio=(float)dst_height/(float)src_height;

	for( y=0 ; y<dst_height ; y++ )
	{
		for( x=0 ; x<dst_width ; x++ )
		{
			gfx_readpixel( dst_img, (x/xratio)+src_x, (y/yratio)+src_y, &col );
			if( apply_alpha ) gfx_mixpixel( dst_img, x+dst_x, y+dst_y, &col );
			else gfx_writepixel( dst_img, x+dst_x, y+dst_y, &col );
		}
	}
}

void gfx_rotate( struct image *img, int degrees )
{
	unsigned int x,y;
	struct image *dst;
//	struct color col;
	if( !( img && img->im_pixels ) ) return;

	switch( degrees )
	{
		case 0:
			return;
			break;
		case 90:
			if( ( dst=gfx_allocimage( img->im_height, img->im_width, NULL ) ) )
			{
				for( y=0; y<img->im_height; y++ )
				{
					for( x=0; x<img->im_width; x++ )
					{
						gfx_pixelcpy( &(dst->im_pixels[xytoff(dst,dst->im_width-y-1, x)]), &(img->im_pixels[xytoff(img,x,y)]) );
/*
						gfx_readpixel( img, x, y, &col );
						gfx_writepixel( dst, dst->im_width-y-1, x, &col );
*/
					}
				}
				memcpy( img->im_pixels, dst->im_pixels, img->im_width*img->im_height*sizeof(Pixel) );
				img->im_width=dst->im_width;
				img->im_height=dst->im_height;
				gfx_freeimage( dst, FALSE );
			}
			break;
		case 180:
			if( ( dst=gfx_allocimage( img->im_width, img->im_height, NULL ) ) )
			{
				for( y=0; y<img->im_height; y++ )
				{
					for( x=0; x<img->im_width; x++ )
					{
						gfx_pixelcpy( &(dst->im_pixels[xytoff(dst,dst->im_width-x-1, dst->im_height-y-1)]), &(img->im_pixels[xytoff(img,x,y)]) );
/*
						gfx_readpixel( img, x, y, &col );
						gfx_writepixel( dst, dst->im_width-x-1, dst->im_height-y-1, &col );
*/
					}
				}
				memcpy( img->im_pixels, dst->im_pixels, img->im_width*img->im_height*sizeof(Pixel) );
				gfx_freeimage( dst, FALSE );
			}
			break;
		case 270:
			if( ( dst=gfx_allocimage( img->im_height, img->im_width, NULL ) ) )
			{
				for( y=0; y<img->im_height; y++ )
				{
					for( x=0; x<img->im_width; x++ )
					{
						gfx_pixelcpy( &(dst->im_pixels[xytoff(dst, y, dst->im_height-x-1)]), &(img->im_pixels[xytoff(img,x,y)]) );
/*
						gfx_readpixel( img, x, y, &col );
						gfx_writepixel( dst, y, dst->im_height-x-1, &col );
*/
					}
				}
				memcpy( img->im_pixels, dst->im_pixels, img->im_width*img->im_height*sizeof( Pixel ) );
				img->im_width=dst->im_width;
				img->im_height=dst->im_height;
				gfx_freeimage( dst, FALSE );
			}
			break;
		default:
			break;
	}
}

void gfx_mixpixel( struct image *img, int x, int y, struct color *col )
{
	struct color dstcol;
	gfx_readpixel( img, x, y, &dstcol );
	dstcol.r=((dstcol.r*(255-col->opacity))+(col->r*col->opacity))/255;
	dstcol.g=((dstcol.g*(255-col->opacity))+(col->g*col->opacity))/255;
	dstcol.b=((dstcol.b*(255-col->opacity))+(col->b*col->opacity))/255;
	dstcol.opacity+=col->opacity;
	if( dstcol.opacity>255 ) dstcol.opacity=255;
	gfx_writepixel( img, x, y, &dstcol );
}

void gfx_mix( struct image *brush, struct image *img, int offx, int offy )
{
	int x, y;
	struct color col;
	Pixel *pixel;
	for( y=0; y<brush->im_height; y++ )
	{
		for( x=0; x<brush->im_width; x++ )
		{
			gfx_readpixel( img, x+offx, y+offy, &col );
			pixel=&(brush->im_pixels[xytoff(brush,x,y)]);
			col.r=((col.r*(255-pixel->a))+(pixel->r*pixel->a))/255;
			col.g=((col.g*(255-pixel->a))+(pixel->g*pixel->a))/255;
			col.b=((col.b*(255-pixel->a))+(pixel->b*pixel->a))/255;
			gfx_writepixel( img, x+offx, y+offy, &col );
		}
	}
}

/* Convinience function for scaling and stacking an image onto another */
void gfx_stack( struct image *brush, struct image *img )
{
//	struct image *middle=NULL;
	if( !brush || !img ) return;
	if( brush->im_width==img->im_width && brush->im_height==img->im_height )
	{
		gfx_mix( brush, img, 0, 0 );
	}
	else
	{
		gfx_scaleimage( brush, 0, 0, brush->im_width, brush->im_height, img, 0, 0, img->im_width, img->im_height, SCALE_SLOW, TRUE );
	}
}


void gfx_replacealpha( struct image *brush, struct image *img )
{
	unsigned int x,y;
	if( !brush || !img ) return;
	for( y=0;y<img->im_height;y++ )
	{
		for( x=0; x<img->im_width; x++ )
		{
			if( gfx_withinbounds( brush, x, y ) )
			{
				img->im_pixels[xytoff(brush,x,y)].a=brush->im_pixels[xytoff(brush,x,y)].a;
			}
		}
	}
}

void gfx_fixalpha( struct image *brush, struct image *img )
{
	struct image *middle=NULL;
	if( !brush || !img ) return;
	if( brush->im_width==img->im_width && brush->im_height==img->im_height )
	{
		gfx_replacealpha( brush, img );
	}
	else
	{
		if( ( middle=gfx_allocimage(img->im_width,img->im_height,NULL) ) )
		{
			gfx_scaleimage( brush, 0, 0, brush->im_width, brush->im_height, middle, 0, 0, middle->im_width, middle->im_height, SCALE_SLOW, FALSE );
			gfx_replacealpha( middle, img );
			gfx_freeimage( middle, FALSE );
		}
	}
}
//#include "gfx_scale_fine.h"
