#ifndef TAGLIST_H
#define TAGLIST_H

#include <stdio.h>

/* This is a reverse engineered port of the TagList concept found in AmigaOS
 * release 2 and later. As I haven't invented it I don't hold any copyrights.
 * Port by Fredrik Rambris <fredrik@rambris.com>.
 * Copyright (c) Amiga, Inc. */

/*****************************************************************************/


/* Tags are a general mechanism of extensible data arrays for parameter
 * specification and property inquiry. In practice, tags are used in arrays,
 * or chain of arrays.
 *
 */

typedef unsigned long ULONG;
typedef char BOOL;
typedef ULONG Tag;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

struct TagItem
{
    Tag	     ti_Tag;	/* identifies the type of data */
    ULONG ti_Data;	/* type-specific data	       */
};

/* constants for Tag.ti_Tag, control tag values */
#define TAG_DONE   (0L)	  /* terminates array of TagItems. ti_Data unused */
#define TAG_END	   (0L)   /* synonym for TAG_DONE			  */
#define	TAG_IGNORE (1L)	  /* ignore this item, not end of array		  */
#define	TAG_MORE   (2L)	  /* ti_Data is pointer to another array of TagItems
			   * note that this tag terminates the current array
			   */
#define	TAG_SKIP   (3L)	  /* skip this and the next ti_Data items	  */

/* differentiates user tags from control tags */
#define TAG_USER   ((ULONG)(1L<<31))

/* If the TAG_USER bit is set in a tag number, it tells utility.library that
 * the tag is not a control tag (like TAG_DONE, TAG_IGNORE, TAG_MORE) and is
 * instead an application tag. "USER" means a client of utility.library in
 * general, including system code like Intuition or ASL, it has nothing to do
 * with user code.
 */


/*****************************************************************************/


/* Tag filter logic specifiers for use with FilterTagItems() */
#define TAGFILTER_AND 0		/* exclude everything but filter hits	*/
#define TAGFILTER_NOT 1		/* exclude only filter hits		*/


/*****************************************************************************/


/* Mapping types for use with MapTags() */
#define MAP_REMOVE_NOT_FOUND 0	/* remove tags that aren't in mapList */
#define MAP_KEEP_NOT_FOUND   1	/* keep tags that aren't in mapList   */


/*****************************************************************************/

struct TagItem *AllocateTagItems( ULONG numTags );
void FreeTagItems( struct TagItem *tagList );
struct TagItem *NextTagItem( struct TagItem **tagItemPtr );
void RefreshTagItemClones( struct TagItem *clone, struct TagItem *original );
struct TagItem *CloneTagItems( struct TagItem *original );
struct TagItem *FindTagItem( Tag tagValue, struct TagItem *tagList );
ULONG GetTagData( Tag tagValue, ULONG defaultVal, struct TagItem *tagList );
BOOL TagInArray( Tag tagValue, Tag *tagArray );
ULONG FilterTagItems( struct TagItem *tagList, Tag *filterArray, ULONG logic );
void ApplyTagChanges( struct TagItem *list, struct TagItem *changeList );
void FilterTagChanges( struct TagItem *changeList, struct TagItem *originalList, ULONG apply );
void MapTags( struct TagItem *tagList, struct TagItem *mapList, ULONG mapType );
ULONG PackBoolTags( ULONG initialFlags, struct TagItem *tagList, struct TagItem *boolMap );

/*** NOT IMPLEMENTED ***/
// ULONG PackStructureTags( void *pack, ULONG *packTable, struct TagItem *tagList );
// ULONG UnpackStructureTags( void *pack, ULONG *packTable, struct TagItem *tagList );

/* Custom Functions */
void ForeachMask( struct TagItem *tagList, ULONG bitMask, void (*fe_hook)( struct TagItem *ti, void *user_data ), void *user_data, ULONG logic );
#endif
