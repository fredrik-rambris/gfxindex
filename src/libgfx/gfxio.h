/* gfxio.h - Definitions and functions for the I/O part of gfx library
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

#ifndef GFXIO_H
#define GFXIO_H

#include <stdio.h>
#include <taglist.h>
#include "gfx.h"
#include "exif.h"

#define GFXIO		(GFX+0x1000)

struct imageio
{
	Node node;
	/* Function used to identify a file and if this library can handle it */
	BOOL (*io_identify) ( FILE *file );
	/* Function to load image, allocate an image struct and point img to it */
	int (*io_load) ( FILE *file, struct image *img, struct TagItem *tags );
	/* Saves the image pointed to in img */
	int (*io_save) ( FILE *file, struct image *img, struct TagItem *tags );
	/* Fills the image struct with info about the file but doesn't load the actual pixeldata*/
	int (*io_getinfo) ( FILE *file, struct image *img, struct TagItem *tags );
	/* Frees any allocated stuff. Also the imageio-struct if it was manually allocated */
	void (*io_cleanup) ( void );
	/* Information (read only) about the library */
	char *io_info;
	/* Default extension to filenames */
	char *io_extension;
	/* Identification (used when selecting format in UIs etc ) */
	char *io_id;
};

extern List *ios;

int gfxio_init( void );
void gfxio_cleanup( void );
void img_clean( struct image *img );
void printioinfo( void );
struct imageio *identify_file( FILE *file, char *filename, int *error );
struct image *gfx_load( char *filename, int *error, Tag tags, ... );
int gfx_save( char *filename, struct image *image, Tag tags, ... );
int gfx_getinfo( char *filename, struct image *image, Tag tags, ... );
#endif /* GFXIO_H */

