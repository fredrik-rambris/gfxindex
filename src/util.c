/* util.c - Small useful functions
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/* Compares two strings and only returns TRUE of equal or FALSE if not */
BOOL fastcompare( const char *a, const char *b )
{
	/* To cope with null strings */
	if( !a && !b ) return( TRUE );
	else if( !a || !b ) return( FALSE );

	for( ; a[0] && b[0] ; a++, b++ )
	{
		if( a[0]!=b[0] ) return( FALSE ); // Return FALSE as soon as it discovers a difference.
	}
	if( a[0]!=b[0] ) return( FALSE );
	return( TRUE );
}

/* This translation table is used to convert UPPER CASE LETTERS to lower case
 * ones. This includes national chars as well (ISO-8859-1/Latin-1) */
static const char transtable[256]=
{
	  0, 
	  1,
	  2,
	  3,
	  4,
	  5,
	  6,
	  7,
	  8,
	  9,
	 10,
	 11,
	 12,
	 13,
	 14,
	 15,
	 16,
	 17,
	 18,
	 19,
	 20,
	 21,
	 22,
	 23,
	 24,
	 25,
	 26,
	 27,
	 28,
	 29,
	 30,
	 31,
	 32, // ' '
	 33, // '!'
	 34, // '"'
	 35, // '#'
	 36, // '$'
	 37, // '%'
	 38, // '&'
	 39, // '''
	 40, // '('
	 41, // ')'
	 42, // '*'
	 43, // '+'
	 44, // ','
	 45, // '-'
	 46, // '.'
	 47, // '/'
	 48, // '0'
	 49, // '1'
	 50, // '2'
	 51, // '3'
	 52, // '4'
	 53, // '5'
	 54, // '6'
	 55, // '7'
	 56, // '8'
	 57, // '9'
	 58, // ':'
	 59, // ';'
	 60, // '<'
	 61, // '='
	 62, // '>'
	 63, // '?'
	 64, // '@'
	 97, // 'a'
	 98, // 'b'
	 99, // 'c'
	100, // 'd'
	101, // 'e'
	102, // 'f'
	103, // 'g'
	104, // 'h'
	105, // 'i'
	106, // 'j'
	107, // 'k'
	108, // 'l'
	109, // 'm'
	110, // 'n'
	111, // 'o'
	112, // 'p'
	113, // 'q'
	114, // 'r'
	115, // 's'
	116, // 't'
	117, // 'u'
	118, // 'v'
	119, // 'w'
	120, // 'x'
	121, // 'y'
	122, // 'z'
	 91, // '['
	 92, // '\'
	 93, // ']'
	 94, // '^'
	 95, // '_'
	 96, // '`'
	 97, // 'a'
	 98, // 'b'
	 99, // 'c'
	100, // 'd'
	101, // 'e'
	102, // 'f'
	103, // 'g'
	104, // 'h'
	105, // 'i'
	106, // 'j'
	107, // 'k'
	108, // 'l'
	109, // 'm'
	110, // 'n'
	111, // 'o'
	112, // 'p'
	113, // 'q'
	114, // 'r'
	115, // 's'
	116, // 't'
	117, // 'u'
	118, // 'v'
	119, // 'w'
	120, // 'x'
	121, // 'y'
	122, // 'z'
	123, // '{'
	124, // '|'
	125, // '}'
	126, // '~'
	127,
	128,
	129,
	130,
	131,
	132,
	133,
	134,
	135,
	136,
	137,
	138,
	139,
	140,
	141,
	142,
	143,
	144,
	145,
	146,
	147,
	148,
	149,
	150,
	151,
	152,
	153,
	154,
	155,
	156,
	157,
	158,
	159,
	160,
	161, // '¡'
	162, // '¢'
	163, // '£'
	164, // '¤'
	165, // '¥'
	166, // '¦'
	167, // '§'
	168, // '¨'
	169, // '©'
	170, // 'ª'
	171, // '«'
	172, // '¬'
	173, // '­'
	174, // '®'
	175, // '¯'
	176, // '°'
	177, // '±'
	178, // '²'
	179, // '³'
	180, // '´'
	181, // 'µ'
	182, // '¶'
	183, // '·'
	184, // '¸'
	185, // '¹'
	186, // 'º'
	187, // '»'
	188, // '¼'
	189, // '½'
	190, // '¾'
	191, // '¿'
	224, // 'à'
	225, // 'á'
	226, // 'â'
	227, // 'ã'
	228, // 'ä'
	229, // 'å'
	230, // 'æ'
	231, // 'ç'
	232, // 'è'
	233, // 'é'
	234, // 'ê'
	235, // 'ë'
	236, // 'ì'
	237, // 'í'
	238, // 'î'
	239, // 'ï'
	240, // 'ð'
	241, // 'ñ'
	242, // 'ò'
	243, // 'ó'
	244, // 'ô'
	245, // 'õ'
	246, // 'ö'
	215, // '×'
	248, // 'ø'
	249, // 'ù'
	250, // 'ú'
	251, // 'û'
	252, // 'ü'
	253, // 'ý'
	254, // 'þ'
	223, // 'ß'
	224, // 'à'
	225, // 'á'
	226, // 'â'
	227, // 'ã'
	228, // 'ä'
	229, // 'å'
	230, // 'æ'
	231, // 'ç'
	232, // 'è'
	233, // 'é'
	234, // 'ê'
	235, // 'ë'
	236, // 'ì'
	237, // 'í'
	238, // 'î'
	239, // 'ï'
	240, // 'ð'
	241, // 'ñ'
	242, // 'ò'
	243, // 'ó'
	244, // 'ô'
	245, // 'õ'
	246, // 'ö'
	247, // '÷'
	248, // 'ø'
	249, // 'ù'
	250, // 'ú'
	251, // 'û'
	252, // 'ü'
	253, // 'ý'
	254, // 'þ'
	255  // 'ÿ'
};

/* Compares two strings ignoring case and only returns TRUE of equal or FALSE if not */
BOOL fastcasecompare( const char *a, const char *b )
{
	/* To cope with null strings */
	if( !a && !b ) return( TRUE );
	else if( !a || !b ) return( FALSE );

	for( ; a[0] && b[0] ; a++, b++ )
	{
		if( transtable[(int)a[0]]!=transtable[(int)b[0]] ) return( FALSE ); // Return FALSE as soon as it discovers a difference.
	}
	if( transtable[(int)a[0]]!=transtable[(int)b[0]] ) return( FALSE );
	return( TRUE );
}

/* Removes all whitespace from the beginning and end of str */
char *stripws( char *str )
{
	int start, end;
	if( !str ) return( NULL );
	for( start=0 ; str[start]==' ' || str[start]=='\n' || str[start]=='\r' || str[start]=='\t' ; start++ );
	if( str[start]=='\0' )
	{
		str[0]='\0';
		return( str );
	}
	for( end=strlen( str ) ; ((str[end]==' ') || (str[end]=='\0') || (str[end]=='\n') || (str[end]=='\r') || (str[end]=='\t')) && end>-1 ; end-- ) str[end]='\0';
	if( end<1 ) return( str );
	memmove( str, str+start, end-start+1 );
	str[ end-start+1 ]='\0';
	return( str );
}

/* Replaces old with new. Returns pointer to new pointer. */
char *setstr( char *old, const char *new )
{
	if( old )
	{
		free( old );
		old=NULL;
	}

	if( new )
	{
		if( ( old=gfx_new(char,strlen( new )+1 ) ) )
		{
			strcpy( old, new );
		}
	}
	return old;
}

/* Joins two strings while keeping the destination string from overflooding */
char *strmaxcat( char *dst, const char *src, int maxlen )
{
	int dstlen, srclen;
	if( !dst || !src || !maxlen ) return dst;
	dstlen=strlen( dst );
	srclen=strlen( src );
	if( dstlen<maxlen ) strncat( dst, src, MIN(srclen,maxlen-dstlen) );
	return dst;
}
/* Joins two strings while keeping the destination string from overflooding */
char *strnmaxcat( char *dst, const char *src, int len, int maxlen )
{
	int dstlen, srclen;
	if( !dst || !src || !maxlen ) return dst;
	dstlen=strlen( dst );
	srclen=MIN(strlen( src ),len);
	if( dstlen<maxlen ) strncat( dst, src, MIN(srclen,maxlen-dstlen) );
	return dst;
}


/* Returns TRUE if file exists and is stat'able */
BOOL file_exist( char *filename )
{
	char path[1024];
	static struct stat buf;
	if( !filename ) return( FALSE );
	if( !strncmp( filename, "~", 1 ) )
	{
		strcpy( (char *)path, (char *)getenv( "HOME" ) );
		strmaxcat( (char *)path, (char *)(filename+1), 1024 );
	}
	else strcpy( (char *)path, (char *)filename );
	return( stat( path, &buf )==0 );
}

/* Replaces all occurances of needle in haystack with str */
char *str_replace( char *needle, char *str, char *haystack )
{
	int times=0;
	char *work;
	char *ptr=haystack, *oldptr;
	int needle_len=strlen( needle ), str_len=strlen( str ), haystack_len=strlen( haystack );

	/* Count how many times needle is in haystack */
	while( ( ptr=strstr( ptr, needle ) ) )
	{
		times++;
		ptr+=needle_len;
	}
	if( !times ) return haystack;

	/* Allocate a new buffer */
	work=gfx_new( char, haystack_len+(times*str_len)-(times*needle_len) );
	work[0]='\0';
	oldptr=haystack;

	/* Do the magic */
	while( ( ptr=strstr( oldptr, needle ) ) )
	{
		if( ptr!=oldptr ) strncat( work, oldptr, (int)(ptr-oldptr) );
		strcat( work, str );
		oldptr=ptr+needle_len;
	}
	if( oldptr[0] ) strcat( work, oldptr );

	/* Now this is risky. If haystack can't hold the data we get a guru meditation here */
	/* One maybe should free haystack and return work instead */
	strcpy( haystack, work );

	free( work );
	return( haystack );
}

/* Tacks a filename on to a directoryname and divides them with a slash if
 * needed. Like the function in Installer found on AmigaOS */
char *tackon( char *dir, char *file )
{
	static char delimiter[2]={ PATH_DELIMITER, '\0' };
	if( !dir ) return( dir );
	if( !file ) return( dir );
	if( !strlen( file ) ) return( dir );
	if( strlen( dir ) && dir[strlen(dir)-1]!=PATH_DELIMITER ) strcat( dir, delimiter );
	strcat( dir, file );
	return( dir );
}

/* Traverses a nullterminated array of pointers to strings and return the */
/* item that matches str or NULL of none was found */
char *match( char **array, char *str )
{
	while( *array )
	{
		if( fastcompare( *array, str ) ) return( *array );
		array++;
	}
	return( (char *)0L );
}

/* Copy a file to the destdir */
int copyfile( char *file, char *destdir )
{
	FILE *fpi, *fpo;
	char *buf;
	int bytes;
	char *outpath;
	char *filename;
	int err=0;
	
	/* Determin the actual filename */
	if( !( filename=strrchr( file, PATH_DELIMITER ) ) ) filename=file;
	else filename++;

	/* We use a copy buffer of 64k */
	if( ( buf=gfx_new( char, 64*1024 ) ) )
	{
		if( ( outpath=gfx_new0( char, strlen( filename )+strlen( destdir )+2 ) ) )
		{
			strcpy( outpath, destdir );
			tackon( outpath, filename );
			if( ( fpi=fopen( file, "rb" ) ) )
			{
				if( ( fpo=fopen( outpath, "wb" ) ) )
				{
					while( ( bytes=fread( buf, 64*1024, 1, fpi ) ) )
					{
						if( !feof( fpi ) ) bytes=64*1024;
						fwrite( buf, bytes, 1, fpo );
					}
					fclose( fpo );
				} else err=1;
				fclose( fpi );
			} else err=1;
			free( outpath );
		} else err=1;
		free( buf );
	} else err=1;
	return( err );
}

/* Duplicates a string and returns a newly allocated copy. 
 * The result must be freed */
char *gfx_strdup(const char *s)
{
	char *out=NULL;
	int slen;
	if( !s ) return out;
	slen=strlen( s );
	if( (out=gfx_new(char,slen+1) ) )
	{
		strcpy( out, s );
	}
	return out;
}

#ifndef strtolower
/* Lowers all the chars in a string in place, returning the very same,
 * now altered string */
	char *strtolower( char *s )
	{
		char *out=s;
		if( !s ) return s;
		while( s[0] )
		{
			s[0]=transtable[(int)s[0]];
			s++;
		}
		return out;
	}
#endif

List *list_new( void )
{
	return gfx_new0( List, 1 );
}

void list_append( List *list, Node *new_node )
{
	if( !list || !new_node ) return;
	if( list->tail )
	{
		list->tail->next=new_node;
	}
	new_node->prev=list->tail;
	new_node->next=NULL;
	if( !list->head ) list->head=new_node;
	list->tail=new_node;
}

void list_prepend( List *list, Node *new_node )
{
	if( !list || !new_node ) return;
	if( list->head )
	{
		list->head->prev=new_node;
	}
	new_node->next=list->head;
	new_node->prev=NULL;
	if( !list->tail ) list->tail=new_node;
	list->head=new_node;
}

Node *list_defang( List *list, Node *node )
{
	if( !list || !list->head || !node ) return NULL;
	if( node->prev ) node->prev->next=node->next;
	if( node->next ) node->next->prev=node->prev;
	if( list->head==node)
	{
		if( node->next ) list->head=node->next;
		else if( node->prev ) list->head=node->prev;
		else list->head=NULL;
	}
	if( list->tail==node )
	{
		if( node->prev ) list->tail=node->prev;
		else if( node->next ) list->tail=node->next;
		else list->tail=NULL;
	}
	node->next=NULL;
	node->prev=NULL;
	return node;
}

void list_delete( List *list, Node *node )
{
	
	if( list_defang( list, node ) )
	{
		if( list->freenode ) list->freenode( node );
		free( node );
	}
}

void swapnodes( List *list, Node *a, Node *b )
{
	if( b->next )
	{
		b->next->prev=a;
	}
	a->next=b->next;
	if( a->prev )
	{
		a->prev->next=b;
	}
	b->prev=a->prev;

	a->prev=b;
	b->next=a;
	if( a==list->head ) list->head=b;
	if( b==list->tail ) list->tail=a;
}

/* A stupid bubblesort. It seems to work... please replace it */
void list_sort( List *list )
{
	Node *node;
	Node *stopnode;
	Node *next;
	if( list->head == list->tail ) return;
	if( !list->compare ) return;
	stopnode=list->tail->prev;
	while( stopnode!=list->head )
	{
		for( node=list->head; node->next; node=node->next )
		{
			if( list->compare( node, node->next )>0 )
			{
				next=node->next;
				swapnodes( list, node, node->next );
				node=next;
			}
		}
		if( stopnode ) stopnode=stopnode->prev;
		else break;
	}
}

void list_free( List *list, BOOL free_list )
{
	Node *node, *next;
	if(!list) return;
	node=list->head;
	if( !node ) return;
	while( node )
	{
		next=node->next;
		if( list->freenode ) list->freenode( node );
		free( node );
		node=next;
	}
	if( free_list ) free( list );
	else
	{
		list->head=NULL;
		list->tail=NULL;
	}
}

void list_foreach( List *list, void (*callback)(Node *node) )
{
	Node *node, *next;
	if( !list ) return;
	if( !list->head ) return;
	if( !callback ) return;
	node=list->head;
	while( node )
	{
		next=node->next;
		callback( node );
		node=next;
	}
}

int list_length( List *list )
{
	int num=0;
	Node *node;
	for( node=list->head; node; node=node->next ) num++;
	return num;
}

void free_stringnode( Node *node )
{
	StringNode *sn=(StringNode *)node;
	free( sn->str );
}

int compare_stringnode( Node *a, Node *b )
{
	StringNode *A=(StringNode *)a, *B=(StringNode *)b;
	if( !a ) fprintf( stderr, "compare_stringnode: a==NULL\n" );
	if( !b ) fprintf( stderr, "compare_stringnode: b==NULL\n" );
	if( !a || !b ) return 0;

	if( !A->str ) fprintf( stderr, "compare_stringnode: A->str==NULL\n" );
	if( !B->str ) fprintf( stderr, "compare_stringnode: B->str==NULL\n" );
	if( !A->str || !B->str ) return 0;
	return strcasecmp( A->str, B->str );

}

/* Convert a string of commaseparated numbers into a null terminated array if
   integers */
int *strtoarr( const char *str )
{
	char *work=strdup( str ), *ptr, *s;
	int times=0, *arr;
	s=work;
	/* First we count how may integers we have in our string */
	for(;;)
	{
		if( ( ptr=strchr(s,',') ) )
		{
			ptr[0]='\0';
			ptr++;
			if( atoi( s )==0 ) return NULL; /* Any nulls or invalid numbers is bad*/
			times++;
			s=ptr;
		}
		else
		{
			if( atoi( s )==0 ) return NULL; /* Any nulls or invalid numbers is bad*/
			times++;
			break;
		}
	}
	strcpy( work, str );
	s=work;
	arr=gfx_new0( int, times+1 );
	times=0;
	/* Then we actually do the conversion */
	for(;;)
	{
		if( ( ptr=strchr(s,',') ) )
		{
			ptr[0]='\0';
			ptr++;
			arr[times++]=atoi( s );
			s=ptr;
		}
		else
		{
			arr[times++]=atoi( s );
			break;
		}
	}
	free( work );
	return arr;
}

int *arrdup( const int *arr )
{
	int c, *newarr;
	if( !arr ) return NULL;
	c=arrlen( arr );
	newarr=gfx_new( int, c+1 );
	memcpy( newarr, arr, (c+1)*sizeof( int ) );
	return newarr;
}

int arrlen( const int *arr )
{
	int c;
	for( c=0; arr[c]; c++ );
	return c;
}

/* If the string is True, Yes, Yepp, Yo, Talliho it's true
   otherwise it's false */
BOOL strtobool( const char *s )
{
	char *a, *answers="01NYFTnyft";
	if( ( a=strchr( answers, s[0] ) ) )
	{
		return ( ((((int)(a-answers))%2)==1) );
	}
	return FALSE;
}


/*
    Takes a buffer of raw text (a normal textfile just plain read
	into memory and creates an array of pointers to the beginning of
	all the lines. By doing this there's no need to use buffered
	I/O-routines (like FGets etc.).

    Ported from Amiga.
*/
char **Text2ArrayPtr( char *Buffer, unsigned int Length, unsigned int *NOL )
{
	char **retptr;
	unsigned int pos, NumLines=0;

	if( !Buffer || !Length ) return NULL;

	/* Scan for newlines and replace them with NULLs. Also count # of lines */
	for( pos=0 ; pos<Length ; pos++ )
	{
		if( Buffer[pos]=='\n' )
		{
			Buffer[pos]='\0';
			NumLines++;
		}
		else if( Buffer[pos]=='\r' )
		{
			Buffer[pos]='\0';
			NumLines++;
			if( Buffer[pos+1]=='\n' )
			{
				Buffer[pos+1]=0x01;
				pos++;
			}
		}
	}

	if( !NumLines ) return( NULL );

	if( !( retptr=calloc( NumLines, sizeof( char * ) ) ) )
	{
		return( NULL );
	}

	/* Set NOL to the number of lines */
	if( NOL ) *NOL=NumLines;

	NumLines=0;

	retptr[0]=Buffer;
	Length--;

	for( pos=1 ; pos<Length ; pos++ )
	{
		if( Buffer[pos]=='\0' )
		{
			if( pos+1<Length )
			{
				if( Buffer[pos+1]==0x01 )
				{
					if( pos+2>=Length ) break;
					else pos++;
				}
				NumLines++;
				retptr[NumLines]=&Buffer[pos+1];
				if( Buffer[pos+1]==0x01 ) retptr[NumLines]++;
			}
		}
	}
	return( retptr );
}

char **readfile( const char *filename, int *numrows )
{
	char path[1024];
	long file_size;
	FILE *fp=NULL;
	char *buf=NULL;
	char **ret=NULL;
	if( !filename ) return( FALSE );
	if( !strncmp( filename, "~", 1 ) )
	{
		strcpy( (char *)path, (char *)getenv( "HOME" ) );
		strmaxcat( (char *)path, (char *)(filename+1), 1024 );
	}
	else strcpy( (char *)path, (char *)filename );
	if( !( fp=fopen( path, "rb" ) ) ) goto error;
	if( fseek( fp, 0, SEEK_END )!=0 ) goto error;
	if( !( file_size=ftell( fp ) ) ) goto error;
	if( fseek( fp, 0, SEEK_SET )!=0 ) goto error;
	if( !( buf=gfx_new( char, file_size ) ) ) goto error;
	if( !fread( buf, file_size, 1, fp ) ) goto error;
	fclose( fp );
	ret=Text2ArrayPtr( buf, file_size, numrows );
	goto success;
error:
	if( buf ) free( buf );
	if( ret ) free( ret );
	if( fp ) fclose( fp );
	buf=NULL;
	ret=NULL;
	fp=NULL;
success:
	return ret;
}
