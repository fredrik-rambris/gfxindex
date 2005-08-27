/* gfx.h - Definitions and prototypes for gfx library
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

#ifndef GFX_H
#define GFX_H

#include <stdio.h>

#ifndef __AMIGA__
#include "util.h"
#include <taglist.h>
#else
#include "amigaglib.h"
#include <clib/utility_protos.h>
#endif

#include "util.h"

/* Errors codes */
#define ERR_OK 0 /* Everything is fine */
#define ERR_UNKNOWN 1 /* Unknown error */
#define ERR_NOFILE 2 /* The FILE-handle was NULL or no filename specified */
#define ERR_HEADER 3 /* Error while reading/writing headers */
#define ERR_MEM 4 /* Couldn't allocate memory */
#define ERR_IO 5 /* Could not read/write data */
#define ERR_FILE 6 /* File doesn't exist or couldn't open file */
#define ERR_UNKNOWNFORMAT 7 /* The file format isn't known */
#define ERR_LOADNOTPOSSIBLE 8 /* Loading is not supported with this format */
#define ERR_SAVENOTPOSSIBLE 9 /* Saving is not supported with this format */
#define ERR_COLPARSEERROR 10 /* Couldn't parse color */
#define ERR_UNKNOWNOPTION 11 /* Option not supported by this particular function */

extern char *gfx_errors[12];

/* Tag base */
#define GFX	(TAG_USER+0x10000)

typedef struct _Pixel
{
	unsigned char r, g, b, a;
} Pixel;

struct image
{
	Pixel *im_pixels; /* 4 bytes per pixel (RGBA) */
//	unsigned char *im_alpha; /* 1 byte per pixel. 0=transparent, 255=opaque */
	int im_width;
	int im_height;
	unsigned int im_loadmodule; /* The IO model used to load the image */
};

struct color
{
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int opacity;
};

enum
{
	SCALE_NEAREST=1,
	SCALE_SLOW
};

/* Prototypes */
struct image *gfx_allocimage( int width, int height, int *err );
void gfx_freeimage( struct image *img, BOOL only_pixels );
int gfx_parsecolor( struct color *col, char *colstr );
BOOL gfx_withinbounds( struct image *img, int x, int y );
void gfx_writepixel( struct image *img, int x, int y, struct color *col );
struct color *gfx_readpixel( struct image *img, int x, int y, struct color *col );
void gfx_rectfill( struct image *img, int x1, int y1, int x2, int y2, struct color *col );
void gfx_draw( struct image *img, int x1, int y1, int x2, int y2, struct color *col );
void gfx_scale_nearest( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height, BOOL apply_alpha );
void gfx_scaleimage( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height, int scale_type, BOOL apply_alpha );
void gfx_scale_fine( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height );
void gfx_rotate( struct image *img, int degrees );
void gfx_mixpixel( struct image *img, int x, int y, struct color *col );
void gfx_mix( struct image *brush, struct image *img, int offx, int offy );
void gfx_stack( struct image *brush, struct image *img );
void gfx_fixalpha( struct image *brush, struct image *img );
#endif /* GFX_H */

