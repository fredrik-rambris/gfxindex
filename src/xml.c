#if HAVE_CONFIG_H
#include <config.h>
#endif

char *utf8toiso( char *str );
#ifdef HAVE_LIBEXPAT

#include <string.h>
#include <stdio.h>
#include "thumbdata.h"
#include "preferences.h"
#include "util.h"
#include "xml.h"
#include <expat.h>

/*
void addPictureNode( xmlNodePtr parent, struct Picture *p )
{
	xmlNodePtr n;
	char buf[32];
	if( p->p_path )
	{
		n=xmlNewChild( parent, NULL, "Picture", NULL );
		xmlNewProp( n, "path", p->p_path );
		sprintf( buf, "%d", p->p_width );
		xmlNewProp(n, "width", buf );
		sprintf( buf, "%d", p->p_height );
		xmlNewProp(n, "height", buf );
	}
}

void writeThumbData( ConfArg *cfg, List *thumbdata, char *file )
{
	Node *node;
	struct PictureNode *pn;
	xmlDocPtr doc;
	xmlNodePtr rootnode,xpn,xn;
	struct Picture **p;
	doc = xmlNewDoc ("1.0");
	rootnode = xmlNewDocNode(doc, NULL, (const xmlChar*)"GFXindex", (const xmlChar*)NULL);
	xmlNewProp( rootnode, "version", VERSION );
	xmlDocSetRootElement(doc, rootnode);
	for( node=thumbdata->head; node; node=node->next )
	{
		pn=(struct PictureNode *)node;
		xpn=xmlNewChild( rootnode, NULL, "PictureNode", NULL );
		if( pn->pn_title ) xmlNewChild( xpn, NULL, "Title", pn->pn_title );
		if( pn->pn_caption ) xmlNewChild( xpn, NULL, "Caption", pn->pn_caption );
		if( pn->pn_original.p_path )
		{
			xn=xmlNewChild( xpn, NULL, "Original", NULL );
			addPictureNode( xn, &(pn->pn_original) );
		}
		if( pn->pn_pictures )
		{
			xn=xmlNewChild( xpn, NULL, "Bigs", NULL );
			p=pn->pn_pictures;
			while( *p )
			{
				if( (*p)->p_path )
				{
					addPictureNode( xn, *p );
				}
				p++;
			}
		}
		if( pn->pn_thumbnail.p_path )
		{
			xn=xmlNewChild( xpn, NULL, "Thumbnail", NULL );
			addPictureNode( xn, &(pn->pn_thumbnail) );
		}
		if( pn->pn_dir )
		{
			xn=xmlNewChild( xpn, NULL, "Dir", NULL );
			xmlNewProp( xn, "path", pn->pn_dir );
		}

	}
	xmlSaveFormatFile(file, doc, 1);
	xmlFreeDoc( doc );
}
*/

struct ReaderState
{
	List *thumbdata;
	struct PictureNode *pn;
	char buf[4096];
	char done, collectall;
	ConfArg *cfg;
};

enum {
	E_NONE=0,
	E_ALBUM,
	E_A_TITLE,
	E_A_CAPTION,
	E_PICTURE,
	E_P_TITLE,
	E_P_CAPTION,
	E_UNKNOWN
};

enum {
	XML_ALBUM=0,
	XML_PICTURE,
	XML_CAPTION,
	XML_TITLE,
	XML_PATH,
	XML_SKIP,
	XML_ROTATE
};

/* This saves a few bytes of executalbe size but lessens the possibility of typos */
char *tags[]={
	"album",
	"picture",
	"caption",
	"title",
	"path",
	"skip",
	"rotate"
};

static int stack[40]; /* Our stack */
static int sp; /* Stack pointer */

#define PUSH(e) stack[++sp]=e
#define POP stack[sp--]


static void data( void *userData, const XML_Char *s, int len )
{
	struct ReaderState *rs=(struct ReaderState *)userData;
	switch( stack[sp] )
	{
		case E_A_TITLE:
		case E_A_CAPTION:
		case E_P_TITLE:
		case E_P_CAPTION:
			if( rs->buf[0] && s[0]!=' ' ) strmaxcat( rs->buf, "\n", 4096 );
			strnmaxcat( rs->buf, s, len, 4096 );
			break;
		default:
			if( rs->collectall )
			{
//				if( rs->buf[0]  ) strmaxcat( rs->buf, " ", 4096 );
				strnmaxcat( rs->buf, s, len, 4096 );
			}
			break;
	}
}

static void start_element(void *userData, const char *name, const char **atts)
{
	struct ReaderState *rs=(struct ReaderState *)userData;
//	char utfbuf[1024];
//	int c;
	if( sp==40 )
	{
		fprintf( stderr, "*** Stack overflow (nothing I can't handle)\n" );
		return;
	}

	if( rs->collectall )
	{
		strmaxcat( rs->buf, "<", 4096 );
		strmaxcat( rs->buf, name, 4096 );
		if( atts )
		{
			while( *atts )
			{
				strmaxcat( rs->buf, " ", 4096 );
				strmaxcat( rs->buf, atts[0], 4096 );
				strmaxcat( rs->buf, "=\"", 4096 );
				strmaxcat( rs->buf, atts[1], 4096 );
				strmaxcat( rs->buf, "\"", 4096 );
				atts+=2;
			}
		}
		strmaxcat( rs->buf, ">", 4096 );
		PUSH(E_UNKNOWN);
	}
	else
	{
		switch( stack[sp] )
		{
			case E_ALBUM:
				if( fastcasecompare( name, tags[XML_TITLE] ) )
				{
					PUSH(E_A_TITLE);
					rs->buf[0]='\0';
					rs->collectall=TRUE;
				}
				else if( fastcasecompare( name, tags[XML_CAPTION] ) )
				{
					PUSH(E_A_CAPTION);
					rs->buf[0]='\0';
					rs->collectall=TRUE;
				}
				else if( fastcasecompare( name, tags[XML_PICTURE] ) )
				{
					PUSH(E_PICTURE);
					if( ( rs->pn=gfx_new0( struct PictureNode, 1 ) ) )
					{
						while( *atts )
						{
							if( fastcasecompare( atts[0], tags[XML_PATH] ) ) rs->pn->pn_original.p_path=setstr( rs->pn->pn_original.p_path, utf8toiso((char *)atts[1]) );
							else if( fastcasecompare( atts[0], tags[XML_SKIP] ) ) rs->pn->pn_skip=strtobool( atts[1] );
							else if( fastcasecompare( atts[0], tags[XML_ROTATE] ) )
							{
								rs->pn->pn_rotate=atoi( atts[1] );
								rs->pn->pn_rotate-=rs->pn->pn_rotate % 90;
							}
							atts+=2;
						}
					}
				}
				else PUSH(E_UNKNOWN);
				break;
			case E_PICTURE:
				if( fastcasecompare( name, tags[XML_TITLE] ) )
				{
					PUSH(E_P_TITLE);
					rs->buf[0]='\0';
					rs->collectall=TRUE;
				}
				else if( fastcasecompare( name, tags[XML_CAPTION] ) )
				{
					PUSH(E_P_CAPTION);
					rs->buf[0]='\0';
					rs->collectall=TRUE;
				}
				else PUSH(E_UNKNOWN);
				break;
			case E_A_TITLE:
			case E_A_CAPTION:
			case E_P_TITLE:
			case E_P_CAPTION:
				PUSH(E_UNKNOWN);
				break;
			case E_NONE:
				if( fastcasecompare( name, tags[XML_ALBUM] ) )
				{
					if( !rs->thumbdata )
					{
						rs->thumbdata=thumbdata_new(NULL);
						PUSH(E_ALBUM);
					}
					else
					{
						rs->done=1;
						PUSH(E_UNKNOWN);
					}
				}
				else PUSH(E_UNKNOWN);
				break;
			default:
				PUSH(E_UNKNOWN);
				break;
		}
	}
}

static void end_element(void *userData, const char *name)
{
	struct ReaderState *rs=(struct ReaderState *)userData;
	int element;
	if( !sp )
	{
		fprintf( stderr, "*** Stack empty (nothing I can't handle)\n" );
		return;
	}
	element=POP;
	switch( element )
	{
		case E_ALBUM:
/*
			print_thumbdata( rs->thumbdata );
			list_free( rs->thumbdata, TRUE );
			rs->thumbdata=NULL;
*/
			break;
		case E_PICTURE:
			list_append( rs->thumbdata, (Node *)(rs->pn) );
//			if( rs->pn ) free( rs->pn );
			rs->pn=NULL;
			break;
		case E_A_TITLE:
			SCONF(rs->cfg,PREFS_TITLE)=setstr( SCONF(rs->cfg,PREFS_TITLE), utf8toiso(rs->buf) );
			rs->collectall=FALSE;
			break;
		case E_A_CAPTION:
			SCONF(rs->cfg,PREFS_CAPTION)=setstr( SCONF(rs->cfg,PREFS_CAPTION), utf8toiso(rs->buf) );
			rs->collectall=FALSE;
			break;
		case E_P_TITLE:
			rs->pn->pn_title=setstr( rs->pn->pn_title, utf8toiso(rs->buf) );
			rs->collectall=FALSE;
			break;
		case E_P_CAPTION:
			rs->pn->pn_caption=setstr( rs->pn->pn_caption, utf8toiso(rs->buf) );
			rs->collectall=FALSE;
			break;
		default:
			if( rs->collectall )
			{
				strmaxcat( rs->buf, "</", 4096 );
				strmaxcat( rs->buf, name, 4096 );
				strmaxcat( rs->buf, ">", 4096 );
			}
			break;
	}
}

#define BUFSIZE 4096

List *readAlbum( char *file, ConfArg *cfg )
{
	size_t len;
	FILE *fp;
	char buf[BUFSIZE];
	XML_Parser parser;
	struct ReaderState rs;
	memset( (void *)stack, '\0', sizeof( stack ) );
	memset( (void *)&rs, '\0', sizeof( struct ReaderState ) );
	rs.cfg=cfg;
	sp=0;
	if( ( fp=fopen( file, "r" ) ) )
	{
		if( ( parser=XML_ParserCreate( NULL ) ) )
		{
			XML_SetElementHandler( parser, start_element, end_element );
			XML_SetCharacterDataHandler( parser, data );
			XML_SetUserData( parser, (void *)&rs );
			rs.done=0;
			while( !rs.done )
			{
				if( !fgets( buf, BUFSIZE, fp ) ) rs.done=1;
				if( !rs.done )
				{
					if( feof( fp ) ) rs.done=1;
				}
				else break;
				stripws( buf );
				len=strlen( buf );
				if( !rs.done && !len ) continue;
			    if (XML_Parse(parser, buf, len, rs.done) == XML_STATUS_ERROR) {
			      fprintf(stderr,
			              "%s in %s at line %d\n",
			              XML_ErrorString(XML_GetErrorCode(parser)),
						  file,
			              XML_GetCurrentLineNumber(parser));
				  break;
			    }
			}
			XML_ParserFree( parser );
			if( rs.done )
			{
				return rs.thumbdata;
			}
			else
			{
				if( rs.thumbdata ) list_free( rs.thumbdata, TRUE );
			}
		}
		fclose( fp );
	}
	return NULL;
}

#endif
/* These functions doesn't use libexpat */

void writeThumbData( ConfArg *cfg, List *thumbdata, char *file )
{
	struct PictureNode *pn;
	struct Picture **p;
	FILE *fp;
	if( !thumbdata || !thumbdata->head || !cfg || !STR_ISSET( file ) ) return;
	
	if( ( fp=fopen( file, "w" ) ) )
	{
		fprintf( fp, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n" );
		fprintf( fp, "<ThumbData>\n" );
		if( STR_ISSET( SCONF(cfg,PREFS_TITLE) ) ) fprintf( fp, " <Title>%s</Title>\n", SCONF(cfg,PREFS_TITLE) );
		if( STR_ISSET( SCONF(cfg,PREFS_CAPTION) ) ) fprintf( fp, " <Caption>%s</Caption>\n", SCONF(cfg,PREFS_CAPTION) );
		for( pn=(struct PictureNode *)(thumbdata->head); pn; pn=(struct PictureNode *)(pn->node.next) )
		{
			if( !pn->pn_skip )
			{
			fprintf( fp, " <PictureNode>\n" );
			if( STR_ISSET( pn->pn_title ) ) fprintf( fp, "  <Title>%s</Title>\n", pn->pn_title );
			if( STR_ISSET( pn->pn_caption ) ) fprintf( fp, "  <Caption>%s</Caption>\n", pn->pn_caption );
			if( STR_ISSET( pn->pn_original.p_path ) && ( BCONF(cfg,PREFS_COPY) || !STR_ISSET( SCONF(cfg,PREFS_OUTDIR) ) ) )
			{
				fprintf( fp, "  <Original>\n" );
				fprintf( fp, "   <Picture path=\"%s\" width=\"%d\" height=\"%d\"/>\n", pn->pn_original.p_path, pn->pn_original.p_width, pn->pn_original.p_height );
				fprintf( fp, "  </Original>\n" );
			}
			if( STR_ISSET( pn->pn_dir ) ) fprintf( fp, "  <Dir path=\"%s\"/>\n", pn->pn_dir );
			if( STR_ISSET( pn->pn_thumbnail.p_path ) )
			{
				fprintf( fp, "  <Thumbnail>\n" );
				fprintf( fp, "   <Picture path=\"%s\" width=\"%d\" height=\"%d\"/>\n", pn->pn_thumbnail.p_path, pn->pn_thumbnail.p_width, pn->pn_thumbnail.p_height );
				fprintf( fp, "  </Thumbnail>\n" );
			}
			if( pn->pn_pictures )
			{
				fprintf( fp, "  <Bigs>\n" );
				p=pn->pn_pictures;
				while( *p )
				{
					fprintf( fp, "   <Picture path=\"%s\" width=\"%d\" height=\"%d\"/>\n", (*p)->p_path, (*p)->p_width, (*p)->p_height );
					p++;
				}
				fprintf( fp, "  </Bigs>\n" );
			}
			fprintf( fp, " </PictureNode>\n" );
			}
		}
		fprintf( fp, "</ThumbData>\n" );
		fclose( fp );
	}
	else
	{
		fprintf( stderr, "*** Couldn't open file '%s'\n", file );
	}
}

void writeAlbum( ConfArg *cfg, List *thumbdata, char *path )
{
	struct PictureNode *pn;
	char *file;
	FILE *fp;
	printf( "cfg: %s, thumbdata: %s, thumbdata->head: %s, path:'%s'\n", thumbdata?"true":"false", thumbdata->head?"true":"false", cfg?"true":"false", path );
	if( !thumbdata || !thumbdata->head || !cfg || !path ) return;
	printf( "alocation\n" );
	if( !( file=gfx_new( char, (strlen( path )+12) ) ) ) return;
	strcpy( file, path );
	tackon( file, "album.xml" );
	printf( "file: '%s'\n", file );
	if( ( fp=fopen( file, "w" ) ) )
	{
		fprintf( fp, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n" );
		fprintf( fp, "<Album>\n" );
		if( STR_ISSET( SCONF(cfg,PREFS_TITLE) ) ) fprintf( fp, " <Title>%s</Title>\n", SCONF(cfg,PREFS_TITLE) );
		if( STR_ISSET( SCONF(cfg,PREFS_CAPTION) ) ) fprintf( fp, " <Caption>%s</Caption>\n", SCONF(cfg,PREFS_CAPTION) );
		for( pn=(struct PictureNode *)(thumbdata->head); pn; pn=(struct PictureNode *)(pn->node.next) )
		{
			if( !pn->pn_dir )
			{
				fprintf( fp, " <Picture path=\"%s\">\n", pn->pn_original.p_path );
				fprintf( fp, "  <Title>%s</Title>\n", pn->pn_title?pn->pn_title:"" );
				fprintf( fp, "  <Caption>%s</Caption>\n", pn->pn_caption?pn->pn_caption:"" );
				fprintf( fp, " </Picture>\n" );

			}
		}
		fprintf( fp, "</Album>\n" );
		fclose( fp );
	}
	else
	{
		fprintf( stderr, "*** Couldn't open file '%s'\n", file );
	}
}

char *utf8toiso( char *str )
{
	char *i, *o;
	short c;
	if( !str ) return NULL;
	o=i=str;
	while( i[0] )
	{
		if( ((i[0] & 224)==192) && ((i[1]&192)==128) )
		{
//			c=((short)(i[0] & 31) << 6) + (short)(i[1] & 95);
			c=((short)(i[0] & 63) << 6) + (short)(i[1] & 63);
			o[0]=c;
			i++;
		}
		else o[0]=i[0];
		i++;
		o++;
	}
	o[0]='\0';
	return str;
}
