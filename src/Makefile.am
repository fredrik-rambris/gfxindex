SUBDIRS = libtaglist libgfx

bin_PROGRAMS = gfxindex

INCLUDES = -I./libtaglist -I./libgfx -DSYSCONFDIR=\"$(sysconfdir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

gfxindex_LDADD = libgfx/libgfx.a libtaglist/libtaglist.a

gfxindex_SOURCES = \
	confargs.c	\
	confargs.h	\
	defaults.h	\
	gfxindex.c	\
	gfx_types.h	\
	global.h	\
	preferences.c	\
	preferences.h	\
	thumbdata.c \
	thumbdata.h	\
	util.c	\
	util.h \
	win32.c \
	win32.h \
	xml.c \
	xml.h \
	exif.c \
	exif.h
