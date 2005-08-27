#ifndef LIBDCRAW_H
#define LIBDCRAW_H

#define _GNU_SOURCE
#include <setjmp.h>

typedef unsigned char uchar;
#ifdef WIN32
typedef unsigned short ushort;
#endif

/* Global Variables */

typedef struct _dcrawhandle {
FILE *ifp;
short order;
char *ifname, make[64], model[64], model2[64];
int data_offset, curve_offset, curve_length, timestamp;
int tiff_data_compression, kodak_data_compression;
int raw_height, raw_width, top_margin, left_margin;
int height, width, colors, black, rgb_max;
int iheight, iwidth, shrink;
int is_canon, is_cmy, is_foveon, use_coeff, trim, ymag;
int zero_after_ff;
unsigned filters;
ushort (*image)[4], white[8][8];
void (*load_raw)(struct _dcrawhandle *handle);
float gamma_val, bright, red_scale, blue_scale;
int four_color_rgb, document_mode, quick_interpolate;
int verbose, use_auto_wb, use_camera_wb;
float camera_red, camera_blue;
float pre_mul[4], coeff[3][4];
int histogram[0x2000];
void (*write_fun)(struct _dcrawhandle *handle, FILE *);
jmp_buf failure;
} dcrawhandle;

void dcraw_inithandle( dcrawhandle *handle );
int identify(dcrawhandle *handle);
void convert_to_rgb(dcrawhandle *handle);
void foveon_interpolate(dcrawhandle *handle);
void scale_colors(dcrawhandle *handle);
void vng_interpolate(dcrawhandle *handle);

#endif
