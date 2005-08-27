/* io_png.h - Definitions and prototypes for PNG-I/O
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

#ifndef IO_PNG_H
#define IO_PNG_H

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#define GFXIO_PNG		(GFXIO+200)

#ifdef HAVE_LIBPNG

#include "gfxio.h"

struct imageio *png_init( void );

#endif

#endif /* IO_PNG_H */
