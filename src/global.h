#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef HAVE_CONFIG_H
 #ifndef PACKAGE
  #include <config.h>
 #endif
#endif

#include "gfx_types.h"

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>

#include "gfx.h"
#include "preferences.h"
#include <taglist.h>


/*
struct ThumbData
{
	char *image;
	guint imagewidth, imageheight;
	char *thumb;
	guint thumbwidth, thumbheight;
	char *extra;
};
*/

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef MAX
#define MAX(a,b) ((a>b)?a:b)
#endif

#ifndef MIN
#define MIN(a,b) ((a<b)?a:b)
#endif

#ifdef __WIN32__
	#define PATH_DELIMITER '\\'
	#include "win32.h"
#else
	#define PATH_DELIMITER '/'
#endif

#endif /* GLOBAL_H */
