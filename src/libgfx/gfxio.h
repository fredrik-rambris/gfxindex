/* gfxio.h - Definitions and functions for the I/O part of gfx library
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

#ifndef GFXIO_H
#define GFXIO_H

#include <stdio.h>
#include <taglist.h>
#include "gfx.h"

#define GFXIO		(GFX+1000)

struct imageio
{
	/* Function used to identify a file and if this library can handle it */
	gboolean (*io_identify) ( FILE *file );
	/* Function to load image, allocate an image struct and point img to it */
	gint (*io_load) ( FILE *file, struct image *img, struct TagItem *tags );
	/* Saves the image pointed to in img */
	gint (*io_save) ( FILE *file, struct image *img, struct TagItem *tags );
	/* Frees any allocated stuff. Also the imageio-struct if it was manually allocated */
	void (*io_cleanup) ( void );
	/* Information (read only) about the library */
	gchar *io_info;
	/* Default extension to filenames */
	gchar *io_extension;
	/* Identification (used when selecting format in UIs etc ) */
	gchar *io_id;
};

extern GList *ios;

gint gfxio_init( void );
void gfxio_cleanup( void );
void img_clean( struct image *img );
void printioinfo( void );
struct imageio *identify_file( FILE *file, gchar *filename, gint *error );
struct image *gfx_load( gchar *filename, gint *error, Tag tags );
gint gfx_save( gchar *filename, struct image *image, Tag tags, ... );
#endif /* GFXIO_H */

