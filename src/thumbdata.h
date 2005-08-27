/* thumbdata.h - Definitions of the thumbnail data structures.
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

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef THUMBDATA_H
#define THUMBDATA_H

#include "util.h"
#include "exif.h"


struct Picture
{
	char *p_path;
	int p_width;
	int p_height;
};

struct PictureNode
{
	Node node;
	unsigned int pn_loadmodule; /* The IO module used to load the image */
	BOOL pn_skip;
	int pn_rotate; /* Degrees of rotation 0, 90, 180, 270 */
	char *pn_title, *pn_caption; /* If not in exif then here */
	struct Picture pn_original;
	struct Picture **pn_pictures; /* An null terminated array of Picture's */
	struct Picture pn_thumbnail;
	char *pn_dir; /* If it's a dir then this is the name */
	ExifInfo *pn_exifinfo;
};

List *thumbdata_new( List *list );
void free_picturenode( Node *pnode );
int compare_picturenode( Node *a, Node *b );
void print_thumbdata( List *list );
struct PictureNode *get_picturenode( List *list, const char *path );
void purge_thumbdata( List *list );
#endif /* THUMBDATA_H */
