/* util.h - Prototypes etc for util.c
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

#ifndef UTIL_H
#define UTIL_H
#include "global.h"

#define gfx_new(type,number) (type *)malloc(sizeof(type)*number)
#define gfx_new0(type,number) (type *)calloc(number,sizeof(type))

BOOL fastcompare( const char *a, const char *b );
BOOL fastcasecompare( const char *a, const char *b );
char *stripws( char *str );
char *setstr( char *old, const char *new );
char *strmaxcat( char *dst, const char *src, int maxlen );
char *strnmaxcat( char *dst, const char *src, int len, int maxlen );
BOOL file_exist( char *filename );
char *str_replace( char *needle, char *str, char *haystack );
char *tackon( char *dir, char *file );
char *match( char **array, char *str );
int copyfile( char *file, char *destdir );
int *strtoarr( const char *str );
int *arrdup( const int *arr );
int arrlen( const int *arr );
BOOL strtobool( const char *s );
char **Text2ArrayPtr( char *Buffer, unsigned int Length, unsigned int *NOL );
char **readfile( const char *filename, int *numrows );

/* The original one in posix doesn't check for nulls etc */
#undef strdup

#ifndef strdup
#define strdup(s) gfx_strdup(s)
#endif

#define STR_ISSET(s) (s!=0 && s[0]!='\0')

char *gfx_strdup(const char *s);


#ifndef strtolower
	char *strtolower( char *s );
#endif

typedef struct _Node
{
	struct _Node *prev;
	struct _Node *next;
} Node;

typedef struct _List
{
	Node *head; /* We keep two pointers here to be able to both prepend and */
	Node *tail; /* append without traversing the whole list */
	int (*compare)( Node *a, Node *b); /* A compare function */
	void (*freenode)( Node *node ); /* A function called before each node is
									   freed */
} List;


typedef struct _StringNode
{
	Node node;
	char *str;
	
} StringNode;

List *list_new( void );
void list_append( List *list, Node *new_node );
void list_prepend( List *list, Node *new_node );
Node *list_defang( List *list, Node *node );
void list_delete( List *list, Node *node );
void list_sort( List *list );
void list_free( List *list, BOOL free_list );
int list_length( List *list );
void list_foreach( List *list, void (*callback)(Node *node) );
void free_stringnode( Node *node );
int compare_stringnode( Node *a, Node *b );
#endif /* UTIL_H */
