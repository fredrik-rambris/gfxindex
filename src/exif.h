#ifndef EXIF_H
#define EXIF_H
#if HAVE_LIBEXIF
#include <stdio.h>
#include "util.h"

typedef struct _ExifInfo
{
	char *ei_make;
	char *ei_model;
	BOOL ei_flash;
	char *ei_exposure;
	char *ei_aperture;
	int ei_rotate;
	char *ei_date;
	char *ei_focal;
} ExifInfo;

ExifInfo *gfx_exif_file( char *file );
void gfx_exif_free( ExifInfo *ei, BOOL free_ei );

#endif
#endif

