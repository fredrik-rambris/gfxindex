SUBDIRS = libtaglist libgfx

bin_PROGRAMS = gfxindex

INCLUDES = -I./libtaglist -I./libgfx

gfxindex_LDADD = libtaglist/libtaglist.a libgfx/libgfx.a

gfxindex_SOURCES = defaults.h \
		   global.h   \
		   gfxindex.c \
		   args.h     \
		   args.c
