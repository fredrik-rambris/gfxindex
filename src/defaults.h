/* defaults.c - Hardcoded defaults. 
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
#include "gfx.h"
#endif

#ifndef DEFAULT_CONFIG
	#define DEFAULT_CONFIG (NULL)
#endif

#ifndef DEFAULT_SAVECONFIG
	#define DEFAULT_SAVECONFIG (NULL)
#endif

#ifndef DEFAULT_QUIET
	#define DEFAULT_QUIET FALSE
#endif

#ifndef DEFAULT_VERBOSE
	#define DEFAULT_VERBOSE 1
#endif

#ifndef DEFAULT_THUMBDIR
	#define DEFAULT_THUMBDIR "thumbnails"
#endif

#ifndef DEFAULT_OUTDIR
	#define DEFAULT_OUTDIR (NULL)
#endif

#ifndef DEFAULT_MAKETHUMBS
	#define DEFAULT_MAKETHUMBS TRUE
#endif

#ifndef DEFAULT_THUMBWIDTH
	#define DEFAULT_THUMBWIDTH 128
#endif

#ifndef DEFAULT_THUMBHEIGHT
	#define DEFAULT_THUMBHEIGHT 128
#endif

#ifndef DEFAULT_WIDTHS
	#define DEFAULT_WIDTHS (NULL)
#endif

#ifndef DEFAULT_DEFWIDTH
	#define DEFAULT_DEFWIDTH 0
#endif

#ifndef DEFAULT_RELWIDTH
	#define DEFAULT_RELWIDTH 0
#endif

#ifndef DEFAULT_COPY
	#define DEFAULT_COPY FALSE
#endif

#ifndef DEFAULT_ORIGINAL
	#define DEFAULT_ORIGINAL FALSE
#endif

#ifndef DEFAULT_BIGQUALITY
	#define DEFAULT_BIGQUALITY 85
#endif

#ifndef DEFAULT_QUALITY
	#define DEFAULT_QUALITY 65
#endif

#ifndef DEFAULT_BGCOLOR
	#define DEFAULT_BGCOLOR "#000000"
#endif

#ifndef DEFAULT_BACKGROUND
	#define DEFAULT_BACKGROUND (char *)NULL
#endif

#ifndef DEFAULT_ALPHA
	#define DEFAULT_ALPHA (char *)NULL
#endif

#ifndef DEFAULT_BEVELBACK
	#define DEFAULT_BEVELBACK "#c0c0c0"
#endif

#ifndef DEFAULT_BEVELBRIGHT
	#define DEFAULT_BEVELBRIGHT "#ffffff"
#endif

#ifndef DEFAULT_BEVELDARK
	#define DEFAULT_BEVELDARK "#000000"
#endif

#ifndef DEFAULT_BEVEL
	#define DEFAULT_BEVEL FALSE
#endif

#ifndef DEFAULT_INNERBEVEL
	#define DEFAULT_INNERBEVEL TRUE
#endif

#ifndef DEFAULT_RECURSIVE
	#define DEFAULT_RECURSIVE FALSE
#endif

#ifndef DEFAULT_OVERWRITE
	#define DEFAULT_OVERWRITE FALSE
#endif

#ifndef DEFAULT_REMAKETHUMBS
	#define DEFAULT_REMAKETHUMBS FALSE
#endif

#ifndef DEFAULT_REMAEBIGS
	#define DEFAULT_REMAKEBIGS FALSE
#endif

#ifndef DEFAULT_PAD
	#define DEFAULT_PAD FALSE
#endif

#ifndef DEFAULT_SOFTPAD
	#define DEFAULT_SOFTPAD TRUE
#endif

#ifndef DEFAULT_DIR
	#define DEFAULT_DIR ""
#endif

#ifndef DEFAULT_GENINDEX
	#define DEFAULT_GENINDEX TRUE
#endif

#ifndef DEFAULT_TITLES
	#define DEFAULT_TITLES TRUE
#endif

#ifndef DEFAULT_EXTENSIONS
	#define DEFAULT_EXTENSIONS TRUE
#endif

#ifndef DEFAULT_CAPTIONS
	#define DEFAULT_CAPTIONS TRUE
#endif

#ifndef DEFAULT_NUMLINK
	#define DEFAULT_NUMLINK FALSE
#endif

#ifndef DEFAULT_THUMBSCALE
	#define DEFAULT_THUMBSCALE 0
#endif

#ifndef DEFAULT_DIRNAV
	#define DEFAULT_DIRNAV TRUE
#endif

#ifndef DEFAULT_XSTOP
	#define DEFAULT_XSTOP 5
#endif

#ifndef DEFAULT_YSTOP
	#define DEFAULT_YSTOP 4
#endif

#ifndef DEFAULT_BODYARGS
	#define DEFAULT_BODYARGS (char *)NULL
#endif

#ifndef DEFAULT_TABLEARGS
	#define DEFAULT_TABLEARGS "BORDER=\"0\" CELLPADDING=\"2\" CELLSPACING=\"0\""
#endif

#ifndef DEFAULT_CELLARGS
	#define DEFAULT_CELLARGS "ALIGN=\"center\""
#endif

#ifndef DEFAULT_CSS
	#define DEFAULT_CSS (char *)NULL
#endif

#ifndef DEFAULT_CSSFILE
	#define DEFAULT_CSSFILE (char *)NULL
#endif

#ifndef DEFAULT_PARENTDOC
	#define DEFAULT_PARENTDOC (char *)NULL
#endif

#ifndef DEFAULT_STR_LEFT
	#define DEFAULT_STR_LEFT "["
#endif

#ifndef DEFAULT_STR_SPACE
	#define DEFAULT_STR_SPACE " "
#endif

#ifndef DEFAULT_STR_DIVIDER
	#define DEFAULT_STR_DIVIDER "|"
#endif

#ifndef DEFAULT_STR_RIGHT
	#define DEFAULT_STR_RIGHT "]"
#endif

#ifndef DEFAULT_STR_PREVIOUS
	#define DEFAULT_STR_PREVIOUS "Prev"
#endif

#ifndef DEFAULT_STR_NEXT
	#define DEFAULT_STR_NEXT "Next"
#endif

#ifndef DEFAULT_STR_INDEX
	#define DEFAULT_STR_INDEX "Index"
#endif

#ifndef DEFAULT_STR_PARENT
	#define DEFAULT_STR_PARENT "Parent"
#endif

#ifndef DEFAULT_SCALE
	#define DEFAULT_SCALE SCALE_SLOW
#endif

#ifndef DEFAULT_TITLE
	#define DEFAULT_TITLE (char *)NULL
#endif

#ifndef DEFAULT_CAPTION
	#define DEFAULT_CAPTION (char *)NULL
#endif

#ifndef DEFAULT_USETITLES
	#define DEFAULT_USETITLES FALSE
#endif

#ifndef DEFAULT_EXIF
	#define DEFAULT_EXIF TRUE
#endif

#ifndef DEFAULT_WRITEALBUM
	#define DEFAULT_WRITEALBUM FALSE
#endif

#ifndef DEFAULT_INDEXTITLE
	#define DEFAULT_INDEXTITLE "%t - Page %p / %P"
#endif

#ifndef DEFAULT_PICTURETITLE
	#define DEFAULT_PICTURETITLE "%t - %T ( %w x %h )"
#endif

#ifndef DEFAULT_INDEXHEADER
	#define DEFAULT_INDEXHEADER (char *)NULL
#endif

#ifndef DEFAULT_INDEXHEADERFILE
	#define DEFAULT_INDEXHEADERFILE (char *)NULL
#endif

#ifndef DEFAULT_INDEXFOOTER
	#define DEFAULT_INDEXFOOTER (char *)NULL
#endif

#ifndef DEFAULT_INDEXFOOTERFILE
	#define DEFAULT_INDEXFOOTERFILE (char *)NULL
#endif

#ifndef DEFAULT_PICTUREHEADER
	#define DEFAULT_PICTUREHEADER (char *)NULL
#endif

#ifndef DEFAULT_PICTUREHEADERFILE
	#define DEFAULT_PICTUREHEADERFILE (char *)NULL
#endif

#ifndef DEFAULT_PICTUREFOOTER
	#define DEFAULT_PICTUREFOOTER (char *)NULL
#endif

#ifndef DEFAULT_PICTUREFOOTERFILE
	#define DEFAULT_PICTUREFOOTERFILE (char *)NULL
#endif

#ifndef DEFAULT_FULLDOC
	#define DEFAULT_FULLDOC TRUE
#endif

#ifndef DEFAULT_FLAT
	#define DEFAULT_FLAT FALSE
#endif
