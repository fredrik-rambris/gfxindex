/* gfx.h - Definitions and prototypes for gfx library
 *
 * GFXIndex (c) 1999-2001 Fredrik Rambris <fredrik@rambris.com>.
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

#ifndef __AMIGA__
#include <glib.h>
#include <taglist.h>
#else
#include "amigaglib.h"
#include <clib/utility_protos.h>
#endif

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
#define GFX	(TAG_USER+10000)
#define TAG_CONTAINS_POINTER 0x10000

struct image
{
	guchar *im_pixels; /* 3 bytes per pixel (RGB) */
	guchar *im_alpha; /* 1 byte per pixel. 0=transparent, 255=opaque */
	gint im_width;
	gint im_height;
};

struct color
{
	/* Set one of theese to negative to be ignored */
	guint r;
	guint g;
	guint b;
	guint opacity;
};

enum
{
	SCALE_NEAREST=1,
	SCALE_FINE
};

/* Prototypes */
struct image *gfx_allocimage( gint width, gint height, gboolean alpha, gint *err );
void gfx_freeimage( struct image *img, gboolean only_pixels );
gint gfx_parsecolor( struct color *col, gchar *colstr );
gboolean gfx_withinbounds( struct image *img, gint x, gint y );
void gfx_writepixel( struct image *img, gint x, gint y, struct color *col );
struct color *gfx_readpixel( struct image *img, gint x, gint y, struct color *col );
void gfx_rectfill( struct image *img, gint x1, gint y1, gint x2, gint y2, struct color *col );
void gfx_draw( struct image *img, gint x1, gint y1, gint x2, gint y2, struct color *col );
void gfx_scale_nearest( struct image *src_img, gint src_x, gint src_y, gint src_width, gint src_height, struct image *dst_img, gint dst_x, gint dst_y, gint dst_width, gint dst_height );
void gfx_scaleimage( struct image *src_img, gint src_x, gint src_y, gint src_width, gint src_height, struct image *dst_img, gint dst_x, gint dst_y, gint dst_width, gint dst_height, gint scale_type );
#endif /* GFX_H */

