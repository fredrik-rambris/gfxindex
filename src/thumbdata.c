#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "thumbdata.h"

List *thumbdata_new( List *list )
{
	if( !list ) list=list_new();
	else memset( (void *)list, '\0', sizeof( List ) );
	list->freenode=(free_picturenode);
	list->compare=(compare_picturenode);
	return list;
}

void free_picturenode( Node *pnode )
{
	struct PictureNode *node=(struct PictureNode *)pnode;
	struct Picture **p;
	if( node->pn_original.p_path ) free( node->pn_original.p_path );
	if( node->pn_thumbnail.p_path ) free( node->pn_thumbnail.p_path );
//	if( node->pn_imageinfo ) free( node->pn_imageinfo );
	if( node->pn_title ) free( node->pn_title );
	if( node->pn_caption ) free( node->pn_caption );
	if( node->pn_pictures )
	{
		p=node->pn_pictures;
		while( *p )
		{
			if( (*p)->p_path ) free( (*p)->p_path );
			free( (*p) );
			p++;
		}
		free( node->pn_pictures );
	}
	if( node->pn_dir ) free( node->pn_dir );
#if HAVE_LIBEXIF
	if( node->pn_exifinfo ) gfx_exif_free( node->pn_exifinfo, TRUE );
#endif
}

int compare_picturenode( Node *a, Node *b )
{
	struct PictureNode *A=(struct PictureNode *)a, *B=(struct PictureNode *)b;
	if( !a ) fprintf( stderr, "compare_picturenode: a==NULL\n" );
	if( !b ) fprintf( stderr, "compare_picturenode: b==NULL\n" );
	if( !a || !b ) return 0;
	if( A->pn_dir && !B->pn_dir ) return -1;
	else if( !A->pn_dir && B->pn_dir ) return 1;
	else if( A->pn_dir && B->pn_dir ) return strcasecmp( A->pn_dir, B->pn_dir );
	else
	{
		if( !A->pn_original.p_path ) fprintf( stderr, "compare_picturenode: A->pn_original.p_path==NULL\n" );
		if( !B->pn_original.p_path ) fprintf( stderr, "compare_picturenode: B->pn_original.p_path==NULL\n" );
		if( !A->pn_original.p_path || !B->pn_original.p_path ) return 0;
		return strcasecmp( A->pn_original.p_path, B->pn_original.p_path );
	}
	return 0;
}

struct PictureNode *get_picturenode( List *list, const char *path )
{
	struct PictureNode *pn;
	if( !path || !list || !list->head ) return NULL;
	for( pn=(struct PictureNode *)(list->head); pn; pn=(struct PictureNode *)(pn->node.next) )
	{
		if( fastcompare( path, pn->pn_original.p_path ) ) return pn;
	}
	return NULL;
}

void purge_thumbdata( List *list )
{
	struct PictureNode *pn, *next;
	if( !list || !list->head ) return;
	
	for( pn=(struct PictureNode *)(list->head); pn; )
	{
		next=(struct PictureNode *)(pn->node.next);
		if( pn->pn_skip || (!pn->pn_dir && !pn->pn_thumbnail.p_path) )
		{
			list_delete( list, &pn->node );
		}
		pn=next;
	}
}

void print_thumbdata( List *list )
{
	struct PictureNode *pn;
	struct Picture **p;
	if( !list || !list->head ) return;
	for( pn=(struct PictureNode *)(list->head); pn; pn=(struct PictureNode *)(pn->node.next) )
	{
		if( pn->pn_dir ) printf( "Dir: %s\n", pn->pn_dir );
		else printf( "'%s': '%s' (%dx%d), '%s' (%dx%d)\n", pn->pn_title, pn->pn_original.p_path, pn->pn_original.p_width, pn->pn_original.p_height, pn->pn_thumbnail.p_path, pn->pn_thumbnail.p_width, pn->pn_thumbnail.p_height );
		if( pn->pn_pictures )
		{
			p=pn->pn_pictures;
			while( *p )
			{
				if( (*p)->p_path )
				{
					printf( "\t'%s' (%dx%d)\n", (*p)->p_path, (*p)->p_width, (*p)->p_height );
				}
				p++;
			}
		}
	}
}
