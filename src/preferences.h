#ifndef PREFERENCES_H
#define PREFERENCES_H
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "confargs.h"

#define CONF(cfarg,val) (cfarg->ca_values[val])
#define SCONF(cfarg,val) ((char *)(cfarg->ca_values[val]))
#define ICONF(cfarg,val) ((int)((cfarg->ca_values[val])))
#define BCONF(cfarg,val) ((BOOL)((cfarg->ca_values[val])))

extern ConfArg *global_confarg;
extern ConfArgItem config_definition[];

/* To speed up things we provide direct access to preferences values
 * by enumerating constants of the positions of their value (hey...
 * it's late here!! ;-) ). Anyway... you get a value by something like
 * conf(cfg,PREFS_THUMBDIR) */
enum {
	PREFS_DIR,
	PREFS_CONFIG,
	PREFS_SAVECONFIG,
	PREFS_OUTDIR,
	PREFS_THUMBDIR,
	PREFS_QUIET,
	PREFS_VERBOSE,
	PREFS_TITLE,
	PREFS_CAPTION,
	PREFS_OVERWRITE,
	PREFS_REMAKETHUMBS,
	PREFS_REMAKEBIGS,
	PREFS_RECURSIVE,
	PREFS_THUMBS,
	PREFS_PAD,
	PREFS_SOFTPAD,
	PREFS_THUMBWIDTH,
	PREFS_THUMBHEIGHT,
	PREFS_WIDTHS,
	PREFS_DEFWIDTH,
	PREFS_RELWIDTH,
	PREFS_COPY,
	PREFS_ORIGINAL,
	PREFS_BIGQUALITY,
	PREFS_QUALITY,
	PREFS_THUMBBGCOLOR,
	PREFS_THUMBBACKGROUND,
	PREFS_THUMBALPHA,
	PREFS_THUMBBEVEL,
	PREFS_INNERBEVEL,
	PREFS_BEVELBG,
	PREFS_BEVELBRIGHT,
	PREFS_BEVELDARK,
	PREFS_SCALE,
	PREFS_INDEXES,
	PREFS_TITLES,
	PREFS_EXTENSIONS,
	PREFS_CAPTIONS,
	PREFS_NUMLINK,
	PREFS_NAVTHUMBS,
	PREFS_DIRNAV,
	PREFS_NUMX,
	PREFS_NUMY,
	PREFS_BODYARGS,
	PREFS_TABLEARGS,
	PREFS_CELLARGS,
	PREFS_CSS,
	PREFS_CSSFILE,
	PREFS_PARENTDOC,
	PREFS_LEFT,
	PREFS_SPACE,
	PREFS_DIVIDER,
	PREFS_RIGHT,
	PREFS_PREV,
	PREFS_NEXT,
	PREFS_INDEX,
	PREFS_PARENT,
	PREFS_USETITLES,
#if HAVE_LIBEXIF
	PREFS_EXIF,
#endif
	PREFS_WRITEALBUM,
	PREFS_INDEXTITLE,
	PREFS_INDEXHEADER,
	PREFS_INDEXHEADERFILE,
	PREFS_INDEXFOOTER,
	PREFS_INDEXFOOTERFILE,
	PREFS_PICTURETITLE,
	PREFS_PICTUREHEADER,
	PREFS_PICTUREHEADERFILE,
	PREFS_PICTUREFOOTER,
	PREFS_PICTUREFOOTERFILE,
	PREFS_FULLDOC,
	PREFS_FLAT,
	PREFS_LAST_ONE_DUMMY
};

#endif
