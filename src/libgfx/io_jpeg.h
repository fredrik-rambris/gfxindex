/* io_jpeg.h - Definitions and prototypes for JPEG-I/O
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

#ifndef IO_JPEG_H
#define IO_JPEG_H

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#ifdef HAVE_LIBJPEG

#include "gfxio.h"

struct imageio *jpeg_init( void );

#define GFXIO_JPEG          (GFXIO+0x100)
#define GFXIO_JPEG_QUALITY  (GFXIO_JPEG+1)
#define GFXIO_JPEG_SCALE    (GFXIO_JPEG+2)

#endif

#endif /* IO_JPEG_H */
