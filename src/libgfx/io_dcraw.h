/* io_dcraw.h - Definitions and prototypes for Raw I/O
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

#ifndef IO_DCRAW_H
#define IO_DCRAW_H

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#define GFXIO_DCRAW		(GFXIO+300)
#define GFXIO_DCRAW_QUICK	(GFXIO_DCRAW+1)
#define GFXIO_DCRAW_HALFSIZE	(GFXIO_DCRAW+2)
#define GFXIO_DCRAW_AUTOWB	(GFXIO_DCRAW+3)
#define GFXIO_DCRAW_CAMWB	(GFXIO_DCRAW+4)

#ifdef HAVE_LIBJPEG

#include "gfxio.h"

struct imageio *dcraw_init( void );
#endif
ExifInfo *dcraw_getexif( char *file );

#endif /* IO_DCRAW_H */
