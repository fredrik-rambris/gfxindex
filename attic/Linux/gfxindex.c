/*
 * GFXIndex (c) 1999 Fredrik Rambris. All rights reserved. Use as you like as
 * long as you give me credits for my work.
 *
 * GFXIndex is a tool that creates HTML-indices of our images. Use with
 * supplied 'makethumbs' script.
 *
 * ** NOTE! This is a port to ANSI-C from a version I made on Amiga with a lot
 * of Amiga-specific stuff. This should now be 99% ANSI-C and should compile
 * under ANSI-C compliant compilers. Tested with egcs under Linux.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VERSION "1.6"
#define OS "Linux"

#ifndef stricmp
#define stricmp(s1,s2) strcasecmp(s1,s2)
#endif

FILE *dbfile=NULL;
FILE *ifile=NULL;
char IndexString[1024]="";
char ThumbString[1024]="";
char *PageTitle=NULL;

struct ThumbData
{
	struct ThumbData *prev;
	struct ThumbData *next;
	char *image_file;
	int image_width;
	int image_height;

	char *thumb_file;
	int thumb_width;
	int thumb_height;
};
struct ThumbData *thumblist=NULL;

/*** BEGIN PREFS ***/

/* DEFAULT DEFINES */
#define DEF_LEFT "["
#define DEF_MID "|"
#define DEF_RIGHT "]"
#define DEF_SPACE " "
#define DEF_PREV "Previous"
#define DEF_NEXT "Next"
#define DEF_PARENT "Parent"
#define DEF_INDEXSTR "Index"
#define DEF_PARENTDOC "../"

/* Chars to make bars like [ Prev | 0 | 1 | 2 | 3 | Next | Parent ] */
char *left=NULL;
char *mid=NULL;
char *right=NULL;
char *space=NULL;
char *prev=NULL;
char *next=NULL;
char *parent=NULL;
char *indexstr=NULL;

/* How many pics on each page */
int xstop=7, ystop=4;

/* Parent document */
char *parentdoc=NULL;

int useparent=0;

/* Use thumbnails for Prev and Next */
int thumb_prev_next=0;
/* Scale of these thumbs... 1=original size, 2=half etc ... */
float scale=1.5;

/* Use Numeric Links in the indices. eg. [ Prev | 1 | 2 | 3 | 4 | 5 | Next ] */
int use_num_links=0;

char *BottomString=NULL;

/*** END PREFS ***/

char *setstr( char *oldstr, char *newstr )
{
	if( oldstr )
	{
		free( oldstr );
		oldstr=NULL;
	}

	if( newstr )
	{
		if( oldstr=malloc( strlen( newstr )+1 ) )
		{
			strcpy( oldstr, newstr );
		}
	}

	return( oldstr );
}

void quit( int code )
{
	struct ThumbData *node, *prevnode;

	if( dbfile )
	{
		fclose( dbfile );
		dbfile=NULL;
	}
	if( ifile )
	{
		fclose( ifile );
		ifile=NULL;
	}

	PageTitle=setstr( PageTitle, NULL );
	left=setstr( left, NULL );
	mid=setstr( mid, NULL );
	right=setstr( right, NULL );
	space=setstr( space, NULL );
	prev=setstr( prev, NULL );
	next=setstr( next, NULL );
	parent=setstr( parent, NULL );
	indexstr=setstr( indexstr, NULL );
	parentdoc=setstr( parentdoc, NULL );
	BottomString=setstr( BottomString, NULL );

	if( thumblist )
	{
		prevnode=NULL;
		for( node=thumblist; node; node=node->next )
		{
			if( prevnode ) free( prevnode );
			prevnode=NULL;
				if( node->image_file ) free( node->image_file );
				if( node->thumb_file ) free( node->thumb_file );
				prevnode=node;
		}
		if( prevnode ) free( prevnode );
		prevnode=NULL;
	}
    exit( code );
}

void indexline( int current, int total )
{
    int count;
    char tempbuf[128];
	if( (!parentdoc) | (!parent) ) useparent=0;
    IndexString[0]='\0';
	if( (!total) & ( !useparent ) ) return;
	if( ( useparent ) | (total) ) if( left ) strcat( IndexString, left );
	if( prev )
	{
		if( space ) strcat( IndexString, space );
	    if( current>1 )
		{
			sprintf( tempbuf, "<A HREF=\"index%ld.html\">", current-1 );
			strcat( IndexString, tempbuf );
		}
	    else if ( current==1 )
		{
			sprintf( tempbuf, "<A HREF=\"index.html\">");
			strcat( IndexString, tempbuf );
		}
	    if( current )
		{
			sprintf( tempbuf, "%s</A>", prev );
			strcat( IndexString, tempbuf );
			if( space ) strcat( IndexString, space );

			if( total && ( (current<total) || use_num_links || useparent ) )
			{
				if( mid ) strcat( IndexString, mid );
				if( space ) strcat( IndexString, space );
			}
		}
	}
	if( ( total!=0 ) & ( use_num_links!=0 ) )
	{
		for( count=0 ; count<=total ; count++ )
		{
			if( count!=current )
			{
				if( !count ) sprintf( tempbuf, "%s<A HREF=\"index.html\">1</A>%s", space?space:"", space?space:"" );
				else sprintf( tempbuf, "%s<A HREF=\"index%ld.html\">%ld</A>%s", space?space:"", count, count+1, space?space:"" );
			}
			else
			{
				sprintf( tempbuf, "%s%ld%s", space?space:"", count+1, space?space:"" );
			}
			strcat( IndexString, tempbuf );
			if( ( count<total ) || useparent || ( current<total ) ) if( mid ) strcat( IndexString, mid );
		}
		if( space ) strcat( IndexString, space );
	}
	if( useparent )
	{
		sprintf( tempbuf, "<A HREF=\"%s\">%s</A>", parentdoc, parent);
		strcat( IndexString, tempbuf );
		if( (next!=0) && ( current<total ) )
		{
			if( space ) strcat( IndexString, space );
			if( mid ) strcat( IndexString, mid );
			if( space ) strcat( IndexString, space );
		}
	}
    //strcat( IndexString, " <A HREF=\"../\">Parent</A> | " );
	if( next )
	{
		if( current<total )
		{
			sprintf( tempbuf, "<A HREF=\"index%ld.html\">%s</A>", current+1, next );
			strcat( IndexString, tempbuf );
		}
	}
	if( (total!=0) || ( useparent ) )
	{
		if( space ) strcat( IndexString, space );
		if( right ) strcat( IndexString, right );
	}
}

void ThumbIndex( struct ThumbData *node, int icount )
{
	char tempbuf[128];
	char *tempptr;
	ThumbString[0]='\0';
	if( left ) strcat( ThumbString, left );
	if( space ) strcat( ThumbString, space );

	if( ( node->prev!=0 ) && ( ( prev!=0 ) || (thumb_prev_next) ) )
	{
		sprintf( tempbuf, "<A HREF=\"%s.html\">", node->prev->image_file );
		strcat( ThumbString, tempbuf );
		if( thumb_prev_next )
		{
			if( tempptr=(char *)strchr( node->prev->thumb_file, '/' ) ) tempptr++;
			else tempptr=node->prev->thumb_file;
			sprintf( tempbuf, "<IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\" ALT=\"Previous\">", tempptr, (int)(((float)node->prev->thumb_width)/scale), (int)(((float)node->prev->thumb_height)/scale) );
			strcat( ThumbString, tempbuf );
		}
		else
		{
			strcat( ThumbString, prev );
		}
		sprintf( tempbuf, "</A>%s%s%s", space?space:"", mid?mid:"", space?space:"" );
		strcat( ThumbString, tempbuf );
	}
	if( indexstr )
	{
		strcat( ThumbString, "<A HREF=\"../" );
		if( icount ) sprintf( tempbuf, "index%ld.html", icount );
		else sprintf( tempbuf, "index.html" );
		strcat( ThumbString, tempbuf );
		sprintf( tempbuf, "\">%s</A>%s", indexstr, space?space:"" );
		strcat( ThumbString, tempbuf );
		if( ( node->next!=0 ) && ( ( next!=0 ) || (thumb_prev_next) ) )
		{
			if( mid ) strcat( ThumbString, mid );
			if( space ) strcat( ThumbString, space );
		}
	}
	if( ( node->next!=0 ) && ( ( next!=0 ) || (thumb_prev_next) ) )
	{
		sprintf( tempbuf, "<A HREF=\"%s.html\">", node->next->image_file );
		strcat( ThumbString, tempbuf );
		if( thumb_prev_next )
		{
			if( tempptr=(char *)strchr( node->next->thumb_file, '/' ) ) tempptr++;
			else tempptr=node->next->thumb_file;
			sprintf( tempbuf, "<IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\" ALT=\"Next\">", tempptr, (int)(((float)node->next->thumb_width)/scale), (int)(((float)node->next->thumb_height)/scale) );
			strcat( ThumbString, tempbuf );
		}
		else
		{
			strcat( ThumbString, next );
		}
		sprintf( tempbuf, "</A>%s", space?space:"" );
		strcat( ThumbString, tempbuf );
	}
	sprintf( tempbuf, "%s<BR>\n", right?right:"" );
	strcat( ThumbString, tempbuf );
}

void intro( int much )
{
    if( much )
    {
	printf("GFXIndex v" VERSION " (" OS ") by Fredrik Rambris <boost@amiga.nu>\n");
	if( much>1 )
	{
	    printf("\nUSAGE: gfxindex [OPTIONS...] [TITLE]\n\n");
	    printf("OPTIONS:\n");
	    printf("-h or -help     - This page\n");
	    printf("-xstop, -ystop  - Number of thumbnails per page         ( DEFAULT = %ld x %ld )\n", xstop, ystop);
	    printf("Navbar components [ Previous | 0 | 1 | 2 | 3 | Parent | Next ]\n");
	    printf("\t-left   - Left outer edge                       ( DEFAULT = '%s' )\n", left);
	    printf("\t-mid    - Separator                             ( DEFAULT = '%s' )\n", mid);
	    printf("\t-space  - Separator                             ( DEFAULT = '%s' )\n", space);
	    printf("\t-right  - Right outer edge                      ( DEFAULT = '%s' )\n", right);
	    printf("\t-prev   - Previous string                       ( DEFAULT = '%s' )\n", prev);
	    printf("\t-next   - Next string                           ( DEFAULT = '%s' )\n", next);
	    printf("\t-parent - Parent string                         ( DEFAULT = '%s' )\n", parent);
	    printf("\t-index  - Index string                          ( DEFAULT = '%s' )\n", indexstr);
	    printf("-parentdoc      - When linking to parent document this URL is used.\n                                                        ( DEFAULT = '%s' )\n", parentdoc);
	    printf("-bottom         - String to put at the bottom of each page.\n                  Default is version and author info.\n");
	    printf("-p              - Use parent link\n");
	    printf("-t              - Use thumbnails for Previous and Next in single-view\n");
	    printf("-n              - Use numeric links to each thumbnail-page ( eg. 1 | 2 | etc )\n");
	}
    }
}

int main( int argc, char *argv[] )
{
    char buffer[512], ifname[64];
    char *fname, *xsize, *ysize, *thumbfname, *thumbxsize, *thumbysize;
    int numpic=0,piccount=0;
    int xcount=0, ycount=0, icount=0;
	struct ThumbData *node=NULL;
	int c;

	left=setstr( left, DEF_LEFT );
	mid=setstr( mid, DEF_MID );
	right=setstr( right, DEF_RIGHT );
	space=setstr( space, DEF_SPACE );
	prev=setstr( prev, DEF_PREV );
	next=setstr( next, DEF_NEXT );
	parent=setstr( parent, DEF_PARENT );
	indexstr=setstr( indexstr, DEF_INDEXSTR );
	parentdoc=setstr( parentdoc, DEF_PARENTDOC );
	BottomString=setstr( BottomString, "<FONT SIZE=\"-1\">Created using GFXIndex v" VERSION " (" OS ") by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></FONT>" );


	while( --argc > 0 )
	{
		argv++[0];
		if( argv[0][0]=='-' )
		{
			argv[0]++;
			if( (argv[0][0]=='h') || (argv[0][0]=='H' ) )
			{
			    intro( 2 );
			    quit( 0 );
			}
			if( !stricmp( argv[0], "left" ) )
			{
				if( argv[1] )
				{
					left=setstr( left, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "mid" ) )
			{
				if( argv[1] )
				{
					mid=setstr( mid, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "right" ) )
			{
				if( argv[1] )
				{
					right=setstr( right, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "space" ) )
			{
				if( argv[1] )
				{
					space=setstr( space, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "prev" ) )
			{
				if( argv[1] )
				{
					prev=setstr( prev, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "next" ) )
			{
				if( argv[1] )
				{
					next=setstr( next, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "parent" ) )
			{
				if( argv[1] )
				{
					parent=setstr( parent, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "index" ) )
			{
				if( argv[1] )
				{
					indexstr=setstr( indexstr, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "parentdoc" ) )
			{
				if( argv[1] )
				{
					parentdoc=setstr( parentdoc, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "bottom" ) )
			{
				if( argv[1] )
				{
					BottomString=setstr( BottomString, argv[1][0]?argv[1]:NULL );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "xstop" ) )
			{
				if( argv[1] )
				{
					xstop=atoi( argv[1] );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "ystop" ) )
			{
				if( argv[1] )
				{
					ystop=atoi( argv[1] );
					argc--;
					argv++[0];
				}
			}
			else if( !stricmp( argv[0], "p" ) )
			{
			    useparent=1;
			}
			else if( !stricmp( argv[0], "t" ) )
			{
			    thumb_prev_next=1;
			}
			else if( !stricmp( argv[0], "n" ) )
			{
			    use_num_links=1;
			}
		}
		else
		{
			PageTitle=setstr( PageTitle, argv[0] );
		}
	}

	if( !( dbfile=fopen( ".thumbnails/files.db", "r" ) ) ) quit( 1 );
	while( fgets( buffer, 511, dbfile ) )
	{
		fname=buffer;
		if( !( xsize=(char *)strchr( fname, ';' ) ) ) quit( 1 );
		xsize[0]='\0';
		xsize++;
		if( !( ysize=(char *)strchr( xsize, ';' ) ) ) quit( 1 );
		ysize[0]='\0';
		ysize++;
		if( !( thumbfname=(char *)strchr( ysize, ';' ) ) ) quit( 1 );
		thumbfname[0]='\0'; 
		thumbfname++;
		if( !( thumbxsize=(char *)strchr( thumbfname, ';' ) ) ) quit( 1 );
		thumbxsize[0]='\0';
		thumbxsize++; 
		if( !( thumbysize=(char *)strchr( thumbxsize, ';' ) ) ) quit( 1 );
		thumbysize[0]='\0'; 
		thumbysize++;
		if( thumbysize[ strlen(thumbysize)-1 ]<' ' ) thumbysize[ strlen(thumbysize)-1 ]='\0';

		/* Allocate a node and link it to the list */
		if( node )
		{
			if( !( node->next=(struct ThumbData *)malloc( sizeof( struct ThumbData ) ) ) ) quit(1);
			node->next->prev=node;
			node=node->next;
		}
		else
		{
			if( !( node=(struct ThumbData *)malloc( sizeof( struct ThumbData ) ) ) ) quit(1);
			if( !thumblist ) thumblist=node;
		}
		if( !( node->image_file=malloc( strlen( fname )+1 ) ) ) quit(1);
		strcpy( node->image_file, fname );
		if( !( node->thumb_file=malloc( strlen( thumbfname )+1 ) ) ) quit( 1 );
		strcpy( node->thumb_file, thumbfname );
		node->image_width=atoi( xsize );
		node->image_height=atoi( ysize );
		node->thumb_width=atoi( thumbxsize );
		node->thumb_height=atoi( thumbysize );
		numpic++;
	}

	close( dbfile );
	dbfile=NULL;

	for( node=thumblist; node; node=node->next )
	{

		if( (ycount+xcount)==0 )
		{
	    	sprintf( ifname, icount?"index%ld.html":"index.html", icount );
	    	printf("Creating %s\n", ifname );
	    	if( !( ifile=fopen( ifname, "w" ) ) ) quit( 1 );
	    	fprintf( ifile, "<HTML>\n <HEAD>\n  <TITLE>%s%s[ %ld / %ld ]</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " (" OS ") by Fredrik Rambris\">\n </HEAD>\n <BODY BGCOLOR=\"#ffffff\">\n  <CENTER>\n", PageTitle?PageTitle:"", PageTitle?" ":"", icount+1, ( numpic/(xstop*ystop) )+1 );
	    	indexline( icount, ( numpic/(xstop*ystop) ) );
			if( IndexString[0] ) fprintf( ifile, "   %s<BR>\n", IndexString );
			fprintf( ifile, "   <TABLE>\n");
		}
		if( xcount==0 ) fprintf( ifile, "    <TR>\n");

		fprintf( ifile, "     <TD ALIGN=\"center\"><A HREF=\"thumbnails/%s.html\"><IMG SRC=\"%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\"><BR><FONT SIZE=\"-1\">%s</FONT></A></TD>\n", node->image_file, node->thumb_file, node->thumb_width, node->thumb_height, node->image_file );
		sprintf( ifname, "thumbnails/%s.html", node->image_file );
		if( !( dbfile=fopen( ifname, "w" ) ) ) quit( 1 );
		fprintf( dbfile, "<HTML>\n <HEAD>\n  <TITLE>%s ( %ld x %ld ) -  [ %ld / %ld ]</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " (" OS ") by Fredrik Rambris\">\n </HEAD>\n <BODY BGCOLOR=\"#ffffff\">\n  <CENTER>\n", node->image_file, node->image_width, node->image_height, piccount+1, numpic );

		ThumbIndex( node, icount );
		fprintf( dbfile, "   %s<BR>", ThumbString );
		fprintf( dbfile, "   <A HREF=\"../%s\"><IMG SRC=\"../%s\" WIDTH=\"%ld\" HEIGHT=\"%ld\" BORDER=\"0\"></A><BR>\n", node->image_file, node->image_file, node->image_width, node->image_height );
		fprintf( dbfile, "   <BR>%s", ThumbString );
		fprintf( dbfile, "  <HR>\n  %s\n  </CENTER>\n </BODY>\n</HTML>", BottomString);
		close( dbfile );
		dbfile=NULL;
		piccount++;
		xcount++;
		if( xcount==xstop )
		{
			fprintf( ifile, "    </TR>\n" );
			xcount=0;
			ycount++;
		}
		if( ycount==ystop )
		{
			fprintf( ifile, "   </TABLE>");
			if( IndexString[0] ) fprintf( ifile, "<BR>\n   %s<BR>\n", IndexString );
			else fprintf( ifile, "\n");
			fprintf( ifile, "   <HR>\n   %s\n  </CENTER>\n </BODY>\n</HTML>", BottomString);

			fclose( ifile );
			ifile=NULL;
			ycount=0;
			icount++;
		}
	}
	if( ifile )
	{
		if( xcount ) fprintf( ifile, "    </TR>\n" );
		fprintf( ifile, "   </TABLE>");
		/* If string is empty don't print it */
		if( IndexString[0] ) fprintf( ifile, "<BR>\n   %s<BR>\n", IndexString );
		else fprintf( ifile, "\n");
		fprintf( ifile, "   <HR>\n   %s\n  </CENTER>\n </BODY>\n</HTML>", BottomString);
	}

	quit( 0 );
	return( 0 );
}
