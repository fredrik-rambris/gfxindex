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
void addlist( struct List *list, void *buffer, ULONG length );
void SortList( struct List *list );
void printlist( struct List *list );
void getnewdir( char *dirstr );
void getdir( BPTR lock );
BOOL Exists( STRPTR file );

struct List Dir;

ULONG sigmask=SIGBREAKF_CTRL_C;

#define _VERS_ "1.3"

char VerStr[]="\0$VER: GFXIndex " _VERS_ " (" __DATE2__ ")";

void main( void )
{
	struct Node *node;
	char ThumbDir[]="thumbnails", ThumbSize[]="80";
	char TempStr[1024],*title=NULL,*dir_name=NULL;
	UWORD xcount=0, ycount=0, numpic=0, icount=0, xstop=7, ystop=3;
	UWORD numpage,tempcount;
	BPTR ofile=NULL;
	BOOL error=FALSE;
	char Template[14]="TITLE/K,DIR/K";
	struct	RDArgs	*MyRDArgs;
	long Args[2]={0,0};

	Printf("GFXIndex v" _VERS_ " © 1998 Fredrik Rambris. All rights reserved.\n");

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
	if( dir_name ) getnewdir( dir_name );
	else getnewdir( "" );

	if( !Exists( ThumbDir ) ) CreateDir(ThumbDir);

	if (Dir.lh_TailPred != (struct Node *)&Dir)
	{

		for (node = Dir.lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
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

		numpage=((LONG)numpic / ((LONG)xstop*(LONG)ystop))+1;

		for (node = Dir.lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
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
				FPrintf( ofile, "  <DIV ALIGN=\"center\">\n");
				FPrintf( ofile, "   <TABLE BORDER=\"0\">\n");
			}

			if( xcount==0 )
			{
				FPrintf( ofile, "    <TR>\n" );
			}

			FPrintf( ofile, "     <TD ALIGN=\"center\">\n      <A HREF=\"%s\"><IMG SRC=\"%s/%s\" WIDTH=\"%s\" HEIGHT=\"%s\" BORDER=\"0\"><BR><FONT SIZE=\"-2\">%s</FONT></A>\n     </TD>\n",node->ln_Name,ThumbDir,node->ln_Name,ThumbSize,ThumbSize,node->ln_Name);

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
				FPrintf( ofile, "   <BR>[ <A HREF=\"../\">Parent</A> ]\n");
				FPrintf( ofile, "   <HR SIZE=\"1\" NOSHADE>\n");
				FPrintf( ofile, "   <FONT SIZE=\"0\">Created using a program by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A>.\n");
				FPrintf( ofile, "  </DIV>\n </BODY>\n</HTML>\n");
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
			FPrintf( ofile, "   <BR>[ <A HREF=\"../\">Parent</A> ]\n");
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


void addlist( struct List *list, void *buffer, ULONG length )
{
	struct Node *node;

	if(node=AllocVec(sizeof(struct Node)+length+1, MEMF_CLEAR|MEMF_PUBLIC))
	{
		node->ln_Name=(char*)node+(sizeof(struct Node));
		memcpy((char*)node+sizeof(struct Node), buffer, length );
		AddTail(list, node);
	}
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

void printlist( struct List *list )
{
	struct Node *node;
	if (list->lh_TailPred != (struct Node *)list)
	{
		for (node = list->lh_Head ; node->ln_Succ ; node = node->ln_Succ)
		{
			printf("%s\n",(char *)node->ln_Name);
		}

	}
}

void getnewdir( char *dirstr )
{
	BPTR lock;
	if( lock=Lock(dirstr,ACCESS_READ) )
	{
		getdir( lock );
		UnLock( lock );
	}
}

void getdir( BPTR lock )
{
	struct FileInfoBlock *fib;

	if( !( fib=AllocDosObject( DOS_FIB, NULL ) ) )
	{
		return;
	}

	if( lock )
	{

		remlist( &Dir );
		Examine( lock, fib );

		while( ExNext( lock, fib ) )
		{
			if( fib->fib_DirEntryType<0 )
			{
				if( Strnicmp( fib->fib_FileName, "index", 5 ) )
				{
					if( !strstr( fib->fib_FileName, ".info" ) )
					{
						if( Stricmp( fib->fib_FileName, "pspbrwse.jbf" ) )
						{
							addlist( &Dir, fib->fib_FileName, strlen(fib->fib_FileName) );
						}
					}
				}
			}
		}
		FreeDosObject( DOS_FIB, fib );
		SortList( &Dir );
	}
}
