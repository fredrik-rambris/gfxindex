/* gfx.c - Functions for manipulating image data
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

#include "gfx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#define xytoff(img,x,y) ((y*img->im_width)+x)

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

struct image *gfx_allocimage( gint width, gint height, gboolean alpha, gint *err )
{
	struct image *img=NULL;
	gint count;

	if( !( img=g_new0( struct image, 1 ) ) ) { if( err ) *err=ERR_MEM; return( NULL ); }
	if( (width*height)>0 )
	{
		img->im_width=width;
		img->im_height=height;
		if( !( img->im_pixels=g_new0( gchar, ((width*(height+1))+1)*3 ) ) ) { if( err ) *err=ERR_MEM; gfx_freeimage( img, FALSE ); return( NULL ); }
		if( alpha )
		{
			if( !( img->im_alpha=g_new( gchar, width*height ) ) ) { if( err ) *err=ERR_MEM; gfx_freeimage( img, FALSE ); return( NULL ); }
			for( count=0 ; count<width*height ; count++ ) img->im_alpha[count]=0xff;
		}
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
	g_free( (gpointer)ti->ti_Data );
	ti->ti_Data=(ULONG)0L;
}

void gfx_freeimage( struct image *img, gboolean only_pixels )
{
	if( !img ) return;
	if( img->im_pixels ) g_free( img->im_pixels );
	img->im_pixels=NULL;
	if( img->im_alpha ) g_free( img->im_alpha );
	img->im_alpha=NULL;
	img->im_width=img->im_height=0;
	//ForeachMask( (struct TagItem *)img->extension->data, TAG_CONTAINS_POINTER, fe_free_extension, NULL, FALSE );
	if( !only_pixels ) g_free( img );
}

gint gfx_parsecolor( struct color *col, gchar *colstr )
{
	gchar *ptr, conv[5]="0xff";
	if( !col ) return( ERR_COLPARSEERROR );
	if( !colstr ) return( ERR_COLPARSEERROR );
	if( strlen( colstr )<3 ) return( ERR_COLPARSEERROR );
	g_strdown( colstr );
	if( !strncmp( colstr, "0x", 2 ) ) ptr=colstr+2;
	else if( !strncmp( colstr, "#", 1 ) ) ptr=colstr+1;
	else if( !strncmp( colstr, "$", 1 ) ) ptr=colstr+1;
	else ptr=colstr;
	conv[2]=ptr[0];
	conv[3]=ptr[1];
	col->r=strtol( conv, (char **)NULL, 16 );
	conv[4]=ptr[0];
	conv[5]=ptr[1];
	col->g=strtol( conv, (char **)NULL, 16 );
	conv[6]=ptr[0];
	conv[7]=ptr[1];
	col->b=strtol( conv, (char **)NULL, 16 );
	return( ERR_OK );
}

gboolean gfx_withinbounds( struct image *img, gint x, gint y )
{
	if( !img ) return( FALSE );
	if( x>-1 && y>-1 && x<img->im_width && y<img->im_height ) return( TRUE );
	return( FALSE );
}

void gfx_writepixel( struct image *img, gint x, gint y, struct color *col )
{
	glong off;
	if( !img ) return;
	if( !img->im_pixels ) return;
	if( !gfx_withinbounds( img, x, y ) ) return;
	off=xytoff(img,x,y);

	if( img->im_alpha ) img->im_alpha[off]=col->opacity;
	else if( (col->opacity>-1) && (!img->im_alpha) )
	{
		gfloat opacity=(gfloat)col->opacity/255.0;
		off*=3;
		img->im_pixels[off]=(img->im_pixels[off]/opacity)+(col->r*opacity);
		off++;
		img->im_pixels[off]=(img->im_pixels[off]/opacity)+(col->g*opacity);
		off++;
		img->im_pixels[off]=(img->im_pixels[off]/opacity)+(col->b*opacity);
	}
	else
	{
		off=xytoff(img,x,y)*3;
		img->im_pixels[off]=col->r;
		off++;
		img->im_pixels[off]=col->g;
		off++;
		img->im_pixels[off]=col->b;
	}
	
}

struct color *gfx_readpixel( struct image *img, gint x, gint y, struct color *col )
{
	glong off=xytoff(img,x,y);
	if( !img ) return( NULL );
	if( !img->im_pixels ) return( NULL );
	if( !gfx_withinbounds( img, x, y ) ) return( NULL );

	if( img->im_alpha ) col->opacity=img->im_alpha[off];
	off*=3;
	col->r=img->im_pixels[off++];
	col->g=img->im_pixels[off++];
	col->b=img->im_pixels[off++];
	return( col );
}

void gfx_rectfill( struct image *img, gint x1, gint y1, gint x2, gint y2, struct color *col )
{
	gint row, rgb;
	glong off, end;
	if( !img ) return;
	if( !img->im_pixels ) return;

	for( row=y1 ; row<y2 ; row++ )
	{
		off=(row*img->im_width)+x1;
		end=off+x2-x1;
		for( ; off<end; off++ )
		{
			rgb=off*3;
			if( (col->opacity>-1) && (img->im_alpha) ) img->im_alpha[off]=col->opacity;
			if( col->r>-1 ) img->im_pixels[rgb++]=col->r;
			if( col->g>-1 ) img->im_pixels[rgb++]=col->g;
			if( col->b>-1 ) img->im_pixels[rgb++]=col->b;
			
		}
	}
}

void gfx_draw( struct image *img, gint x1, gint y1, gint x2, gint y2, struct color *col )
{
	gint dy, dx, incr1, incr2, d, x, y, xend, yend, xdirflag, ydirflag;
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

void gfx_scaleimage( struct image *src_img, gint src_x, gint src_y, gint src_width, gint src_height, struct image *dst_img, gint dst_x, gint dst_y, gint dst_width, gint dst_height, gint scale_type )
{
	switch( scale_type )
	{
		case SCALE_NEAREST:
		gfx_scale_nearest( src_img, src_x, src_y, src_width, src_height, dst_img, dst_x, dst_y, dst_width, dst_height );
		break;

		default:
		gfx_scale_nearest( src_img, src_x, src_y, src_width, src_height, dst_img, dst_x, dst_y, dst_width, dst_height );
		break;
	}
}

void gfx_scale_nearest( struct image *src_img, gint src_x, gint src_y, gint src_width, gint src_height, struct image *dst_img, gint dst_x, gint dst_y, gint dst_width, gint dst_height )
{
	gint x, y;
	struct color col;
	gfloat xratio, yratio;
	if( !( src_img && dst_img && src_img->im_pixels && dst_img->im_pixels ) ) return;
	col.opacity=255;
	xratio=(gfloat)dst_width/(gfloat)src_width;
	yratio=(gfloat)dst_height/(gfloat)src_height;

	for( y=0 ; y<dst_height ; y++ )
	{
		for( x=0 ; x<dst_width ; x++ )
		{
			gfx_readpixel( src_img, (x/xratio)+src_x, (y/yratio)+src_y, &col );
			gfx_writepixel( dst_img, x+dst_x, y+dst_y, &col );
		}
	}
}

//#include "gfx_scale_fine.h"
