#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "confargs.h"
#include "gfx.h"
#include "defaults.h"
#include "util.h"
ConfArg *global_confarg=NULL;

void quality_sanity_check( ConfArgItem *ci, int *quality )
{
	if( *quality>100 ) *quality=100;
	if( *quality<0 ) *quality=0;
}

/* We bubble sort the list. Largest first */
void widths_sanity_check( ConfArgItem *ci, int **widthsp )
{
	int i, start=0, temp, end=0, *widths=*widthsp;
	if( !widths ) return;
	while( widths[start] ) start++;
	while( end<start )
	{
		for( i=start; i>end; i-- )
		{
			if( widths[i]>widths[i-1] )
			{
				temp=widths[i];
				widths[i]=widths[i-1];
				widths[i-1]=temp;
			}
		}
		end++;
	}
}

/* Here we define what should go into the configuration file */
ConfArgItem config_definition[]=
{
	{ "dir", CT_ARG_STR, 'd', CIA_CMDLINE, DEFAULT_DIR, "Starting directory", NULL },
	{ "config", CT_ARG_STR, '\0', CIA_CMDLINE, (void *)( DEFAULT_CONFIG ), "Read options from file", NULL },
	{ "saveconfig", CT_ARG_STR, '\0', CIA_CMDLINE, (void *)( DEFAULT_SAVECONFIG ), "Save options to file and quit", NULL },
	{ "outdir", CT_ARG_STR, 'O', CIA_BOTH, DEFAULT_OUTDIR, "Output base directory", NULL },
	{ "thumbdir", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_THUMBDIR, "The directory where the thumbnails will be put", NULL },
	{ "quiet", CT_ARG_BOOL, 'q', CIA_BOTH, (void *)( DEFAULT_QUIET ), "Suspend all output to the console", NULL },
	{ "verbose", CT_ARG_INT, 'v', CIA_BOTH, (void *)( DEFAULT_VERBOSE ), "Verbosity level (0-2). 0=quiet, 2=chatty.", NULL },
	{ "title", CT_ARG_STR, 't', CIA_BOTH, DEFAULT_TITLE, "The title of the album", NULL },
	{ "caption", CT_ARG_STR, 'c', CIA_BOTH, DEFAULT_CAPTION, "The caption of the album", NULL },
	{ "overwrite", CT_ARG_BOOL, 'o', CIA_BOTH, (void *)( DEFAULT_OVERWRITE ), "Overwrite thumbnails, etc.", NULL },
	{ "remakethumbs", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_REMAKETHUMBS ), "Force creation of thumbnails", NULL },
	{ "remakebigs", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_REMAKEBIGS ), "Force creation of bigs", NULL },
	{ "recursive", CT_ARG_BOOL, 'r', CIA_BOTH, (void *)( DEFAULT_RECURSIVE ), "Dive into subdirectories", NULL },
	{ "thumbs", CT_ARG_BOOL, 'T', CIA_BOTH, (void *)( DEFAULT_MAKETHUMBS ), "Create thumbnails", NULL },
	{ "pad", CT_ARG_BOOL, 'p', CIA_BOTH, (void *)( DEFAULT_PAD ), "Pad thumbnails for equal size", NULL },
	{ "softpad", CT_ARG_BOOL, 'p', CIA_BOTH, (void *)( DEFAULT_SOFTPAD ), "Add padding in IMG tag instead", NULL },
	{ "thumbwidth", CT_ARG_INT, 'w', CIA_BOTH, (void *)( DEFAULT_THUMBWIDTH ), "Width of thumbnails", NULL },
	{ "thumbheight", CT_ARG_INT, 'h', CIA_BOTH, (void *)( DEFAULT_THUMBHEIGHT ), "Height of thumbnails", NULL },
	{ "widths", CT_ARG_INTARRAY, 'W', CIA_BOTH, DEFAULT_WIDTHS, "Scale original to these widths. (comma separated)", (void *)widths_sanity_check },
	{ "defwidth", CT_ARG_INT, '\0', CIA_BOTH, DEFAULT_DEFWIDTH, "Which width should be considered default", NULL },
	{ "relwidth", CT_ARG_INT, '\0', CIA_BOTH, DEFAULT_RELWIDTH, "Use percentage width when viewing picture", NULL },
	{ "copy", CT_ARG_BOOL, '\0', CIA_BOTH, DEFAULT_COPY, "Copy original image to outdir when it's defined", NULL }, 
	{ "original", CT_ARG_BOOL, '\0', CIA_BOTH, DEFAULT_ORIGINAL, "Show original when using downscaled bigs", NULL },
	{ "bigquality", CT_ARG_INT, '\0', CIA_BOTH, (void *)( DEFAULT_BIGQUALITY ), "JPEG quality (0-100) of bigs when saving", (void *)quality_sanity_check },
	{ "quality", CT_ARG_INT, 'Q', CIA_BOTH, (void *)( DEFAULT_QUALITY ), "JPEG quality (0-100) of thumbnails when saving", (void *)quality_sanity_check },
	{ "thumbbgcolor", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_BGCOLOR, "Background color of thumbnails", NULL },
	{ "thumbbackground", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_BACKGROUND, "Background image of thumbnails", NULL },
	{ "thumbalpha", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_ALPHA, "Copy alphachannel from this image to thumbnails", NULL },
	{ "thumbbevel", CT_ARG_BOOL, 'b', CIA_BOTH, (void *)( DEFAULT_BEVEL ), "Create beveled thumbnails", NULL },
	{ "innerbevel", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_INNERBEVEL ), "Draw inner bevel", NULL },
	{ "bevelbg", CT_ARG_STR, 'g', CIA_BOTH, DEFAULT_BEVELBACK, "Background color in beveled state", NULL },
	{ "bevelbright", CT_ARG_STR, 'l', CIA_BOTH, DEFAULT_BEVELBRIGHT, "Color of bright edges in bevel", NULL },
	{ "beveldark", CT_ARG_STR, 'm', CIA_BOTH, DEFAULT_BEVELDARK, "Color of dark edges in bevel", NULL },
	{ "scale", CT_ARG_INT, 'f', CIA_BOTH, (void *)( DEFAULT_SCALE ), "Method of rescaling", NULL },
	{ "indexes", CT_ARG_BOOL, 'i', CIA_BOTH, (void *)( DEFAULT_GENINDEX ), "Create HTML indexes", NULL },
	{ "titles", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_TITLES ), "Show filenames/titles", NULL },
	{ "extensions", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_EXTENSIONS ), "Show extensions", NULL },
	{ "captions", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_CAPTIONS ), "Show captions", NULL },
	{ "numlink", CT_ARG_BOOL, 'n', CIA_BOTH, (void *)( DEFAULT_NUMLINK ), "Use numeric links to individual pages", NULL },
	{ "navthumbs", CT_ARG_INT, 'u', CIA_BOTH, (void *)( DEFAULT_THUMBSCALE ), "Use thumbnails for Prev and Next (set scale in percent)", NULL },
	{ "dirnav", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_DIRNAV ), "Show directory depth when recursing", NULL },
	{ "numx", CT_ARG_INT, 'x', CIA_BOTH, (void *)( DEFAULT_XSTOP ), "Number of thumbnails per row", NULL },
	{ "numy", CT_ARG_INT, 'y', CIA_BOTH, (void *)( DEFAULT_YSTOP ), "Number of rows per page", NULL },
	{ "bodyargs", CT_ARG_STR, 'a', CIA_BOTH, DEFAULT_BODYARGS, "Statements inserted into the BODY tag", NULL },
	{ "tableargs", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_TABLEARGS, "Statements inserted into the TABLE tag", NULL },
	{ "cellargs", CT_ARG_STR, 'e', CIA_BOTH, DEFAULT_CELLARGS, "Statements inserted into the TD tag", NULL },
	{ "css", CT_ARG_STR, 's', CIA_BOTH, DEFAULT_CSS, "Relative path to Cascading Style Sheet", NULL },
	{ "cssfile", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_CSSFILE, "Embed Cascading Style Sheet", NULL },
	{ "parentdoc", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PARENTDOC, "Document linkt to on Parent links", NULL },
	{ "left", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_LEFT, "String used for the navbar", NULL },
	{ "space", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_SPACE, "String used for the navbar", NULL },
	{ "divider", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_DIVIDER, "String used for the navbar", NULL },
	{ "right", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_RIGHT, "String used for the navbar", NULL },
	{ "prev", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_PREVIOUS, "String used for the navbar", NULL },
	{ "next", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_NEXT, "String used for the navbar", NULL },
	{ "index", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_STR_INDEX, "String used for the navbar", NULL },
	{ "parent", CT_ARG_STR, '\0',CIA_BOTH, DEFAULT_STR_PARENT, "String used for the navbar", NULL },
	{ "usetitles", CT_ARG_BOOL, '\0', CIA_BOTH, DEFAULT_USETITLES, "Use titles instead of 'Prev' or 'Next'", NULL },
#if HAVE_LIBEXIF
	{ "exif", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_EXIF ), "Show EXIF information if available", NULL },
#endif
	{ "writealbum", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)( DEFAULT_WRITEALBUM ), "Write album.xml template", NULL },
	{ "indextitle", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_INDEXTITLE, "Format of index title", NULL },
	{ "indexheader", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_INDEXHEADER, "Format of header", NULL },
	{ "indexheaderfile", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_INDEXHEADERFILE, "Embed file at the beginning of each document", NULL },
	{ "indexfooter", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_INDEXFOOTER, "Format of footer", NULL },
	{ "indexfooterfile", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_INDEXFOOTERFILE, "Embed file at the end of each document", NULL },
	{ "picturetitle", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PICTURETITLE, "Format of picture title", NULL },
	{ "pictureheader", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PICTUREHEADER, "Format of header", NULL },
	{ "pictureheaderfile", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PICTUREHEADERFILE, "Embed file at the beginning of each document", NULL },
	{ "picturefooter", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PICTUREFOOTER, "Format of footer", NULL },
	{ "picturefooterfile", CT_ARG_STR, '\0', CIA_BOTH, DEFAULT_PICTUREFOOTERFILE, "Embed file at the end of each document", NULL },
	{ "fulldoc", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)DEFAULT_FULLDOC, "Write full html document or just the body contents", NULL },
	{ "flat", CT_ARG_BOOL, '\0', CIA_BOTH, (void *)DEFAULT_FLAT, "Flat directory structure", NULL },
	{ NULL, CT_ARG_END, '\0', CIA_BOTH, NULL, NULL, NULL }
};
