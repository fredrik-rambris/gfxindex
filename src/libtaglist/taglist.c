#include "taglist.h"
#include <stdlib.h>
#include <string.h>

/* This is a reverse engineered port of the TagList concept found in AmigaOS
 * release 2 and later. As I haven't invented it I don't hold any copyrights.
 * Port by Fredrik Rambris <fredrik@rambris.com> 
 * Copyright (c) Amiga, Inc. */


/****************************************************************************/
/* Original Amiga-like functions                                            */
/****************************************************************************/

struct TagItem *AllocateTagItems( ULONG numTags )
{
	if( !numTags ) return( NULL );
	return( (struct TagItem *)calloc( sizeof( struct TagItem ), numTags ) );
}


void FreeTagItems( struct TagItem *tagList )
{
	if( tagList ) free( tagList );
}

struct TagItem *NextTagItem( struct TagItem **tagItemPtr )
{
	if( !tagItemPtr ) return( NULL );

	for( ; (*tagItemPtr)->ti_Tag!=TAG_DONE ; (*tagItemPtr)++ )
	{
		switch( (*tagItemPtr)->ti_Tag )
		{
			case TAG_IGNORE:
				continue;
			case TAG_SKIP:
				(*tagItemPtr)+=(*tagItemPtr)->ti_Data;
				continue;
			case TAG_MORE:
				*tagItemPtr=((struct TagItem *)(*tagItemPtr)->ti_Data)-1;
				continue;
			default:
				(*tagItemPtr)++;
				return( (*tagItemPtr)-1 );
		}
	}
	return( NULL );
}

void RefreshTagItemClones( struct TagItem *clone, struct TagItem *original )
{
	struct TagItem *tstate, *tag;
	tstate=original;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		memcpy( clone++, tag, sizeof( struct TagItem ) );
	}
}

struct TagItem *CloneTagItems( struct TagItem *original )
{
	struct TagItem *tstate, *tag, *newtags;
	ULONG count=0;
	tstate=original;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) ) count++;
	if( !( newtags=AllocateTagItems( count+1 ) ) ) return( NULL );
	RefreshTagItemClones( newtags, original );
	return( newtags );
}

struct TagItem *FindTagItem( Tag tagValue, struct TagItem *tagList )
{
	struct TagItem *tstate, *tag;
	if( !tagList ) return( NULL );
	tstate=tagList;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( tag->ti_Tag==tagValue ) return( tag );
	}
	return( NULL );
	
}

ULONG GetTagData( Tag tagValue, ULONG defaultVal, struct TagItem *tagList )
{
	struct TagItem *tag;
	if( !tagList ) return( defaultVal );
	if( !tagValue ) return( defaultVal );
	if( ( tag=FindTagItem( tagValue, tagList ) ) )
	{
		return( tag->ti_Data );
	}
	return( defaultVal );
}

BOOL TagInArray( Tag tagValue, Tag *tagArray )
{
	for( ; tagArray[0]!=TAG_DONE ; tagArray++ )
	{
		if( tagArray[0]==tagValue ) return( TRUE );
	}
	return( FALSE );
}

ULONG FilterTagItems( struct TagItem *tagList, Tag *filterArray, ULONG logic )
{
	struct TagItem *tstate, *tag;
	ULONG count=0;
	if( !tagList ) return( count );
	tstate=tagList;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( !( (ULONG)TagInArray( tag->ti_Tag, filterArray ) ^ logic ) ) tag->ti_Tag=TAG_IGNORE; /* Remove if everything is set */
		else count++; /* Count the left-overs */
	}
	return( count );
}

void ApplyTagChanges( struct TagItem *list, struct TagItem *changeList )
{
	struct TagItem *tstate, *tag, *changeTag;
	if( !list ) return;
	if( !changeList ) return;
	tstate=list;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( ( changeTag=FindTagItem( tag->ti_Tag, changeList ) ) )
		{
			tag->ti_Data=changeTag->ti_Data;
		}
	}
}

void FilterTagChanges( struct TagItem *changeList, struct TagItem *originalList, ULONG apply )
{
	struct TagItem *tstate, *tag, *originalTag;
	if( !originalList ) return;
	if( !changeList ) return;
	tstate=changeList;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( ( originalTag=FindTagItem( tag->ti_Tag, originalList ) ) )
		{
			if( tag->ti_Data==originalTag->ti_Data ) tag->ti_Tag=TAG_IGNORE;
			else if( apply ) originalTag->ti_Data=tag->ti_Data;
		}
	}
}

void MapTags( struct TagItem *tagList, struct TagItem *mapList, ULONG mapType )
{
	struct TagItem *tstate, *tag, *mapTag;
	if( !tagList ) return;
	if( !mapList ) return;
	tstate=tagList;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( ( mapTag=FindTagItem( tag->ti_Tag, mapList ) ) )
		{
			tag->ti_Tag=(Tag)mapTag->ti_Data;
		}
		else if( mapType==MAP_REMOVE_NOT_FOUND )
		{
			tag->ti_Tag=TAG_IGNORE;
		}
	}
}

ULONG PackBoolTags( ULONG initialFlags, struct TagItem *tagList, struct TagItem *boolMap )
{
	ULONG flags=initialFlags;
	struct TagItem *tstate, *tag, *boolTag;
	if( !tagList ) return( flags );
	if( !boolMap ) return( flags );
	tstate=boolMap;
	while( ( boolTag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( ( tag=FindTagItem( boolTag->ti_Tag, tagList ) ) )
		{
			if( tag->ti_Data ) flags|=boolTag->ti_Data;
			else  flags&=~boolTag->ti_Data;
		}
	}
	return( flags );
}


/****************************************************************************/
/* My own functions                                                         */
/****************************************************************************/

/* ForeachMask
 * Traverses through the taglist and for any tag whos bits matches those in
 * bitmask executes fe_hook. If logic is set to TRUE the logic is reversed.
 */
void ForeachMask( struct TagItem *tagList, ULONG bitMask, void (*fe_hook)( struct TagItem *ti, void *user_data ), void *user_data, ULONG logic )
{
	struct TagItem *tstate, *tag;
	if( !tagList ) return;
	if( !fe_hook ) return;
	tstate=tagList;
	while( ( tag=(struct TagItem *)NextTagItem( &tstate ) ) )
	{
		if( ( ( tag->ti_Tag & bitMask ) == bitMask ) ^ logic )
		{
			printf( "fe_hook( %d )\n", (int)tag->ti_Tag );
			fe_hook( tag, user_data );
		}
	}
}
