#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include <clib/exec_protos.h>
#include <exec/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
				if( dir_name ) sprintf(TempStr,"c:gfxcon \"%s%s\" TO \"%s/%s\" FORMAT JPEG BOXFITALL %s %s CENTERBOX %s %s 255 255 255 QUALITY 50 NOPROGRESS >NIL:", dir_name ,node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize,ThumbSize,ThumbSize);
				else sprintf(TempStr,"c:gfxcon \"%s\" TO \"%s/%s\" FORMAT JPEG BOXFITALL %s %s CENTERBOX %s %s 255 255 255 QUALITY 50 NOPROGRESS >NIL:",node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize,ThumbSize,ThumbSize);
*/

void *DisplayNameList(struct List *list,UWORD code);
void remlist( struct List *list );
void *addlist( struct List *list, void *buffer, ULONG length );
void SortList( struct List *list );
void printlist( struct List *list );
BOOL getnewdir( char *dirstr );
BOOL getdir( BPTR lock );
BOOL Exists( STRPTR file );

struct List Dir;

ULONG sigmask=SIGBREAKF_CTRL_C;

#define _VERS_ "1.5"

char VerStr[]="\0$VER: GFXIndex " _VERS_ " (" __DATE2__ ")";

void main( void )
{
	struct Node *node;
	char ThumbDir[]="thumbnails", ThumbSize[]="80";
	static char TempStr[1024];
	char *title=NULL,*dir_name=NULL, *xsize, *ysize, *comment;
	UWORD xcount=0, ycount=0, numpic=0, icount=0, xstop=7, ystop=3;
	int XSize, YSize;
	UWORD numpage,tempcount, stage;
	BPTR ofile=NULL, htmlfile=NULL;
	BOOL error=FALSE;
	char Template[]="TITLE/K,DIR/K,NOHTML/S";
	struct	RDArgs	*MyRDArgs;
	long Args[3]={0,0,0};

	Printf("GFXIndex v" _VERS_ " © 1998-1999 Fredrik Rambris. All rights reserved.\n");

	if((MyRDArgs = ReadArgs(Template, Args, NULL)))
	{
		if( Args[0] )
		{
			if( title=AllocVec( strlen( (char *)Args[0] )+2, MEMF_CLEAR ) )
			{
				strcpy( title, (char *)Args[0] );
			}
		}

		if( Args[1] )
		{
			if( dir_name=AllocVec( strlen( (char *)Args[1] )+2, MEMF_CLEAR ) )
			{
				strcpy( dir_name, (char *)Args[1] );
			}
		}

		FreeArgs(MyRDArgs);
	}


	SetSignal( 0L, 0L );
	NewList( &Dir );
	printf("Getting directory listing...\n");
	if( dir_name )
	{
		if( !getnewdir( dir_name ) ) goto slut;
	}
	else
	{
		if( !getnewdir( "" ) ) goto slut;
	}

	if( !Exists( ThumbDir ) ) CreateDir(ThumbDir);

	if (Dir.lh_TailPred != (struct Node *)&Dir)
	{

		for (node = Dir.lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
			if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				printf(" *** BREAK\n");
				goto slut;
			}
			sprintf(TempStr,"%s/%s",ThumbDir,node->ln_Name);
			if( !Exists(TempStr) )
			{
				if( dir_name ) sprintf(TempStr,"c:gfxcon \"%s%s\" TO \"%s/%s\" FORMAT JPEG BOXFITALL %s %s CENTERBOX %s %s 255 255 255 QUALITY 50 NOPROGRESS >NIL:", dir_name ,node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize,ThumbSize,ThumbSize);
				else sprintf(TempStr,"c:gfxcon \"%s\" TO \"%s/%s\" FORMAT JPEG BOXFITALL %s %s CENTERBOX %s %s 255 255 255 QUALITY 50 NOPROGRESS >NIL:",node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize,ThumbSize,ThumbSize);

				printf("Creating thumbnail for '%s'...\n",node->ln_Name);
				Execute( TempStr, NULL, NULL );
			}
			numpic++;
		}
		printf("\n");
		numpage=((LONG)numpic / ((LONG)xstop*(LONG)ystop))+1;

		for (node = Dir.lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
			if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				printf(" *** BREAK\n");
				goto slut;
			}

			comment=node->ln_Name;
			comment+=strlen( comment )+1;
			if( !strlen(comment) ) comment=NULL;

			if( (xcount+ycount==0) )
			{
				if(ofile) Close(ofile);

				if( icount ) sprintf(TempStr,"index%ld.html",icount);
				else sprintf(TempStr,"index.html");
				if( !( ofile=Open( TempStr, MODE_NEWFILE ) ) )
				{
					printf("\nCouldn't open outfile\n");
					goto slut;
				}
				FPrintf( ofile, "<HTML>\n");
				FPrintf( ofile, " <HEAD>\n");
				FPrintf( ofile, "  <META NAME=\"generator\" CONTENT=\"%s\">\n", "GFXIndex v" _VERS_ " by Fredrik Rambris");
				FPrintf( ofile, "  <TITLE>");
				if( title ) FPrintf( ofile, "%s · ",title);
				FPrintf( ofile, "[Page %ld / %ld]", icount+1,((LONG)numpic / ((LONG)xstop*(LONG)ystop))+1 );
				FPrintf( ofile, "</TITLE>\n");
				FPrintf( ofile, " </HEAD>\n");
				FPrintf( ofile, " <BODY BGCOLOR=\"#ffffff\">\n");
				FPrintf( ofile, "  <CENTER>\n");
				FPrintf( ofile, "   <TABLE BORDER=\"0\">\n");
			}

			if( xcount==0 )
			{
				FPrintf( ofile, "    <TR>\n" );
			}

			if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				printf(" *** BREAK\n");
				goto slut;
			}

			if( !Args[2] )
			{
				Printf("Creating HTML for %s...\n", node->ln_Name);
				sprintf(TempStr,"Visage %s INFO >t:ImInfo", node->ln_Name);
				Execute( TempStr, NULL, NULL );
				if( !( htmlfile=Open( "t:ImInfo", MODE_OLDFILE ) ) )
				{
					printf("\nCouldn't get size of image\n");
					goto slut;
				}
				if( !FGets( htmlfile, TempStr, 1023 ) ) goto slut;
				if( !FGets( htmlfile, TempStr, 1023 ) ) goto slut;

				stage=0;
				xsize=NULL;
				ysize=NULL;
				for( tempcount=0;TempStr[tempcount];tempcount++ )
				{
					if( (TempStr[tempcount]!=' ')&(stage==0) ) stage++;
					else if( (TempStr[tempcount]==' ')&(stage==1) ) stage++;
					else if( (TempStr[tempcount]!=' ')&(stage==2) ) stage++;
					else if( (TempStr[tempcount]==' ')&(stage==3) ) stage++;
					else if( (TempStr[tempcount]!=' ')&(stage==4) )
					{
						stage++;
						xsize=&TempStr[tempcount];
					}
					else if( (TempStr[tempcount]=='x')&(stage==5) )
					{
						stage++;
						TempStr[tempcount]='\0';
						ysize=&TempStr[tempcount+1];
					}
					else if( (TempStr[tempcount]=='x')&(stage==6) )
					{
						TempStr[tempcount]='\0';
						break;
					}

				}
				Close( htmlfile );
				htmlfile=NULL;

				if( xsize ) XSize=atoi( xsize );
				if( ysize ) YSize=atoi( ysize );

				sprintf(TempStr, "%s/%s.html", ThumbDir, node->ln_Name );
				if( !( htmlfile=Open( TempStr, MODE_NEWFILE ) ) )
				{
					printf("\nCouldn't open outfile\n");
					goto slut;
				}
				if( icount ) sprintf(TempStr,"index%ld.html",icount);
				else sprintf(TempStr,"index.html");
				FPrintf( htmlfile, "<HTML>\n");
				FPrintf( htmlfile, " <HEAD>\n");
				FPrintf( htmlfile, "  <META NAME=\"generator\" CONTENT=\"%s\">\n", "GFXIndex v" _VERS_ " by Fredrik Rambris");
				if( comment ) FPrintf( htmlfile, "  <TITLE>%s</TITLE>\n", comment );
				else FPrintf( htmlfile, "  <TITLE>Image: %s</TITLE>\n", node->ln_Name );
				FPrintf( htmlfile, " </HEAD>\n");
				FPrintf( htmlfile, " <BODY BGCOLOR=\"#ffffff\">\n");
				FPrintf( htmlfile, "  <CENTER>\n");
				FPrintf( htmlfile, "   [ ");
				if( node->ln_Pred->ln_Pred ) FPrintf( htmlfile, "<A HREF=\"%s.html\">Previous</A>", node->ln_Pred->ln_Name );
				if( node->ln_Pred->ln_Pred ) FPrintf( htmlfile, " | " );
				FPrintf( htmlfile, "<A HREF=\"../%s\">Index</A>", TempStr );
				if( node->ln_Succ->ln_Succ ) FPrintf( htmlfile, " | " );
				if( node->ln_Succ->ln_Succ ) FPrintf( htmlfile, "<A HREF=\"%s.html\">Next</A>", node->ln_Succ->ln_Name );
				FPrintf( htmlfile, " ]<P>\n" );
				FPrintf( htmlfile, "   <IMG SRC=\"../%s\"", node->ln_Name );
				if( (xsize!=0)&(ysize!=0) ) FPrintf( htmlfile, " WIDTH=\"%ld\" HEIGHT=\"%ld\"", XSize, YSize );
				FPrintf( htmlfile, ">\n", node->ln_Name );
				FPrintf( htmlfile, "   <HR SIZE=\"1\" NOSHADE>\n");
				FPrintf( htmlfile, "   <FONT SIZE=\"0\">Created using a program by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A>.\n");
				FPrintf( htmlfile, "  </CENTER>\n");
				FPrintf( htmlfile, " </BODY>\n");
				FPrintf( htmlfile, "</HTML>\n");
				Close( htmlfile );
				htmlfile=NULL;
			}
			FPrintf( ofile, "     <TD ALIGN=\"center\">\n      <A HREF=\"%s/%s.html\"><IMG SRC=\"%s/%s\" WIDTH=\"%s\" HEIGHT=\"%s\" BORDER=\"0\"",ThumbDir,node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize );
			if( comment ) FPrintf( ofile, " ALT=\"%s\"", comment );
			FPrintf( ofile, "><BR><FONT SIZE=\"-2\">%s</FONT></A>\n     </TD>\n",node->ln_Name);

			xcount++;
			if( xcount>=xstop )
			{
				xcount=0;
				ycount++;
				FPrintf( ofile, "    </TR>\n" );
			}

			if( ycount>=ystop )
			{
				ycount=0;
				FPrintf( ofile, "   </TABLE><BR>\n" );
				FPrintf( ofile, "   [ " );
				if( icount>1 ) FPrintf( ofile, "<A HREF=\"index%ld.html\">Previous</A> | ",icount-1);
				else if( icount==1 ) FPrintf( ofile, "<A HREF=\"index.html\">Previous</A> | ");
				if( numpage>1 )
				{
					for( tempcount=0;tempcount<numpage;tempcount++)
					{
						if( tempcount!=icount )
						{
							if( tempcount==0 ) FPrintf( ofile, "<A HREF=\"index.html\">" );
							else FPrintf( ofile, "<A HREF=\"index%ld.html\">", tempcount );
						}
						FPrintf( ofile, "%ld", tempcount+1 );
						if( tempcount!=icount ) FPrintf( ofile, "</A>");
						FPrintf( ofile, " | \n");
					}
				}
				FPrintf( ofile, "<A HREF=\"index%ld.html\">Next</A> ]\n",icount+1);
				FPrintf( ofile, "   <BR>[ <A HREF=\"../index.html\">Parent</A> ]\n");
				FPrintf( ofile, "   <HR SIZE=\"1\" NOSHADE>\n");
				FPrintf( ofile, "   <FONT SIZE=\"0\">Created using a program by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A>.\n");
				FPrintf( ofile, "  </CENTER>\n </BODY>\n</HTML>\n");
				icount++;
			}
		}
	}

	if( !error )
	{
		if( xcount>0 )
		{
			xcount=0;
			ycount++;
			FPrintf( ofile, "    </TR>\n" );
		}

		if( xcount+ycount>0 )
		{
			ycount=0;
			FPrintf( ofile, "   </TABLE><BR>\n" );
			if( icount>1 ) FPrintf( ofile, "   [ <A HREF=\"index%ld.html\">Previous</A> |\n",icount-1);
			else if( icount==1 ) FPrintf( ofile, "   [ <A HREF=\"index.html\">Previous</A> |\n");
			if( numpage>1 )
			{
				for( tempcount=0;tempcount<numpage;tempcount++)
				{
					if( tempcount!=icount )
					{
						if( tempcount==0 ) FPrintf( ofile, "<A HREF=\"index.html\">" );
						else FPrintf( ofile, "<A HREF=\"index%ld.html\">", tempcount );
					}
					FPrintf( ofile, "%ld", tempcount+1 );
					if( tempcount!=icount ) FPrintf( ofile, "</A>");
					if( tempcount<(numpage-1) ) FPrintf( ofile, " | \n");
					else FPrintf( ofile, " ]\n");
				}
			}
			FPrintf( ofile, "   <BR>[ <A HREF=\"../index.html\">Parent</A> ]\n");
			FPrintf( ofile, "   <HR SIZE=\"1\" NOSHADE>\n");
			FPrintf( ofile, "   <FONT SIZE=\"0\">Created using a program by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A>.\n");
			FPrintf( ofile, "  </DIV>\n </BODY>\n</HTML>\n");
			icount++;
		}
	}


	slut:
	printf("\r\x1b[1 p\x1b[K");
	if( title ) FreeVec( title );
	if( dir_name ) FreeVec( dir_name );
	if( ofile ) Close( ofile );
	if( htmlfile ) Close( htmlfile );
	remlist( &Dir );
}

BOOL Exists( STRPTR file )
{
	BPTR ex_lock;
	if( ex_lock=Lock( file, ACCESS_READ ) )
	{
		UnLock( ex_lock );
		return( TRUE );
	}
	else return( FALSE );
}

void *DisplayNameList(struct List *list,UWORD code)
{
	struct Node *node;
	ULONG i=0;
	if (list->lh_TailPred != (struct Node *)list)
	{
		for (node = list->lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
			if(code==i)
				break;
			i++;
		}
		return(node->ln_Name);
	}
}

void remlist( struct List *list )
{
	struct Node *node;

	while(1)
	{
		node=RemTail(list);
		if (node==NULL) break;
		FreeVec(node);
	}
}


void *addlist( struct List *list, void *buffer, ULONG length )
{
	struct Node *node;

	if(node=AllocVec(sizeof(struct Node)+length+1, MEMF_CLEAR))
	{
		node->ln_Name=(char*)node+(sizeof(struct Node));
		memcpy((char*)node+sizeof(struct Node), buffer, length );
		AddTail(list, node);
		return( node->ln_Name );
	}
	return( NULL );
}

void SortList( struct List *list )
{
	char *tmp;
	BOOL flag;
	struct Node *node;

	bubbel:
	flag=FALSE;
	for (node = list->lh_Head ; node->ln_Succ->ln_Succ ; node = node->ln_Succ)
	{
		if( Stricmp(node->ln_Name,node->ln_Succ->ln_Name)>0 )
		{
			tmp=node->ln_Name;
			node->ln_Name=node->ln_Succ->ln_Name;
			node->ln_Succ->ln_Name=tmp;
			flag=TRUE;
		}
	}
	if(flag) goto bubbel;
}

BOOL getnewdir( char *dirstr )
{
	BPTR lock;
	BOOL ret=FALSE;
	if( lock=Lock(dirstr,ACCESS_READ) )
	{
		ret=getdir( lock );
		UnLock( lock );
	}
	return( ret );
}

BOOL getdir( BPTR lock )
{
	struct FileInfoBlock *fib;
	BOOL ret=TRUE;
	UBYTE *str;
	UWORD len;

	if( !( fib=AllocDosObject( DOS_FIB, NULL ) ) )
	{
		return( FALSE );
	}

	if( lock )
	{

		remlist( &Dir );
		Examine( lock, fib );

		while( ExNext( lock, fib ) )
		{
			if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				printf(" *** BREAK\n");
				ret=FALSE;
				break;
			}

			if( fib->fib_DirEntryType<0 )
			{
				if( !strstr( fib->fib_FileName, ".html" ) )
				{
					if( !strstr( fib->fib_FileName, ".info" ) )
					{
						if( Stricmp( fib->fib_FileName, "pspbrwse.jbf" ) )
						{
							len=strlen( fib->fib_FileName )+2;
							if( fib->fib_Comment ) len+=strlen(fib->fib_Comment);
							if( !(str=addlist( &Dir, fib->fib_FileName, len ) ) )
							{
								printf(" *** ERROR\n");
								ret=FALSE;
								break;
							}
							str+=strlen( str )+1;
							if( fib->fib_Comment ) if( strlen( fib->fib_Comment) ) strcpy( str, fib->fib_Comment );
						}
					}
				}
			}
		}
		FreeDosObject( DOS_FIB, fib );
		SortList( &Dir );
	}
	return(ret);
}
