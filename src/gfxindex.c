/* gfxindex.c - Main sceleton
 * GFXIndex (c) 1999-2001 Fredrik Rambris <fredrik@rambris.com>.
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

#include "global.h"
#include "io_jpeg.h"
#include "gfxio.h"
#include "gfx.h"
#include "args.h"

struct Global *global=NULL;

gint main( int argc, char **argv )
{

	if( gfxio_init() ) error( "Couldn't initiate image I/O" );
	args_init( argc, argv );
	if( !global->quiet ) printf( "GFXIndex v" VERSION " (c) Fredrik Rambris 1999-2001.\n" );
	makethumbs( global->dir, NULL, 0, global );
	cleanup();
	return( 0 );
}

void error( gchar *msg )
{
	if( msg ) fprintf(stderr, "*ERROR* %s\n", msg );
	cleanup();
	global=NULL;
	exit(1);
}

void cleanup( void )
{
	gfxio_cleanup();
	freeconf( global );
}

void makethumbs( gchar *dir, struct ProcessInfo *processinfo, gint level, struct Global *cfg )
{
	struct image *im=NULL, *thumb=NULL;
	DIR *dirhandle;
	gchar *str;
	struct dirent *de;
	struct stat buf;
	GString *path=g_string_new(NULL);
	GString *thumbpath=g_string_new(NULL);
	gboolean slash=(dir[strlen(dir)-1]!='/'), thumbslash=(global->thumbdir[strlen(global->thumbdir)-1]!='/');

	gint w, h, x1, y1, x2, y2;
	gdouble factor;
	GList *thumbs = NULL;
	struct ThumbData *td=NULL;
	struct ProcessInfo *pi;
	gint err=0;
	struct Global *local;
	local=dupconf( cfg );
	g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", global->thumbdir, thumbslash?"/":"", "gfxindex" );
	readconf( thumbpath->str, local );
	g_string_sprintf( thumbpath, "%s%s%s", dir, slash?"/":"", ".gfxindex" );
	readconf( thumbpath->str, local );
	free_empties( local );
	//printf("%s\n", dir );
	//if( processinfo ) processinfo=FALSE;

	if( processinfo ) pi=processinfo;
	else
	{
		struct color col[3]=
		{
			{ 0x00, 0x00, 0x00, 0xff }, /* Black */ 
			{ 0xff, 0xff, 0xff, 0xff }, /* White */
			{ 0x00, 0x00, 0x00, 0xff }  /* Black */
		};

		if( !( pi=g_new0( struct ProcessInfo, 1 ) ) )
		{
			error( "Couldn't allocate memory" );
		}

		memcpy( col, pi->col, sizeof( struct color )*3 );
		pi->up=FALSE;
	}

	if( local->bevel )
	{
		gfx_parsecolor( &pi->col[0], local->bevelback );
		gfx_parsecolor( &pi->col[1], local->bevelbright );
		gfx_parsecolor( &pi->col[2], local->beveldark );
	}
	else
	{
		gfx_parsecolor( &pi->col[0], local->bgcolor );
	}
	slash=(dir[strlen(dir)-1]!='/');
	thumbslash=(local->thumbdir[strlen(local->thumbdir)-1]!='/');
	if( ( dirhandle=opendir( dir ) ) )
	{
		/* First we see to it that the thumbdir exist */
		g_string_sprintf( thumbpath, "%s%s%s", dir, slash?"/":"", local->thumbdir );
		if( !file_exist( thumbpath->str ) ) mkdir( thumbpath->str, 0755 );

		/* If we don't want to overwrite everything. Load the current thumblist */
		g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", "files.db" );
		if( !local->overwrite ) thumbs=loadthumblist( thumbs, thumbpath->str );

		while( ( de=readdir( dirhandle ) ) )
		{
			/* Skip entries with leading dot */
			if( de->d_name[0]!='.' )
			{
				/* Create whole path to the file */
				g_string_sprintf( path, "%s%s%s", dir, slash?"/":"", de->d_name );
				if( stat( path->str, &buf ) ) perror( path->str );
				else
				{
					if( S_ISDIR(buf.st_mode) && local->recursive )
					{
						makethumbs( path->str, pi, level+1, local );
					}
					else if( S_ISREG(buf.st_mode) && local->makethumbs )
					{
						if( checkext( path->str ) )
						{
							g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", de->d_name );

							/* Replace the extension with jpg */
							if( ( str=strrchr( thumbpath->str, '.' ) ) )
							{
								g_string_truncate( thumbpath, str-thumbpath->str+1 );
								g_string_append( thumbpath, "jpg" );
							}

							if( local->overwrite || !file_exist( thumbpath->str ) )
							{
								if( !local->quiet )
								{
									if( pi->up ) printf( "\x1bM\x1b[K" );
									printf( "Creating thumbnail for %s...\n", path->str );
									pi->up=TRUE;
								}
								/* Then we load the original image */
								if( ( im=gfx_load( path->str, &err, TAG_DONE ) ) )
								{
									/* Calculate scaling factor */
									if( im->im_width > im->im_height )
										factor=(gdouble)im->im_width / (gdouble)local->ThumbWidth;
									else
										factor=(gdouble)im->im_height / (gdouble)local->ThumbHeight;

									/* The width and height of the scaled down image */
									w=im->im_width/factor;
									h=im->im_height/factor;

									/* Allocate our workspace */
									thumb=gfx_allocimage( (local->pad?local->ThumbWidth:w), (local->pad?local->ThumbHeight:h), FALSE, NULL );
									if( !thumb ) error( "Couldn't allocate memory" );

									/* Create the actual thumbnail */
									/* First we clear the area */
									gfx_rectfill( thumb, 0, 0, thumb->im_width, thumb->im_height, &pi->col[0] );

									/* Draw outer bevel */
									if( local->bevel )
									{
										gfx_draw( thumb, 0, 0, local->ThumbWidth-2, 0, &pi->col[1] );
										gfx_draw( thumb, 0, 1, 0, local->ThumbHeight-2, &pi->col[1] );
										gfx_draw( thumb, local->ThumbWidth-1, 1, local->ThumbWidth-1, local->ThumbHeight-1, &pi->col[2] );
										gfx_draw( thumb, 0, local->ThumbHeight-1, local->ThumbWidth-2, local->ThumbHeight-1, &pi->col[2] );
									}

									/* Draw inner bevel */
									if( local->bevel )
									{
										w-=8;
										h-=8;
										x1=((local->ThumbWidth-w )/2)-1;
										y1=((local->ThumbHeight-h )/2)-1;
										x2=x1+(w+1);
										y2=y1+(h+1);

										gfx_draw( thumb, x1, y1, x2-1, y1, &pi->col[2] );
										gfx_draw( thumb, x1, y1+1, x1, y2-1, &pi->col[2] );
										gfx_draw( thumb, x2, y1+1, x2, y2, &pi->col[1] );
										gfx_draw( thumb, x1+1,y2, x2-1, y2, &pi->col[1] );
									}
									/* Scale and place the image in the thumbnail */

									if( local->pad )
									{
										gfx_scaleimage( im, 0, 0, im->im_width, im->im_height, thumb, (local->ThumbWidth-w)/2, (local->ThumbHeight-h)/2, w, h, SCALE_NEAREST );
									}
									else
									{
										gfx_scaleimage( im, 0,0, im->im_width, im->im_height, thumb, 0, 0, thumb->im_width, thumb->im_height, SCALE_NEAREST );
									}

									str=thumbpath->str;
									str+=strlen( dir );
									if( slash ) str++;

									if( ( td=findnode( thumbs, de->d_name ) ) )
									{
										td->imagewidth=im->im_width;
										td->imageheight=im->im_height;
										td->thumbwidth=thumb->im_width;
										td->thumbheight=thumb->im_height;
										td->image=setstr( NULL, de->d_name );
										td->thumb=setstr( NULL, str );
									}
									else if( ( td=g_new0( struct ThumbData, 1 ) ) )
									{
										td->imagewidth=im->im_width;
										td->imageheight=im->im_height;
										td->thumbwidth=thumb->im_width;
										td->thumbheight=thumb->im_height;
										td->image=setstr( NULL, de->d_name );
										td->thumb=setstr( NULL, str );
										thumbs=g_list_prepend( thumbs, td );
									} else g_warning( "Couldn't allocate memory for thumbnail-list node. Creating thumbnails anyway." );

									gfx_save( thumbpath->str, thumb, 0,
#ifdef HAVE_LIBJPEG
											GFXIO_JPEG_QUALITY, local->quality,
#endif
											TAG_DONE );

									if( im ) gfx_freeimage( im, FALSE );
									im=NULL;
									if( thumb ) gfx_freeimage( thumb, FALSE );
									thumb=NULL;
								}
								else
								{
									g_warning( "%s: %s", path->str, gfx_errors[err] );
								}
							}
						}
					}
				}
			}
		}
		if( local->makethumbs )
		{
			g_string_sprintf( thumbpath, "%s%s%s%s%s", dir, slash?"/":"", local->thumbdir, thumbslash?"/":"", "files.db" );
			thumbs=g_list_sort( thumbs, (GCompareFunc)thumbcomp );
			savethumblist( thumbs, thumbpath->str );
		}
		closedir( dirhandle );
		if( local->genindex ) gfxindex( local, dir, thumbs, level );
	}
	else perror( "opendir" );
	if( path ) g_string_free( path, TRUE );
	if( thumbpath ) g_string_free( thumbpath, TRUE );
	g_list_foreach( thumbs, freenode, NULL );
	g_list_free( thumbs );
	if( !processinfo )
	{
		g_free( pi );
	}
	freeconf( local );
	local=NULL;
}

void gfxindex( struct Global *local, gchar *dir, GList *thumbs, gint level )
{
	guint numpics=g_list_length( thumbs );
	guint ppp=local->xstop * local->ystop;
	guint xcount=0, ycount=0, page=0, count;
	GString *path=g_string_new(NULL);
	GString *index=g_string_new(NULL);
	GString *thumbindex=g_string_new(NULL);
	gboolean slash=(dir[strlen(dir)-1]!='/');
	//, thumbslash=(local->thumbdir[strlen(local->thumbdir)-1]!='/');
	guint numpages=numpics/ppp;
	FILE *file=NULL, *thumbfile=NULL;
	struct ThumbData *td;
	gchar *tmpstr, *strptr;
	GList *node;
	char space[32]="";
	int hspace, vspace;
	if( (gfloat)numpics/(gfloat)ppp > numpages ) numpages++;
	for( node=thumbs ; node ; node=node->next )
	{
		td=(struct ThumbData *)node->data;
		g_string_sprintf( path, "%s%s%s.html", dir, slash?"/":"", td->thumb );
		if( !( thumbfile=fopen( path->str, "w" ) ) ) goto error;
		g_string_sprintf( path, "../%s", indexstr( page ) );
		fprintf( thumbfile, "<HTML>\n <HEAD>\n  <TITLE>%s%s%s ( %d x %d )</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " by Fredrik Rambris (fredrik@rambris.com)\">\n", local->title?local->title:"", local->title?" - ":"", td->image, td->imagewidth, td->imageheight );
		if( local->css )
		{
			fprintf( thumbfile, "  <LINK REL=\"stylesheet\" HREF=\"" );
			for( count=0 ; count<=level ; count ++ ) fprintf( thumbfile, "../" );
			fprintf( thumbfile, "%s\" TYPE=\"text/css\">\n", local->css );
		}
		fprintf( thumbfile, " </HEAD>\n <BODY%s%s>\n  <DIV ALIGN=\"center\">\n", local->bodyargs?" ":"", local->bodyargs?local->bodyargs:"" );
		if( numpics>1 ) navbar_new( thumbindex );
		if( node->prev && ( local->previous || local->thumbscale ) )
		{
			if( !( tmpstr=strrchr( ((struct ThumbData *)node->prev->data)->thumb, '/' ) ) ) tmpstr=((struct ThumbData *)node->prev->data)->thumb;
			else tmpstr++;

			if( local->thumbscale )
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" ALT=\"%s\" BORDER=\"0\"></A>", tmpstr, tmpstr, (long)(((struct ThumbData *)node->prev->data)->thumbwidth*local->thumbscale/100), (long)(((struct ThumbData *)node->prev->data)->thumbheight*local->thumbscale/100), local->previous );
			}
			else
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, local->previous );
			}
		}
		if( local->index ) navbar_add( local, thumbindex, "<A HREF=\"%s\">%s</A>", path->str, local->index );
		if( node->next && ( local->next || local->thumbscale ) )
		{
			if( !( tmpstr=strrchr( ((struct ThumbData *)node->next->data)->thumb, '/' ) ) ) tmpstr=((struct ThumbData *)node->next->data)->thumb;
			else tmpstr++;

			if( local->thumbscale )
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" ALT=\"%s\" BORDER=\"0\"></A>", tmpstr, tmpstr, (long)(((struct ThumbData *)node->next->data)->thumbwidth*local->thumbscale/100), (long)(((struct ThumbData *)node->next->data)->thumbheight*local->thumbscale/100), local->next );
			}
			else
			{
				navbar_add( local, thumbindex, "<A HREF=\"%s.html\">%s</A>", tmpstr, local->next );
			}
		}
		if( numpics>1 ) navbar_end( local, thumbindex );
		if( numpics>1 ) fprintf( thumbfile, "   <SPAN CLASS=\"navbar\">%s</SPAN><BR>\n", thumbindex->str );
		fprintf( thumbfile, "   <A HREF=\"%s\"><IMG SRC=\"../%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\" VSPACE=\"2\"></A><BR>\n", path->str, td->image, td->imagewidth, td->imageheight );
		if( numpics>1 ) fprintf( thumbfile, "   <SPAN CLASS=\"navbar\">%s</SPAN>\n", thumbindex->str );
		if( local->showcredits ) fprintf( thumbfile, "   <HR>\n   <SPAN CLASS=\"credits\">Created using <A HREF=\"http://fredrik.rambris.com/gfxindex/\">GFXIndex</A> v" VERSION " by <A HREF=\"mailto:fredrik@rambris.com\">Fredrik Rambris</A></SPAN>\n" );
		fprintf( thumbfile, "  </DIV>\n </BODY>\n</HTML>\n" );
		fclose( thumbfile );
		thumbfile=NULL;
		if( ycount+xcount==0 )
		{
			ycount=0;
			xcount=0;
			g_string_sprintf( path, "%s%s%s", dir, slash?"/":"", indexstr( page ) );
			if( !( file=fopen( path->str, "w" ) ) ) goto error;
			fprintf( file, "<HTML>\n <HEAD>\n  <TITLE>%s%sPage %d / %d</TITLE>\n  <META NAME=\"generator\" CONTENT=\"GFXIndex v" VERSION " by Fredrik Rambris (fredrik@rambris.com)\">\n", local->title?local->title:"", local->title?" - ":"", page+1, numpages );
			if( local->css )
			{
				fprintf( file, "  <LINK REL=\"stylesheet\" HREF=\"" );
				for( count=0 ; count<level ; count ++ ) fprintf( file, "../" );
				fprintf( file, "%s\" TYPE=\"text/css\">\n", local->css );
			}
			fprintf( file, " </HEAD>\n <BODY%s%s>\n  <DIV ALIGN=\"center\">\n",  local->bodyargs?" ":"", local->bodyargs?local->bodyargs:"" );
			g_string_truncate( index, 0 );
			if( numpages>1 || (local->parent && local->parentdoc) ) navbar_new( index );
			if( numpages>1 )
			{
				if( page>0 && local->previous ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page-1 ), local->previous );
			}
			if( local->parent && local->parentdoc ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", local->parentdoc, local->parent );
			if( numpages>1 )
			{	
				if( local->numlink )
				{
					for( count=0 ; count<numpages ; count++ )
					{
						if( count!=page ) navbar_add( local, index, "<A HREF=\"%s\">", indexstr( count ) );
						else navbar_add( local, index, "<SPAN CLASS=\"current\">" );
						g_string_sprintfa( index, "%d", count+1 );
						if( count!=page ) g_string_sprintfa( index, "</A>" );
						else g_string_sprintfa( index, "</SPAN>" );
					}
				}
				if( page<(numpages-1) && local->next ) navbar_add( local, index, "<A HREF=\"%s\">%s</A>", indexstr( page+1 ), local->next );
			}
			if( strlen( index->str ) )
			{
				navbar_end( local, index );
				fprintf( file, "   <SPAN CLASS=\"navbar\">%s</SPAN><BR>\n", index->str );
			}
											
			fprintf( file, "   <TABLE BORDER=\"0\" CELLPADDING=\"2\" CELLSPACING=\"0\">\n" );
		}
		if( xcount==0 ) fprintf( file, "     <TR>\n" );
		space[0]='\0';;
		if( !local->pad )
		{
				hspace=0;
				vspace=0;
				if( td->thumbwidth<local->ThumbWidth )
				{
					hspace=(local->ThumbWidth-td->thumbwidth)/2;
				}
				if( td->thumbheight<local->ThumbHeight)
				{
					vspace=(local->ThumbHeight-td->thumbheight)/2;
				}
				if( hspace ) sprintf( space, " HSPACE=\"%d\"", hspace );
				if( vspace ) sprintf( space+strlen( space ), " VSPACE=\"%d\"", vspace );
		}
		tmpstr=g_strdup( td->image );
		if( local->hideext )
		{
			strptr=strchr( tmpstr, '.' );
			strptr[0]='\0';
		}
		fprintf( file, "     <TD%s%s><A HREF=\"%s.html\"><IMG SRC=\"%s\" WIDTH=\"%d\" HEIGHT=\"%d\" BORDER=\"0\"%s>%s%s</A></TD>\n", local->cellargs?" ":"", local->cellargs?local->cellargs:"", td->thumb, td->thumb, td->thumbwidth, td->thumbheight, space, local->titles?"<BR>":"", local->titles?tmpstr:"" );
		g_free( tmpstr );
		xcount++;
		if( xcount>=local->xstop )
		{
			xcount=0;
			ycount++;
			fprintf( file, "    </TR>\n" );
		}
		if( ycount>=local->ystop )
		{
			ycount=0;
			page++;
			fprintf( file, "   </TABLE>\n" );
			if( strlen( index->str ) ) fprintf( file, "    <SPAN CLASS=\"navbar\">%s</SPAN>\n", index->str ); 
			if( local->showcredits ) fprintf( file, "   <HR>\n   <SPAN CLASS=\"credits\">Created using <A HREF=\"http://fredrik.rambris.com/gfxindex/\" TARGET=\"_blank\">GFXIndex</A> v" VERSION " by Fredrik Rambris</SPAN>\n" );
			fprintf( file, "  </DIV>\n </BODY>\n</HTML>\n" );
			fclose( file );
			file=NULL;
		}
	}
	if( file )
	{
		fprintf( file, "    </TR>\n   </TABLE>\n" );
		if( strlen( index->str ) ) fprintf( file, "    <SPAN CLASS=\"navbar\">%s</SPAN>\n", index->str ); 
		if( local->showcredits ) fprintf( file, "   <HR>\n   <SPAN CLASS=\"credits\">Created using <A HREF=\"http://fredrik.rambris.com/gfxindex/\" TARGET=\"_blank\">GFXIndex</A> v" VERSION " by Fredrik Rambris</SPAN>\n" );
		fprintf( file, "  </DIV>\n </BODY>\n</HTML>\n" );
		fclose( file );
		file=NULL;
	}
	error:
	if( file ) fclose( file );
	if( thumbfile ) fclose( thumbfile );
	g_string_free( path, TRUE );
	g_string_free( index, TRUE );
	g_string_free( thumbindex, TRUE );
}

gchar *indexstr( int number )
{
	static gchar buf[20];
	if( number>0 ) sprintf( buf, "index%d.html", number );
	else sprintf( buf, "index.html" );
	return( buf );
}

void navbar_new( GString *str )
{
	g_string_truncate( str, 0 );
}

void navbar_add( struct Global *local, GString *str, gchar *newstr, ... )
{
	va_list ap;
	gchar buf[512];
	va_start( ap, newstr );
	vsnprintf( buf, 511, newstr, ap );
	if( str->str[0] ) { if( local->space ) g_string_append( str, local->space ); if( local->divider ) g_string_append( str, local->divider ); }
	else { if( local->left ) g_string_append( str, local->left ); }
	if( local->space ) g_string_append( str, local->space );
	g_string_append( str, buf );
	va_end( ap );
}

void navbar_end( struct Global *local, GString *str )
{
	if( local->space ) g_string_append( str, local->space );
	if( local->right ) g_string_append( str, local->right );
}

gboolean checkext( gchar *file )
{
	gchar *str;
	/* Extract the extension */
	if( !( str=strrchr( file, '.') ) ) return( FALSE );
	str++;

	/* These are hardcoded for now... will be replaced by some smarter thing later on */
	if( !strcasecmp( str, "jpg") ) return( TRUE );
	if( !strcasecmp( str, "jpeg") ) return( TRUE );
	if( !strcasecmp( str, "gif") ) return( TRUE );
	if( !strcasecmp( str, "png") ) return( TRUE );
	/* I've commented these out because webbrowsers generally doesn't support them anyway */
	//if( !strcasecmp( str, "tif") ) return( TRUE );
	//if( !strcasecmp( str, "xpm") ) return( TRUE );
	//if( !strcasecmp( str, "tga") ) return( TRUE );

	return( FALSE );
}

gint dircomp( const struct dirent **a, const struct dirent **b )
{
	return( g_strcasecmp( b[0]->d_name, a[0]->d_name ) );
}

gint thumbcomp( gpointer a, gpointer b )
{
	struct ThumbData *tda=a, *tdb=b;
	return( g_strcasecmp( tda->image, tdb->image ) );
}

void savethumblist( GList *thumbs, gchar *file )
{
	FILE *fp;
	struct ThumbData *td;
	if( !thumbs || !file ) return;
	if( ( fp=fopen( file, "w+" ) ) )
	{
		for( ; thumbs ; thumbs=thumbs->next )
		{
			td=(struct ThumbData *)thumbs->data;
			fprintf( fp, "%s;%d;%d;%s;%d;%d%s%s\n", td->image, td->imagewidth, td->imageheight, td->thumb, td->thumbwidth, td->thumbheight, td->extra?";":"", td->extra?td->extra:"" );
		}
		fclose( fp );
	}
}

GList *loadthumblist( GList *thumbs, gchar *file )
{
	FILE *fp;
	struct ThumbData *td;
	static gchar buf[1024];
	gchar **line;
	if( !file ) return( thumbs );
	if( !file_exist( file ) ) return( thumbs );
	if( ( fp=fopen( file, "r" ) ) )
	{
		while( fgets( buf, 1023, fp ) )
		{
			g_strstrip( buf );
			if( ( line=g_strsplit( buf, ";", 6 ) ) )
			{
				if( ( td=g_new0( struct ThumbData, 1 ) ) )
				{
					td->image=setstr( NULL, line[0] );
					td->thumb=setstr( NULL, line[3] );
					td->imagewidth=atoi( line[1] );
					td->imageheight=atoi( line[2] );
					td->thumbwidth=atoi( line[4] );
					td->thumbheight=atoi( line[5] );
					td->extra=setstr( NULL, line[6] );
					thumbs=g_list_prepend( thumbs, td );
				} else g_warning( "Couldn't allocate memory for node" );
				g_strfreev( line );
			} else g_warning( "Couldn't split up line" );
		}
		thumbs=g_list_sort( thumbs, (GCompareFunc)thumbcomp );
	}
	else g_warning( "Couldn't load thumbnail list" );
	return( thumbs );
}

void freenode( gpointer data, gpointer user_data )
{
	struct ThumbData *td=data;
	td->image=setstr( td->image, NULL );
	td->thumb=setstr( td->thumb, NULL );
	g_free( td );
}

gboolean fastcompare( gchar *a, gchar *b )
{
	for( ; a[0] && b[0] ; a++, b++ )
	{
		if( a[0]!=b[0] ) return( FALSE );
	}
	if( a[0]!=b[0] ) return( FALSE );
	return( TRUE );
}

GList *removenode( GList *thumbs, gchar *imagename )
{
	GList *node, *next;
	struct ThumbData *td;
	for( node=thumbs; node ; node=next )
	{
		next=node->next;
		td=(struct ThumbData *)node->data;
		if( fastcompare( td->image, imagename ) )
		{
			thumbs=g_list_remove_link( thumbs, node );
			freenode( node->data, NULL );
			g_list_free_1( node );
		}
	}
	return( thumbs );
}

struct ThumbData *findnode( GList *thumbs, gchar *imagename )
{
	GList *node;
	struct ThumbData *td;
	for( node=thumbs; node ; node=node->next )
	{
		td=(struct ThumbData *)node->data;
		if( fastcompare( td->image, imagename ) )
		{
			return( td );
		}
	}
	return( NULL );
}

gboolean file_exist( gchar *filename )
{
	struct stat buf;
	return( stat( filename, &buf )==0 );
}
