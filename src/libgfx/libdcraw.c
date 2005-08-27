/*
   dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2004 by Dave Coffin, dcoffin a cybercom o net

   This is a portable ANSI C program to convert raw image files from
   any digital camera into PPM format.  TIFF and CIFF parsing are
   based upon public specifications, but no such documentation is
   available for the raw sensor data, so writing this program has
   been an immense effort.

   This code is freely licensed for all uses, commercial and
   otherwise.  Comments, questions, and encouragement are welcome.

   $Revision: 1.198 $
   $Date: 2004/07/06 18:36:13 $

   Hacked and slashed by Fredrik Rambris
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_DCRAW

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
   By defining NO_JPEG, you lose only the ability to
   decode compressed .KDC files from the Kodak DC120.
 */
#ifndef NO_JPEG
#include <jpeglib.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define strcasecmp stricmp
typedef __int64 INT64;
#else
#include <unistd.h>
#include <netinet/in.h>
typedef long long INT64;
#endif

#ifdef LJPEG_DECODE
#error Please compile dcraw.c by itself.
#error Do not link it with ljpeg_decode.
#endif

#ifndef LONG_BIT
#define LONG_BIT (8 * sizeof (long))
#endif

#include "libdcraw.h"

static void write_ppm(dcrawhandle *handle, FILE *);

void dcraw_inithandle( dcrawhandle *handle )
{
	if (!handle) return;
	handle->gamma_val=0.6;
	handle->bright=1.0;
	handle->red_scale=1.0;
	handle->blue_scale=1.0;
	handle->write_fun=write_ppm;
}

dcrawhandle *dcraw_createhandle (void)
{
	dcrawhandle *handle=NULL;
	handle=calloc (sizeof (dcrawhandle), 1);
	if (!handle) return handle;
	dcraw_inithandle( handle );
	return handle;
}

struct decode {
  struct decode *branch[2];
  int leaf;
} first_decode[2048], *second_decode, *free_decode;

/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2
 */
#define FC(row,col) \
	(handle->filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define BAYER(row,col) \
	handle->image[((row) >> handle->shrink)*handle->iwidth + ((col) >> handle->shrink)][FC(row,col)]

/*
   PowerShot 600 uses 0xe1e4e1e4:

	  0 1 2 3 4 5
	0 G M G M G M
	1 C Y C Y C Y
	2 M G M G M G
	3 C Y C Y C Y

   PowerShot A5 uses 0x1e4e1e4e:

	  0 1 2 3 4 5
	0 C Y C Y C Y
	1 G M G M G M
	2 C Y C Y C Y
	3 M G M G M G

   PowerShot A50 uses 0x1b4e4b1e:

	  0 1 2 3 4 5
	0 C Y C Y C Y
	1 M G M G M G
	2 Y C Y C Y C
	3 G M G M G M
	4 C Y C Y C Y
	5 G M G M G M
	6 Y C Y C Y C
	7 M G M G M G

   PowerShot Pro70 uses 0x1e4b4e1b:

	  0 1 2 3 4 5
	0 Y C Y C Y C
	1 M G M G M G
	2 C Y C Y C Y
	3 G M G M G M
	4 Y C Y C Y C
	5 G M G M G M
	6 C Y C Y C Y
	7 M G M G M G

   PowerShots Pro90 and G1 use 0xb4b4b4b4:

	  0 1 2 3 4 5
	0 G M G M G M
	1 Y C Y C Y C

   All RGB cameras use one of these Bayer grids:

	0x16161616:	0x61616161:	0x49494949:	0x94949494:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
	1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
	2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
	3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B

 */

#ifndef __GLIBC__
static char *memmem (char *haystack, size_t haystacklen,
              char *needle, size_t needlelen)
{
  char *c;
  for (c = haystack; c <= haystack + haystacklen - needlelen; c++)
    if (!memcmp (c, needle, needlelen))
      return c;
  return NULL;
}
#endif

static void merror ( dcrawhandle *handle, void *ptr, char *where)
{
  if (ptr) return;
  fprintf (stderr, "%s: Out of memory in %s\n", handle->ifname, where);
  longjmp (handle->failure, 1);
}

/*
   Get a 2-byte integer, making no assumptions about CPU byte order.
   Nor should we assume that the compiler evaluates left-to-right.
 */
static ushort fget2 (dcrawhandle *handle, FILE *f)
{
  uchar a, b;

  a = fgetc(f);
  b = fgetc(f);
  if (handle->order == 0x4949)		/* "II" means little-endian */
    return a + (b << 8);
  else				/* "MM" means big-endian */
    return (a << 8) + b;
}

/*
   Same for a 4-byte integer.
 */
static int fget4 (dcrawhandle *handle, FILE *f)
{
  uchar a, b, c, d;

  a = fgetc(f);
  b = fgetc(f);
  c = fgetc(f);
  d = fgetc(f);
  if (handle->order == 0x4949)
    return a + (b << 8) + (c << 16) + (d << 24);
  else
    return (a << 24) + (b << 16) + (c << 8) + d;
}

static void canon_600_load_raw (dcrawhandle *handle)
{
  uchar  data[1120], *dp;
  ushort pixel[896], *pix;
  int irow, orow, col;

  for (irow=orow=0; irow < handle->height; irow++)
  {
    fread (data, 1120, 1, handle->ifp);
    for (dp=data, pix=pixel; dp < data+1120; dp+=10, pix+=8)
    {
      pix[0] = (dp[0] << 2) + (dp[1] >> 6    );
      pix[1] = (dp[2] << 2) + (dp[1] >> 4 & 3);
      pix[2] = (dp[3] << 2) + (dp[1] >> 2 & 3);
      pix[3] = (dp[4] << 2) + (dp[1]      & 3);
      pix[4] = (dp[5] << 2) + (dp[9]      & 3);
      pix[5] = (dp[6] << 2) + (dp[9] >> 2 & 3);
      pix[6] = (dp[7] << 2) + (dp[9] >> 4 & 3);
      pix[7] = (dp[8] << 2) + (dp[9] >> 6    );
    }
    for (col=0; col < handle->width; col++)
      BAYER(orow,col) = pixel[col] << 4;
    for (col=handle->width; col < 896; col++)
      handle->black += pixel[col];
    if ((orow+=2) > handle->height)
      orow = 1;
  }
  handle->black = ((INT64) handle->black << 4) / ((896 - handle->width) * handle->height);
}

static void canon_a5_load_raw (dcrawhandle *handle)
{
  uchar  data[1940], *dp;
  ushort pixel[1552], *pix;
  int row, col;

  for (row=0; row < handle->height; row++) {
    fread (data, handle->raw_width * 10 / 8, 1, handle->ifp);
    for (dp=data, pix=pixel; pix < pixel+handle->raw_width; dp+=10, pix+=8)
    {
      pix[0] = (dp[1] << 2) + (dp[0] >> 6);
      pix[1] = (dp[0] << 4) + (dp[3] >> 4);
      pix[2] = (dp[3] << 6) + (dp[2] >> 2);
      pix[3] = (dp[2] << 8) + (dp[5]     );
      pix[4] = (dp[4] << 2) + (dp[7] >> 6);
      pix[5] = (dp[7] << 4) + (dp[6] >> 4);
      pix[6] = (dp[6] << 6) + (dp[9] >> 2);
      pix[7] = (dp[9] << 8) + (dp[8]     );
    }
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = (pixel[col] & 0x3ff) << 4;
    for (col=handle->width; col < handle->raw_width; col++)
      handle->black += pixel[col] & 0x3ff;
  }
  if (handle->raw_width > handle->width)
    handle->black = ((INT64) handle->black << 4) / ((handle->raw_width - handle->width) * handle->height);
}

/*
   getbits(-1) initializes the buffer
   getbits(n) where 0 <= n <= 25 returns an n-bit integer
 */
static unsigned getbits (dcrawhandle *handle, int nbits)
{
  static unsigned long bitbuf=0;
  static int vbits=0;
  unsigned c, ret;

  if (nbits == 0) return 0;
  if (nbits == -1)
    ret = bitbuf = vbits = 0;
  else {
    ret = bitbuf << (LONG_BIT - vbits) >> (LONG_BIT - nbits);
    vbits -= nbits;
  }
  while (vbits < LONG_BIT - 7) {
    c = fgetc(handle->ifp);
    bitbuf = (bitbuf << 8) + c;
    if (c == 0xff && handle->zero_after_ff)
      fgetc(handle->ifp);
    vbits += 8;
  }
  return ret;
}

static void init_decoder ()
{
  memset (first_decode, 0, sizeof first_decode);
  free_decode = first_decode;
}

/*
   Construct a decode tree according the specification in *source.
   The first 16 bytes specify how many codes should be 1-bit, 2-bit
   3-bit, etc.  Bytes after that are the leaf values.

   For example, if the source is

    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

   then the code is

	00		0x04
	010		0x03
	011		0x05
	100		0x06
	101		0x02
	1100		0x07
	1101		0x01
	11100		0x08
	11101		0x09
	11110		0x00
	111110		0x0a
	1111110		0x0b
	1111111		0xff
 */
static uchar *make_decoder (dcrawhandle *handle, const uchar *source, int level)
{
  struct decode *cur;
  static int leaf;
  int i, next;

  if (level==0) leaf=0;
  cur = free_decode++;
  if (free_decode > first_decode+2048) {
    fprintf (stderr, "%s: decoder table overflow\n", handle->ifname);
    longjmp (handle->failure, 2);
  }
  for (i=next=0; i <= leaf && next < 16; )
    i += source[next++];
  if (i > leaf) {
    if (level < next) {
      cur->branch[0] = free_decode;
      make_decoder (handle, source, level+1);
      cur->branch[1] = free_decode;
      make_decoder (handle, source, level+1);
    } else
      cur->leaf = source[16 + leaf++];
  }
  return (uchar *) source + 16 + leaf;
}

static void crw_init_tables (dcrawhandle *handle, unsigned table)
{
  static const uchar first_tree[3][29] = {
    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

    { 0,2,2,3,1,1,1,1,2,0,0,0,0,0,0,0,
      0x03,0x02,0x04,0x01,0x05,0x00,0x06,0x07,0x09,0x08,0x0a,0x0b,0xff  },

    { 0,0,6,3,1,1,2,0,0,0,0,0,0,0,0,0,
      0x06,0x05,0x07,0x04,0x08,0x03,0x09,0x02,0x00,0x0a,0x01,0x0b,0xff  },
  };

  static const uchar second_tree[3][180] = {
    { 0,2,2,2,1,4,2,1,2,5,1,1,0,0,0,139,
      0x03,0x04,0x02,0x05,0x01,0x06,0x07,0x08,
      0x12,0x13,0x11,0x14,0x09,0x15,0x22,0x00,0x21,0x16,0x0a,0xf0,
      0x23,0x17,0x24,0x31,0x32,0x18,0x19,0x33,0x25,0x41,0x34,0x42,
      0x35,0x51,0x36,0x37,0x38,0x29,0x79,0x26,0x1a,0x39,0x56,0x57,
      0x28,0x27,0x52,0x55,0x58,0x43,0x76,0x59,0x77,0x54,0x61,0xf9,
      0x71,0x78,0x75,0x96,0x97,0x49,0xb7,0x53,0xd7,0x74,0xb6,0x98,
      0x47,0x48,0x95,0x69,0x99,0x91,0xfa,0xb8,0x68,0xb5,0xb9,0xd6,
      0xf7,0xd8,0x67,0x46,0x45,0x94,0x89,0xf8,0x81,0xd5,0xf6,0xb4,
      0x88,0xb1,0x2a,0x44,0x72,0xd9,0x87,0x66,0xd4,0xf5,0x3a,0xa7,
      0x73,0xa9,0xa8,0x86,0x62,0xc7,0x65,0xc8,0xc9,0xa1,0xf4,0xd1,
      0xe9,0x5a,0x92,0x85,0xa6,0xe7,0x93,0xe8,0xc1,0xc6,0x7a,0x64,
      0xe1,0x4a,0x6a,0xe6,0xb3,0xf1,0xd3,0xa5,0x8a,0xb2,0x9a,0xba,
      0x84,0xa4,0x63,0xe5,0xc5,0xf3,0xd2,0xc4,0x82,0xaa,0xda,0xe4,
      0xf2,0xca,0x83,0xa3,0xa2,0xc3,0xea,0xc2,0xe2,0xe3,0xff,0xff  },

    { 0,2,2,1,4,1,4,1,3,3,1,0,0,0,0,140,
      0x02,0x03,0x01,0x04,0x05,0x12,0x11,0x06,
      0x13,0x07,0x08,0x14,0x22,0x09,0x21,0x00,0x23,0x15,0x31,0x32,
      0x0a,0x16,0xf0,0x24,0x33,0x41,0x42,0x19,0x17,0x25,0x18,0x51,
      0x34,0x43,0x52,0x29,0x35,0x61,0x39,0x71,0x62,0x36,0x53,0x26,
      0x38,0x1a,0x37,0x81,0x27,0x91,0x79,0x55,0x45,0x28,0x72,0x59,
      0xa1,0xb1,0x44,0x69,0x54,0x58,0xd1,0xfa,0x57,0xe1,0xf1,0xb9,
      0x49,0x47,0x63,0x6a,0xf9,0x56,0x46,0xa8,0x2a,0x4a,0x78,0x99,
      0x3a,0x75,0x74,0x86,0x65,0xc1,0x76,0xb6,0x96,0xd6,0x89,0x85,
      0xc9,0xf5,0x95,0xb4,0xc7,0xf7,0x8a,0x97,0xb8,0x73,0xb7,0xd8,
      0xd9,0x87,0xa7,0x7a,0x48,0x82,0x84,0xea,0xf4,0xa6,0xc5,0x5a,
      0x94,0xa4,0xc6,0x92,0xc3,0x68,0xb5,0xc8,0xe4,0xe5,0xe6,0xe9,
      0xa2,0xa3,0xe3,0xc2,0x66,0x67,0x93,0xaa,0xd4,0xd5,0xe7,0xf8,
      0x88,0x9a,0xd7,0x77,0xc4,0x64,0xe2,0x98,0xa5,0xca,0xda,0xe8,
      0xf3,0xf6,0xa9,0xb2,0xb3,0xf2,0xd2,0x83,0xba,0xd3,0xff,0xff  },

    { 0,0,6,2,1,3,3,2,5,1,2,2,8,10,0,117,
      0x04,0x05,0x03,0x06,0x02,0x07,0x01,0x08,
      0x09,0x12,0x13,0x14,0x11,0x15,0x0a,0x16,0x17,0xf0,0x00,0x22,
      0x21,0x18,0x23,0x19,0x24,0x32,0x31,0x25,0x33,0x38,0x37,0x34,
      0x35,0x36,0x39,0x79,0x57,0x58,0x59,0x28,0x56,0x78,0x27,0x41,
      0x29,0x77,0x26,0x42,0x76,0x99,0x1a,0x55,0x98,0x97,0xf9,0x48,
      0x54,0x96,0x89,0x47,0xb7,0x49,0xfa,0x75,0x68,0xb6,0x67,0x69,
      0xb9,0xb8,0xd8,0x52,0xd7,0x88,0xb5,0x74,0x51,0x46,0xd9,0xf8,
      0x3a,0xd6,0x87,0x45,0x7a,0x95,0xd5,0xf6,0x86,0xb4,0xa9,0x94,
      0x53,0x2a,0xa8,0x43,0xf5,0xf7,0xd4,0x66,0xa7,0x5a,0x44,0x8a,
      0xc9,0xe8,0xc8,0xe7,0x9a,0x6a,0x73,0x4a,0x61,0xc7,0xf4,0xc6,
      0x65,0xe9,0x72,0xe6,0x71,0x91,0x93,0xa6,0xda,0x92,0x85,0x62,
      0xf3,0xc5,0xb2,0xa4,0x84,0xba,0x64,0xa5,0xb3,0xd2,0x81,0xe5,
      0xd3,0xaa,0xc4,0xca,0xf2,0xb1,0xe4,0xd1,0x83,0x63,0xea,0xc3,
      0xe2,0x82,0xf1,0xa3,0xc2,0xa1,0xc1,0xe3,0xa2,0xe1,0xff,0xff  }
  };

  if (table > 2) table = 2;
  init_decoder();
  make_decoder (handle, first_tree[table], 0);
  second_decode = free_decode;
  make_decoder (handle, second_tree[table], 0);
}

/*
   Decompress "count" blocks of 64 samples each.
 */
static void crw_decompress (dcrawhandle *handle, ushort *outbuf, int count)
{
  struct decode *decode, *dindex;
  int i, leaf, len, diff, diffbuf[64];
  static int carry, pixel, base[2];

  handle->zero_after_ff = 1;
  if (!outbuf) {			/* Initialize */
    carry = pixel = 0;
    fseek (handle->ifp, count, SEEK_SET);
    getbits(handle,-1);
    return;
  }
  while (count--) {
    memset(diffbuf,0,sizeof diffbuf);
    decode = first_decode;
    for (i=0; i < 64; i++ ) {

      for (dindex=decode; dindex->branch[0]; )
	dindex = dindex->branch[getbits(handle,1)];
      leaf = dindex->leaf;
      decode = second_decode;

      if (leaf == 0 && i) break;
      if (leaf == 0xff) continue;
      i  += leaf >> 4;
      len = leaf & 15;
      if (len == 0) continue;
      diff = getbits(handle,len);
      if ((diff & (1 << (len-1))) == 0)
	diff -= (1 << len) - 1;
      if (i < 64) diffbuf[i] = diff;
    }
    diffbuf[0] += carry;
    carry = diffbuf[0];
    for (i=0; i < 64; i++ ) {
      if (pixel++ % handle->raw_width == 0)
	base[0] = base[1] = 512;
      outbuf[i] = ( base[i & 1] += diffbuf[i] );
    }
    outbuf += 64;
  }
}

/*
   Return 0 if the image starts with compressed data,
   1 if it starts with uncompressed low-order bits.

   In Canon compressed data, 0xff is always followed by 0x00.
 */
static int canon_has_lowbits(dcrawhandle *handle)
{
  uchar test[0x4000];
  int ret=1, i;

  fseek (handle->ifp, 0, SEEK_SET);
  fread (test, 1, sizeof test, handle->ifp);
  for (i=540; i < sizeof test - 1; i++)
    if (test[i] == 0xff) {
      if (test[i+1]) return 1;
      ret=0;
    }
  return ret;
}

static void canon_compressed_load_raw(dcrawhandle *handle)
{
  ushort *pixel, *prow;
  int lowbits, shift, i, row, r, col, save, val;
  unsigned irow, icol;
  uchar c;
  INT64 bblack=0;

  pixel = calloc (handle->raw_width*8, sizeof *pixel);
  merror (handle, pixel, "canon_compressed_load_raw()");
  lowbits = canon_has_lowbits(handle);
  shift = 4 - lowbits*2;
  crw_decompress (handle, 0, 540 + lowbits*handle->raw_height*handle->raw_width/4);
  for (row = 0; row < handle->raw_height; row += 8) {
    crw_decompress (handle, pixel, handle->raw_width/8);	/* Get eight rows */
    if (lowbits) {
      save = ftell(handle->ifp);			/* Don't lose our place */
      fseek (handle->ifp, 26 + row*handle->raw_width/4, SEEK_SET);
      for (prow=pixel, i=0; i < handle->raw_width*2; i++) {
	c = fgetc(handle->ifp);
	for (r=0; r < 8; r+=2, prow++) {
	  val = (*prow << 2) + ((c >> r) & 3);
	  if (handle->raw_width == 2672 && val < 512) val += 2;
	  *prow = val;
	}
      }
      fseek (handle->ifp, save, SEEK_SET);
    }
    for (r=0; r < 8; r++) {
      irow = row - handle->top_margin + r;
      if (irow >= handle->height) continue;
      for (col = 0; col < handle->raw_width; col++) {
	icol = col - handle->left_margin;
	if (icol < handle->width)
	  BAYER(irow,icol) = pixel[r*handle->raw_width+col] << shift;
	else
	  bblack += pixel[r*handle->raw_width+col];
      }
    }
  }
  free(pixel);
  if (handle->raw_width > handle->width)
    handle->black = (bblack << shift) / ((handle->raw_width - handle->width) * handle->height);
}

static void kodak_curve (dcrawhandle *handle, ushort *curve)
{
  int i, entries, tag, type, len, val;

  for (i=0; i < 0x1000; i++)
    curve[i] = i;
  if (strcasecmp(handle->make,"KODAK")) return;
  if (!handle->curve_offset) {
    fseek (handle->ifp, 12, SEEK_SET);
    entries = fget2(handle, handle->ifp);
    while (entries--) {
      tag  = fget2(handle, handle->ifp);
      type = fget2(handle, handle->ifp);
      len  = fget4(handle, handle->ifp);
      val  = fget4(handle, handle->ifp);
      if (tag == 0x90d) {
	handle->curve_offset = val;
	handle->curve_length = len;
      }
    }
  }
  if (handle->curve_offset) {
    fseek (handle->ifp, handle->curve_offset, SEEK_SET);
    for (i=0; i < handle->curve_length; i++)
      curve[i] = fget2(handle, handle->ifp);
    for ( ; i < 0x1000; i++)
      curve[i] = curve[i-1];
    handle->rgb_max = curve[i-1] << 2;
  }
  fseek (handle->ifp, handle->data_offset, SEEK_SET);
}

/*
   Not a full implementation of Lossless JPEG,
   just enough to decode Canon and Kodak images.
 */
static void lossless_jpeg_load_raw(dcrawhandle *handle)
{
  int tag, len, jhigh=0, jwide=0, trick, row, col, diff;
  uchar data[256], *dp;
  int vpred[2] = { 0x800, 0x800 }, hpred[2];
  struct decode *dstart[2], *dindex;
  ushort curve[0x10000];
  INT64 bblack=0;
  int min=INT_MAX;

  kodak_curve(handle, curve);
  handle->order = 0x4d4d;
  if (fget2(handle, handle->ifp) != 0xffd8) return;
  do {
    tag = fget2(handle, handle->ifp);
    len = fget2(handle, handle->ifp) - 2;
    if (tag <= 0xff00 || len > 255) return;
    fread (data, 1, len, handle->ifp);
    switch (tag) {
      case 0xffc3:
	jhigh = (data[1] << 8) + data[2];
	jwide = (data[3] << 8) + data[4];
	break;
      case 0xffc4:
	init_decoder();
	dstart[0] = dstart[1] = free_decode;
	for (dp = data; dp < data+len && *dp < 2; ) {
	  dstart[*dp] = free_decode;
	  dp = make_decoder (handle, ++dp, 0);
	}
    }
  } while (tag != 0xffda);

  trick = 2 * jwide / handle->width;
  handle->zero_after_ff = 1;
  getbits(handle, -1);
  for (row=0; row < handle->raw_height; row++)
    for (col=0; col < handle->raw_width; col++)
    {
      for (dindex = dstart[col & 1]; dindex->branch[0]; )
	dindex = dindex->branch[getbits(handle, 1)];
      len = dindex->leaf;
      diff = getbits(handle, len);
      if ((diff & (1 << (len-1))) == 0)
	diff -= (1 << len) - 1;
      if (col < 2 && (row % trick == 0)) {
	vpred[col] += diff;
	hpred[col] = vpred[col];
      } else
	hpred[col & 1] += diff;
      diff = hpred[col & 1];
      if (diff < 0) diff = 0;
      if ((unsigned) (row-handle->top_margin) >= handle->height)
	continue;
      if ((unsigned) (col-handle->left_margin) < handle->width) {
	BAYER(row-handle->top_margin,col-handle->left_margin) = curve[diff] << 2;
	if (min > curve[diff])
	    min = curve[diff];
      } else
	bblack += curve[diff];
    }
  if (handle->raw_width > handle->width)
    handle->black = (bblack << 2) / ((handle->raw_width - handle->width) * handle->height);
  if (!strcasecmp(handle->make,"KODAK"))
    handle->black = min << 2;
}

static void nikon_compressed_load_raw(dcrawhandle *handle)
{
  static const uchar nikon_tree[] = {
    0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,
    5,4,3,6,2,7,1,0,8,9,11,10,12
  };
  int vpred[4], hpred[2], csize, row, col, i, len, diff;
  ushort *curve;
  struct decode *dindex;

  init_decoder();
  make_decoder (handle, nikon_tree, 0);

  fseek (handle->ifp, handle->curve_offset, SEEK_SET);
  for (i=0; i < 4; i++)
    vpred[i] = fget2(handle, handle->ifp);
  csize = fget2(handle, handle->ifp);
  curve = calloc (csize, sizeof *curve);
  merror (handle, curve, "nikon_compressed_load_raw()");
  for (i=0; i < csize; i++)
    curve[i] = fget2(handle, handle->ifp);

  fseek (handle->ifp, handle->data_offset, SEEK_SET);
  getbits(handle, -1);

  for (row=0; row < handle->height; row++)
    for (col=0; col < handle->raw_width; col++)
    {
      for (dindex=first_decode; dindex->branch[0]; )
	dindex = dindex->branch[getbits(handle, 1)];
      len = dindex->leaf;
      diff = getbits(handle, len);
      if ((diff & (1 << (len-1))) == 0)
	diff -= (1 << len) - 1;
      if (col < 2) {
	i = 2*(row & 1) + (col & 1);
	vpred[i] += diff;
	hpred[col] = vpred[i];
      } else
	hpred[col & 1] += diff;
      if ((unsigned) (col-handle->left_margin) >= handle->width) continue;
      diff = hpred[col & 1];
      if (diff < 0) diff = 0;
      if (diff >= csize) diff = csize-1;
      BAYER(row,col-handle->left_margin) = curve[diff] << 2;
    }
  free(curve);
}

static void nikon_load_raw(dcrawhandle *handle)
{
  int irow, row, col, i;

  getbits(handle, -1);
  for (irow=0; irow < handle->height; irow++) {
    row = irow;
    if (handle->model[0] == 'E') {
      row = irow * 2 % handle->height + irow / (handle->height/2);
      if (row == 1 && atoi(handle->model+1) < 5000) {
	fseek (handle->ifp, 0, SEEK_END);
	fseek (handle->ifp, ftell(handle->ifp)/2, SEEK_SET);
	getbits(handle, -1);
      }
    }
    for (col=0; col < handle->raw_width; col++) {
      i = getbits(handle, 12);
      if ((unsigned) (col-handle->left_margin) < handle->width)
	BAYER(row,col-handle->left_margin) = i << 2;
      if (handle->tiff_data_compression == 34713 && (col % 10) == 9)
	getbits(handle, 8);
    }
  }
}

/*
   Figure out if a NEF file is compressed.  These fancy heuristics
   are only needed for the D100, thanks to a bug in some cameras
   that tags all images as "compressed".
 */
static int nikon_is_compressed(dcrawhandle *handle)
{
  uchar test[256];
  int i;

  if (handle->tiff_data_compression != 34713)
    return 0;
  if (strcmp(handle->model,"D100"))
    return 1;
  fseek (handle->ifp, handle->data_offset, SEEK_SET);
  fread (test, 1, 256, handle->ifp);
  for (i=15; i < 256; i+=16)
    if (test[i]) return 1;
  return 0;
}

/*
   Returns 1 for a Coolpix 990, 0 for a Coolpix 995.
 */
static int nikon_e990(dcrawhandle *handle)
{
  int i, histo[256];
  const uchar often[] = { 0x00, 0x55, 0xaa, 0xff };

  memset (histo, 0, sizeof histo);
  fseek (handle->ifp, 2064*1540*3/4, SEEK_SET);
  for (i=0; i < 2000; i++)
    histo[fgetc(handle->ifp)]++;
  for (i=0; i < 4; i++)
    if (histo[often[i]] > 400)
      return 1;
  return 0;
}

/*
   Returns 1 for a Coolpix 2100, 0 for anything else.
 */
static int nikon_e2100(dcrawhandle *handle)
{
  uchar t[12];
  int i;

  fseek (handle->ifp, 0, SEEK_SET);
  for (i=0; i < 1024; i++) {
    fread (t, 1, 12, handle->ifp);
    if (((t[2] & t[4] & t[7] & t[9]) >> 4
	& t[1] & t[6] & t[8] & t[11] & 3) != 3)
      return 0;
  }
  return 1;
}

static void nikon_e2100_load_raw(dcrawhandle *handle)
{
  uchar   data[2424], *dp;
  ushort pixel[1616], *pix;
  int row, col;

  for (row=0; row <= handle->height; row+=2) {
    if (row == handle->height) {
      fseek (handle->ifp, 8792, SEEK_CUR);
      row = 1;
    }
    fread (data, 2424, 1, handle->ifp);
    for (dp=data, pix=pixel; dp < data+2424; dp+=12, pix+=8)
    {
      pix[0] = (dp[ 3] << 2) + (dp[2] >> 6);
      pix[1] = (dp[ 1] >> 2) + (dp[2] << 6);
      pix[2] = (dp[ 0] << 2) + (dp[7] >> 6);
      pix[3] = (dp[ 6] >> 2) + (dp[7] << 6);
      pix[4] = (dp[ 5] << 2) + (dp[4] >> 6);
      pix[5] = (dp[11] >> 2) + (dp[4] << 6);
      pix[6] = (dp[10] << 2) + (dp[9] >> 6);
      pix[7] = (dp[ 8] >> 2) + (dp[9] << 6);
    }
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = (pixel[col] & 0x3ff) << 4;
  }
}

static void nikon_e950_load_raw(dcrawhandle *handle)
{
  int irow, row, col;

  getbits(handle, -1);
  for (irow=0; irow < handle->height; irow++) {
    row = irow * 2 % handle->height;
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = getbits(handle, 10) << 4;
    for (col=28; col--; )
      getbits(handle, 8);
  }
}

/*
   The Fuji Super CCD is just a Bayer grid rotated 45 degrees.
 */
static void fuji_s2_load_raw(dcrawhandle *handle)
{
  ushort pixel[2944];
  int row, col, r, c;

  fseek (handle->ifp, (2944*24+32)*2, SEEK_CUR);
  for (row=0; row < 2144; row++) {
    fread (pixel, 2, 2944, handle->ifp);
    for (col=0; col < 2880; col++) {
      r = row + ((col+1) >> 1);
      c = 2143 - row + (col >> 1);
      BAYER(r,c) = ntohs(pixel[col]) << 2;
    }
  }
}

static void fuji_common_load_raw (dcrawhandle *handle, int ncol, int icol, int nrow)
{
  ushort pixel[2048];
  int row, col, r, c;

  for (row=0; row < nrow; row++) {
    fread (pixel, 2, ncol, handle->ifp);
    if (ntohs(0xaa55) == 0xaa55)	/* data is little-endian */
      swab ((void *)pixel, (void *)pixel, ncol*2);
    for (col=0; col <= icol; col++) {
      r = icol - col + (row >> 1);
      c = col + ((row+1) >> 1);
      BAYER(r,c) = pixel[col] << 2;
    }
  }
}

static void fuji_s5000_load_raw(dcrawhandle *handle)
{
  fseek (handle->ifp, (1472*4+24)*2, SEEK_CUR);
  fuji_common_load_raw (handle, 1472, 1423, 2152);
}

static void fuji_s7000_load_raw(dcrawhandle *handle)
{
  fuji_common_load_raw (handle, 2048, 2047, 3080);
}

/*
   The Fuji Super CCD SR has two photodiodes for each pixel.
   The secondary has about 1/16 the sensitivity of the primary,
   but this ratio may vary.
 */
static void fuji_f700_load_raw(dcrawhandle *handle)
{
  ushort pixel[2944];
  int row, col, r, c, val;

  for (row=0; row < 2168; row++) {
    fread (pixel, 2, 2944, handle->ifp);
    if (ntohs(0xaa55) == 0xaa55)	/* data is little-endian */
      swab ((void *)pixel, (void *)pixel, 2944*2);
    for (col=0; col < 1440; col++) {
      r = 1439 - col + (row >> 1);
      c = col + ((row+1) >> 1);
      val = pixel[col+16];
      if (val == 0x3fff) {		/* If the primary is maxed, */
	val = pixel[col+1488] << 4;	/* use the secondary.       */
	handle->rgb_max = 0xffff;
      }
      if (val > 0xffff)
	val = 0xffff;
      BAYER(r,c) = val;
    }
  }
}

static void rollei_load_raw(dcrawhandle *handle)
{
  uchar pixel[10];
  unsigned iten=0, isix, i, buffer=0, row, col, todo[16];

  isix = handle->raw_width * handle->raw_height * 5 / 8;
  while (fread (pixel, 1, 10, handle->ifp) == 10) {
    for (i=0; i < 10; i+=2) {
      todo[i]   = iten++;
      todo[i+1] = pixel[i] << 8 | pixel[i+1];
      buffer    = pixel[i] >> 2 | buffer << 6;
    }
    for (   ; i < 16; i+=2) {
      todo[i]   = isix++;
      todo[i+1] = buffer >> (14-i)*5;
    }
    for (i=0; i < 16; i+=2) {
      row = todo[i] / handle->raw_width - handle->top_margin;
      col = todo[i] % handle->raw_width - handle->left_margin;
      if (row < handle->height && col < handle->width)
	BAYER(row,col) = (todo[i+1] & 0x3ff) << 4;
    }
  }
}

static void phase_one_load_raw(dcrawhandle *handle)
{
  int row, col, a, b;
  ushort pixel[4134], akey, bkey;

  fseek (handle->ifp, 8, SEEK_CUR);
  fseek (handle->ifp, fget4(handle, handle->ifp) + 296, SEEK_CUR);
  akey = fget2(handle, handle->ifp);
  bkey = fget2(handle, handle->ifp);
  fseek (handle->ifp, handle->data_offset + 12 + handle->top_margin*handle->raw_width*2, SEEK_SET);
  for (row=0; row < handle->height; row++) {
    fread (pixel, 2, handle->raw_width, handle->ifp);
    for (col=0; col < handle->raw_width; col+=2) {
      a = ntohs(pixel[col+0]) ^ akey;
      b = ntohs(pixel[col+1]) ^ bkey;
      pixel[col+0] = (b & 0xaaaa) | (a & 0x5555);
      pixel[col+1] = (a & 0xaaaa) | (b & 0x5555);
    }
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = pixel[col+handle->left_margin];
  }
}

static void ixpress_load_raw(dcrawhandle *handle)
{
  ushort pixel[4090];
  int row, col;

  fseek (handle->ifp, 304 + 6*2*4090, SEEK_SET);
  for (row=handle->height; --row >= 0; ) {
    fread (pixel, 2, 4090, handle->ifp);
    if (ntohs(0xaa55) == 0xaa55)	/* data is little-endian */
      swab ((void *)pixel, (void *)pixel, 4090*2);
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = pixel[handle->width-1-col];
  }
}

/* For this function only, raw_width is in bytes, not pixels! */
static void packed_12_load_raw(dcrawhandle *handle)
{
  int row, col;

  getbits(handle, -1);
  for (row=0; row < handle->height; row++) {
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = getbits(handle, 12) << 2;
    for (col = handle->width*3/2; col < handle->raw_width; col++)
      getbits(handle, 8);
  }
}

static void unpacked_load_raw (dcrawhandle *handle, int order, int rsh)
{
  ushort *pixel;
  int row, col;

  pixel = calloc (handle->raw_width, sizeof *pixel);
  merror (handle, pixel, "unpacked_load_raw()");
  for (row=0; row < handle->height; row++) {
    fread (pixel, 2, handle->raw_width, handle->ifp);
    if (order != ntohs(0x55aa))
      swab ((void *)pixel, (void *)pixel, handle->width*2);
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = pixel[col] << 8 >> (8+rsh);
  }
  free(pixel);
}

static void be_16_load_raw(dcrawhandle *handle)		/* "be" = "big-endian" */
{
  unpacked_load_raw (handle, 0x55aa, 0);
}

static void be_high_12_load_raw(dcrawhandle *handle)
{
  unpacked_load_raw (handle, 0x55aa, 2);
}

static void be_low_12_load_raw(dcrawhandle *handle)
{
  unpacked_load_raw (handle, 0x55aa,-2);
}

static void be_low_10_load_raw(dcrawhandle *handle)
{
  unpacked_load_raw (handle, 0x55aa,-4);
}

static void le_high_12_load_raw(dcrawhandle *handle)	/* "le" = "little-endian" */
{
  unpacked_load_raw (handle, 0xaa55, 2);
}

static void olympus_cseries_load_raw(dcrawhandle *handle)
{
  int irow, row, col;

  for (irow=0; irow < handle->height; irow++) {
    row = irow * 2 % handle->height + irow / (handle->height/2);
    if (row < 2) {
      fseek (handle->ifp, handle->data_offset - row*(-handle->width*handle->height*3/4 & -2048), SEEK_SET);
      getbits(handle, -1);
    }
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = getbits(handle, 12) << 2;
  }
}

static void eight_bit_load_raw(dcrawhandle *handle)
{
  uchar *pixel;
  int row, col;

  pixel = calloc (handle->raw_width, sizeof *pixel);
  merror (handle, pixel, "eight_bit_load_raw()");
  for (row=0; row < handle->height; row++) {
    fread (pixel, 1, handle->raw_width, handle->ifp);
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = pixel[col] << 6;
  }
  free (pixel);
}

static void casio_qv5700_load_raw(dcrawhandle *handle)
{
  uchar  data[3232],  *dp;
  ushort pixel[2576], *pix;
  int row, col;

  for (row=0; row < handle->height; row++) {
    fread (data, 1, 3232, handle->ifp);
    for (dp=data, pix=pixel; dp < data+3220; dp+=5, pix+=4) {
      pix[0] = (dp[0] << 2) + (dp[1] >> 6);
      pix[1] = (dp[1] << 4) + (dp[2] >> 4);
      pix[2] = (dp[2] << 6) + (dp[3] >> 2);
      pix[3] = (dp[3] << 8) + (dp[4]     );
    }
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = (pixel[col] & 0x3ff) << 4;
  }
}

static void nucore_load_raw(dcrawhandle *handle)
{
  uchar *data, *dp;
  int irow, row, col;

  data = calloc (handle->width, 2);
  merror (handle, data, "nucore_load_raw()");
  for (irow=0; irow < handle->height; irow++) {
    fread (data, 2, handle->width, handle->ifp);
    if (handle->model[0] == 'B' && handle->width == 2598)
      row = handle->height - 1 - irow/2 - handle->height/2 * (irow & 1);
    else
      row = irow;
    for (dp=data, col=0; col < handle->width; col++, dp+=2)
      BAYER(row,col) = (dp[0] << 2) + (dp[1] << 10);
  }
  free(data);
}

static const int *make_decoder_int (dcrawhandle *handle, const int *source, int level)
{
  struct decode *cur;

  cur = free_decode++;
  if (level < source[0]) {
    cur->branch[0] = free_decode;
    source = make_decoder_int (handle, source, level+1);
    cur->branch[1] = free_decode;
    source = make_decoder_int (handle, source, level+1);
  } else {
    cur->leaf = source[1];
    source += 2;
  }
  return source;
}

static int radc_token (dcrawhandle *handle, int tree)
{
  int t;
  static struct decode *dstart[18], *dindex;
  static const int *s, source[] = {
    1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
    1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
    2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
    2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
    2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
    2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
    2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
    2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
    2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
    2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
    1,0, 2,2, 2,-2,
    1,-3, 1,3,
    2,-17, 2,-5, 2,5, 2,17,
    2,-7, 2,2, 2,9, 2,18,
    2,-18, 2,-9, 2,-2, 2,7,
    2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
    2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
    2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
  };

  if (free_decode == first_decode)
    for (s=source, t=0; t < 18; t++) {
      dstart[t] = free_decode;
      s = make_decoder_int (handle, s, 0);
    }
  if (tree == 18) {
    if (handle->model[2] == '4')
      return (getbits(handle, 5) << 3) + 4;	/* DC40 */
    else
      return (getbits(handle, 6) << 2) + 2;	/* DC50 */
  }
  for (dindex = dstart[tree]; dindex->branch[0]; )
    dindex = dindex->branch[getbits(handle, 1)];
  return dindex->leaf;
}

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

static void kodak_radc_load_raw(dcrawhandle *handle)
{
  int row, col, tree, nreps, rep, step, i, c, s, r, x, y, val;
  short last[3] = { 16,16,16 }, mul[3], buf[3][3][386];

  init_decoder();
  getbits(handle, -1);
  for (i=0; i < sizeof(buf)/sizeof(short); i++)
    buf[0][0][i] = 2048;
  for (row=0; row < handle->height; row+=4) {
    for (i=0; i < 3; i++)
      mul[i] = getbits(handle, 6);
    for (c=0; c < 3; c++) {
      val = ((0x1000000/last[c] + 0x7ff) >> 12) * mul[c];
      s = val > 65564 ? 10:12;
      x = ~(-1 << (s-1));
      val <<= 12-s;
      for (i=0; i < sizeof(buf[0])/sizeof(short); i++)
	buf[c][0][i] = (buf[c][0][i] * val + x) >> s;
      last[c] = mul[c];
      for (r=0; r <= !c; r++) {
	buf[c][1][handle->width/2] = buf[c][2][handle->width/2] = mul[c] << 7;
	for (tree=1, col=handle->width/2; col > 0; ) {
	  if ((tree = radc_token(handle, tree))) {
	    col -= 2;
	    if (tree == 8)
	      FORYX buf[c][y][x] = radc_token(handle, tree+10) * mul[c];
	    else
	      FORYX buf[c][y][x] = radc_token(handle, tree+10) * 16 + PREDICTOR;
	  } else
	    do {
	      nreps = (col > 2) ? radc_token(handle, 9) + 1 : 1;
	      for (rep=0; rep < 8 && rep < nreps && col > 0; rep++) {
		col -= 2;
		FORYX buf[c][y][x] = PREDICTOR;
		if (rep & 1) {
		  step = radc_token(handle, 10) << 4;
		  FORYX buf[c][y][x] += step;
		}
	      }
	    } while (nreps == 9);
	}
	for (y=0; y < 2; y++)
	  for (x=0; x < handle->width/2; x++) {
	    val = (buf[c][y+1][x] << 4) / mul[c];
	    if (val < 0) val = 0;
	    if (c)
	      BAYER(row+y*2+c-1,x*2+2-c) = val;
	    else
	      BAYER(row+r*2+y,x*2+y) = val;
	  }
	memcpy (buf[c][0]+!c, buf[c][2], sizeof buf[c][0]-2*!c);
      }
    }
    for (y=row; y < row+4; y++)
      for (x=0; x < handle->width; x++)
	if ((x+y) & 1) {
	  val = (BAYER(y,x)-2048)*2 + (BAYER(y,x-1)+BAYER(y,x+1))/2;
	  if (val < 0) val = 0;
	  BAYER(y,x) = val;
	}
  }
}

#undef FORYX
#undef PREDICTOR

#ifdef NO_JPEG
static void kodak_jpeg_load_raw(dcrawhandle *handle) {}
#else

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
  static char jpeg_buffer[4096];
  size_t nbytes;
  dcrawhandle *handle=(dcrawhandle *)(cinfo->client_data);

  nbytes = fread (jpeg_buffer, 1, 4096, handle->ifp);
  swab (jpeg_buffer, jpeg_buffer, nbytes);
  cinfo->src->next_input_byte = jpeg_buffer;
  cinfo->src->bytes_in_buffer = nbytes;
  return TRUE;
}

static void kodak_jpeg_load_raw(dcrawhandle *handle)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPARRAY buf;
  JSAMPLE (*pixel)[3];
  int row, col;

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  jpeg_stdio_src (&cinfo, handle->ifp);
  cinfo.src->fill_input_buffer = fill_input_buffer;
  cinfo.client_data=(void *)handle;
  jpeg_read_header (&cinfo, TRUE);
  jpeg_start_decompress (&cinfo);
  if ((cinfo.output_width      != handle->width  ) ||
      (cinfo.output_height*2   != handle->height ) ||
      (cinfo.output_components != 3      )) {
    fprintf (stderr, "%s: incorrect JPEG dimensions\n", handle->ifname);
    jpeg_destroy_decompress (&cinfo);
    longjmp (handle->failure, 3);
  }
  buf = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, handle->width*3, 1);

  while (cinfo.output_scanline < cinfo.output_height) {
    row = cinfo.output_scanline * 2;
    jpeg_read_scanlines (&cinfo, buf, 1);
    pixel = (void *) buf[0];
    for (col=0; col < handle->width; col+=2) {
      BAYER(row+0,col+0) = pixel[col+0][1] << 6;
      BAYER(row+1,col+1) = pixel[col+1][1] << 6;
      BAYER(row+0,col+1) = (pixel[col][0] + pixel[col+1][0]) << 5;
      BAYER(row+1,col+0) = (pixel[col][2] + pixel[col+1][2]) << 5;
    }
  }
  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);
}

#endif

static void kodak_dc120_load_raw(dcrawhandle *handle)
{
  static const int mul[4] = { 162, 192, 187,  92 };
  static const int add[4] = {   0, 636, 424, 212 };
  uchar pixel[848];
  int row, shift, col;

  for (row=0; row < handle->height; row++)
  {
    fread (pixel, 848, 1, handle->ifp);
    shift = row * mul[row & 3] + add[row & 3];
    for (col=0; col < handle->width; col++)
      BAYER(row,col) = (ushort) pixel[(col + shift) % 848] << 6;
  }
}

static void kodak_dc20_coeff (dcrawhandle *handle, float juice)
{
  static const float my_coeff[3][4] =
  { {  2.25,  0.75, -1.75, -0.25 },
    { -0.25,  0.75,  0.75, -0.25 },
    { -0.25, -1.75,  0.75,  2.25 } };
  static const float flat[3][4] =
  { {  1, 0,   0,   0 },
    {  0, 0.5, 0.5, 0 },
    {  0, 0,   0,   1 } };
  int r, g;

  for (r=0; r < 3; r++)
    for (g=0; g < 4; g++)
      handle->coeff[r][g] = my_coeff[r][g] * juice + flat[r][g] * (1-juice);
  handle->use_coeff = 1;
}

static void kodak_easy_load_raw(dcrawhandle *handle)
{
  uchar *pixel;
  ushort curve[0x1000];
  unsigned row, col, icol;

  kodak_curve (handle, curve);
  if (handle->raw_width > handle->width)
    handle->black = 0;
  pixel = calloc (handle->raw_width, sizeof *pixel);
  merror (handle, pixel, "kodak_easy_load_raw()");
  for (row=0; row < handle->height; row++) {
    fread (pixel, 1, handle->raw_width, handle->ifp);
    for (col=0; col < handle->raw_width; col++) {
      icol = col - handle->left_margin;
      if (icol < handle->width)
	BAYER(row,icol) = (ushort) curve[pixel[col]] << 2;
      else
	handle->black += curve[pixel[col]];
    }
  }
  if (handle->raw_width > handle->width)
    handle->black = ((INT64) handle->black << 2) / ((handle->raw_width - handle->width) * handle->height);
  if (!strncmp(handle->model,"DC2",3))
    handle->black = 0;
  free(pixel);
}

static void kodak_compressed_load_raw(dcrawhandle *handle)
{
  uchar c, blen[256];
  ushort raw[6], curve[0x1000];
  unsigned row, col, len, save, i, israw=0, bits=0, pred[2];
  INT64 bitbuf=0;
  int diff;

  kodak_curve (handle, curve);
  for (row=0; row < handle->height; row++)
    for (col=0; col < handle->width; col++)
    {
      if ((col & 255) == 0) {		/* Get the bit-lengths of the */
	len = handle->width - col;		/* next 256 pixel values      */
	if (len > 256) len = 256;
	save = ftell(handle->ifp);
	for (israw=i=0; i < len; i+=2) {
	  c = fgetc(handle->ifp);
	  if ((blen[i+0] = c & 15) > 12 ||
	      (blen[i+1] = c >> 4) > 12 )
	    israw = 1;
	}
	bitbuf = bits = pred[0] = pred[1] = 0;
	if (len % 8 == 4) {
	  bitbuf  = fgetc(handle->ifp) << 8;
	  bitbuf += fgetc(handle->ifp);
	  bits = 16;
	}
	if (israw)
	  fseek (handle->ifp, save, SEEK_SET);
      }
      if (israw) {			/* If the data is not compressed */
	switch (col & 7) {
	  case 0:
	    fread (raw, 2, 6, handle->ifp);
	    for (i=0; i < 6; i++)
	      raw[i] = ntohs(raw[i]);
	    diff = raw[0] >> 12 << 8 | raw[2] >> 12 << 4 | raw[4] >> 12;
	    break;
	  case 1:
	    diff = raw[1] >> 12 << 8 | raw[3] >> 12 << 4 | raw[5] >> 12;
	    break;
	  default:
	    diff = raw[(col & 7) - 2] & 0xfff;
	}
      } else {				/* If the data is compressed */
	len = blen[col & 255];		/* Number of bits for this pixel */
	if (bits < len) {		/* Got enough bits in the buffer? */
	  for (i=0; i < 32; i+=8)
	    bitbuf += (INT64) fgetc(handle->ifp) << (bits+(i^8));
	  bits += 32;
	}
	diff = bitbuf & (0xffff >> (16-len));  /* Pull bits from buffer */
	bitbuf >>= len;
	bits -= len;
	if ((diff & (1 << (len-1))) == 0)
	  diff -= (1 << len) - 1;
	pred[col & 1] += diff;
	diff = pred[col & 1];
      }
      BAYER(row,col) = curve[diff] << 2;
    }
}

static void kodak_yuv_load_raw(dcrawhandle *handle)
{
  uchar c, blen[384];
  unsigned row, col, len, bits=0;
  INT64 bitbuf=0;
  int i, li=0, si, diff, six[6], y[4], cb=0, cr=0, rgb[3];
  ushort *ip, curve[0x1000];

  kodak_curve (handle, curve);
  for (row=0; row < handle->height; row+=2)
    for (col=0; col < handle->width; col+=2) {
      if ((col & 127) == 0) {
	len = (handle->width - col + 1) * 3 & -4;
	if (len > 384) len = 384;
	for (i=0; i < len; ) {
	  c = fgetc(handle->ifp);
	  blen[i++] = c & 15;
	  blen[i++] = c >> 4;
	}
	li = bitbuf = bits = y[1] = y[3] = cb = cr = 0;
	if (len % 8 == 4) {
	  bitbuf  = fgetc(handle->ifp) << 8;
	  bitbuf += fgetc(handle->ifp);
	  bits = 16;
	}
      }
      for (si=0; si < 6; si++) {
	len = blen[li++];
	if (bits < len) {
	  for (i=0; i < 32; i+=8)
	    bitbuf += (INT64) fgetc(handle->ifp) << (bits+(i^8));
	  bits += 32;
	}
	diff = bitbuf & (0xffff >> (16-len));
	bitbuf >>= len;
	bits -= len;
	if ((diff & (1 << (len-1))) == 0)
	  diff -= (1 << len) - 1;
	six[si] = diff;
      }
      y[0] = six[0] + y[1];
      y[1] = six[1] + y[0];
      y[2] = six[2] + y[3];
      y[3] = six[3] + y[2];
      cb  += six[4];
      cr  += six[5];
      for (i=0; i < 4; i++) {
	ip = handle->image[(row+(i >> 1))*(handle->width) + col+(i & 1)];
	rgb[0] = y[i] + cr;
	rgb[1] = y[i];
	rgb[2] = y[i] + cb;
	for (c=0; c < 3; c++)
	  if (rgb[c] > 0) ip[c] = curve[rgb[c]] << 2;
      }
    }
}

static void sony_decrypt (unsigned *data, int len, int start, int key)
{
  static unsigned pad[128], p;

  if (start) {
    for (p=0; p < 4; p++)
      pad[p] = key = key * 48828125 + 1;
    pad[3] = pad[3] << 1 | (pad[0]^pad[2]) >> 31;
    for (p=4; p < 127; p++)
      pad[p] = (pad[p-4]^pad[p-2]) << 1 | (pad[p-3]^pad[p-1]) >> 31;
    for (p=0; p < 127; p++)
      pad[p] = htonl(pad[p]);
  }
  while (len--)
    *data++ ^= pad[p++ & 127] = pad[(p+1) & 127] ^ pad[(p+65) & 127];
}

static void sony_load_raw(dcrawhandle *handle)
{
  uchar head[40];
  ushort pixel[3360];
  unsigned i, key, row, col, icol;
  INT64 bblack=0;

  fseek (handle->ifp, 200896, SEEK_SET);
  fseek (handle->ifp, (unsigned) fgetc(handle->ifp)*4 - 1, SEEK_CUR);
  handle->order = 0x4d4d;
  key = fget4(handle, handle->ifp);
  fseek (handle->ifp, 164600, SEEK_SET);
  fread (head, 1, 40, handle->ifp);
  sony_decrypt ((void *) head, 10, 1, key);
  for (i=26; i-- > 22; )
    key = key << 8 | head[i];
  fseek (handle->ifp, 862144, SEEK_SET);
  for (row=0; row < handle->height; row++) {
    fread (pixel, 2, handle->raw_width, handle->ifp);
    sony_decrypt ((void *) pixel, handle->raw_width/2, !row, key);
    for (col=0; col < 3343; col++)
      if ((icol = col-handle->left_margin) < handle->width)
	BAYER(row,icol) = ntohs(pixel[col]);
      else
	bblack += ntohs(pixel[col]);
  }
  handle->black = bblack / ((3343 - handle->width) * handle->height);
}

static void sony_rgbe_coeff(dcrawhandle *handle)
{
  int r, g;
  static const float my_coeff[3][4] =
  { {  1.321918,  0.000000,  0.149829, -0.471747 },
    { -0.288764,  1.129213, -0.486517,  0.646067 },
    {  0.061336, -0.199343,  1.138007,  0.000000 } };

  for (r=0; r < 3; r++)
    for (g=0; g < 4; g++)
      handle->coeff[r][g] = my_coeff[r][g];
  handle->use_coeff = 1;
}

static void foveon_decoder (dcrawhandle *handle, unsigned huff[1024], unsigned code)
{
  struct decode *cur;
  int i, len;

  cur = free_decode++;
  if (free_decode > first_decode+2048) {
    fprintf (stderr, "%s: decoder table overflow\n", handle->ifname);
    longjmp (handle->failure, 2);
  }
  if (code) {
    for (i=0; i < 1024; i++)
      if (huff[i] == code) {
	cur->leaf = i;
	return;
      }
  }
  if ((len = code >> 27) > 26) return;
  code = (len+1) << 27 | (code & 0x3ffffff) << 1;

  cur->branch[0] = free_decode;
  foveon_decoder (handle, huff, code);
  cur->branch[1] = free_decode;
  foveon_decoder (handle, huff, code+1);
}

static void foveon_load_raw(dcrawhandle *handle)
{
  struct decode *dindex;
  short diff[1024], pred[3];
  unsigned huff[1024], bitbuf=0;
  int row, col, bit=-1, c, i;

  fseek (handle->ifp, 260, SEEK_SET);
  for (i=0; i < 1024; i++)
    diff[i] = fget2(handle, handle->ifp);
  for (i=0; i < 1024; i++)
    huff[i] = fget4(handle, handle->ifp);

  init_decoder();
  foveon_decoder (handle, huff, 0);

  for (row=0; row < handle->raw_height; row++) {
    memset (pred, 0, sizeof pred);
    if (!bit) fget4(handle, handle->ifp);
    for (col=bit=0; col < handle->raw_width; col++) {
      for (c=0; c < 3; c++) {
	for (dindex=first_decode; dindex->branch[0]; ) {
	  if ((bit = (bit-1) & 31) == 31)
	    for (i=0; i < 4; i++)
	      bitbuf = (bitbuf << 8) + fgetc(handle->ifp);
	  dindex = dindex->branch[bitbuf >> bit & 1];
	}
	pred[c] += diff[dindex->leaf];
      }
      if ((unsigned) (row-handle->top_margin)  >= handle->height ||
	  (unsigned) (col-handle->left_margin) >= handle->width ) continue;
      for (c=0; c < 3; c++)
	if (pred[c] > 0)
	  handle->image[(row-handle->top_margin)*handle->width+(col-handle->left_margin)][c] = pred[c];
    }
  }
}

static int apply_curve(dcrawhandle *handle, int i, const int *curve)
{
  if (i <= -curve[0])
    return -curve[curve[0]]-1;
  else if (i < 0)
    return -curve[1-i];
  else if (i < curve[0])
    return  curve[1+i];
  else
    return  curve[curve[0]]+1;
}

void foveon_interpolate(dcrawhandle *handle)
{
  float mul[3] =
  { 1.0321, 1.0, 1.1124 };
  static const int weight[3][3][3] =
  { { {   4141,  37726,  11265  },
      { -30437,  16066, -41102  },
      {    326,   -413,    362  } },
    { {   1770,  -1316,   3480  },
      {  -2139,    213,  -4998  },
      {  -2381,   3496,  -2008  } },
    { {  -3838, -24025, -12968  },
      {  20144, -12195,  30272  },
      {   -631,  -2025,    822  } } },
  curve1[73] = { 72,
     0,1,2,2,3,4,5,6,6,7,8,9,9,10,11,11,12,13,13,14,14,
    15,16,16,17,17,18,18,18,19,19,20,20,20,21,21,21,22,
    22,22,23,23,23,23,23,24,24,24,24,24,25,25,25,25,25,
    25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26 },
  curve2[21] = { 20,
    0,1,1,2,3,3,4,4,5,5,6,6,6,7,7,7,7,7,7,7 },
  curve3[73] = { 72,
     0,1,1,2,2,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,10,10,
    11,11,11,12,12,12,12,12,12,13,13,13,13,13,13,13,13,
    14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
    14,14,14,14,14,14,14,14,14,14,14,14,14,14,14 },
  curve4[37] = { 36,
    0,1,1,2,3,3,4,4,5,6,6,7,7,7,8,8,9,9,9,10,10,10,
    11,11,11,11,11,12,12,12,12,12,12,12,12,12 },
  curve5[111] = { 110,
    0,1,1,2,3,3,4,5,6,6,7,7,8,9,9,10,11,11,12,12,13,13,
    14,14,15,15,16,16,17,17,18,18,18,19,19,19,20,20,20,
    21,21,21,21,22,22,22,22,22,23,23,23,23,23,24,24,24,24,
    24,24,24,24,25,25,25,25,25,25,25,25,25,25,25,25,26,26,
    26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,
    26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26 },
  *curves[3] = { curve3, curve4, curve5 },
  trans[3][3] =
  { {   7576,  -2933,   1279  },
    { -11594,  29911, -12394  },
    {   4000, -18850,  20772  } };
  ushort *pix, prev[3], (*shrink)[3];
  int row, col, c, i, j, diff, sum, ipix[3], work[3][3], total[4];
  int (*smrow[7])[3], smlast, smred, smred_p=0, hood[7], min, max;

  /* Sharpen all colors */
  for (row=0; row < handle->height; row++) {
    pix = handle->image[row*handle->width];
    memcpy (prev, pix, sizeof prev);
    for (col=0; col < handle->width; col++) {
      for (c=0; c < 3; c++) {
	diff = pix[c] - prev[c];
	prev[c] = pix[c];
	ipix[c] = pix[c] + ((diff + (diff*diff >> 14)) * 0x3333 >> 14);
      }
      for (c=0; c < 3; c++) {
	work[0][c] = ipix[c]*ipix[c] >> 14;
	work[2][c] = ipix[c]*work[0][c] >> 14;
	work[1][2-c] = ipix[(c+1) % 3] * ipix[(c+2) % 3] >> 14;
      }
      for (c=0; c < 3; c++) {
	for (sum=i=0; i < 3; i++)
	  for (  j=0; j < 3; j++)
	    sum += weight[c][i][j] * work[i][j];
	ipix[c] = (ipix[c] + (sum >> 14)) * mul[c];
	if (ipix[c] < 0)     ipix[c] = 0;
	if (ipix[c] > 32000) ipix[c] = 32000;
	pix[c] = ipix[c];
      }
      pix += 4;
    }
  }
  /* Array for 5x5 Gaussian averaging of red values */
  smrow[6] = calloc (handle->width*5, sizeof **smrow);
  merror (handle, smrow[6], "foveon_interpolate()");
  for (i=0; i < 5; i++)
    smrow[i] = smrow[6] + i*handle->width;

  /* Sharpen the reds against these Gaussian averages */
  for (smlast=-1, row=2; row < handle->height-2; row++) {
    while (smlast < row+2) {
      for (i=0; i < 6; i++)
	smrow[(i+5) % 6] = smrow[i];
      pix = handle->image[++smlast*handle->width+2];
      for (col=2; col < handle->width-2; col++) {
	smrow[4][col][0] =
	  (pix[0]*6 + (pix[-4]+pix[4])*4 + pix[-8]+pix[8] + 8) >> 4;
	pix += 4;
      }
    }
    pix = handle->image[row*handle->width+2];
    for (col=2; col < handle->width-2; col++) {
      smred = (smrow[2][col][0]*6 + (smrow[1][col][0]+smrow[3][col][0])*4
		+ smrow[0][col][0]+smrow[4][col][0] + 8) >> 4;
      if (col == 2)
	smred_p = smred;
      i = pix[0] + ((pix[0] - ((smred*7 + smred_p) >> 3)) >> 2);
      if (i < 0)     i = 0;
      if (i > 10000) i = 10000;
      pix[0] = i;
      smred_p = smred;
      pix += 4;
    }
  }
  /* Limit each color value to the range of its neighbors */
  hood[0] = 4;
  for (i=0; i < 3; i++) {
    hood[i+1] = (i-handle->width-1)*4;
    hood[i+4] = (i+handle->width-1)*4;
  }
  for (row=1; row < handle->height-1; row++) {
    pix = handle->image[row*handle->width+1];
    memcpy (prev, pix-4, sizeof prev);
    for (col=1; col < handle->width-1; col++) {
      for (c=0; c < 3; c++) {
	for (min=max=prev[c], i=0; i < 7; i++) {
	  j = pix[hood[i]];
	  if (min > j) min = j;
	  if (max < j) max = j;
	}
	prev[c] = *pix;
	if (*pix < min) *pix = min;
	if (*pix > max) *pix = max;
	pix++;
      }
      pix++;
    }
  }
/*
   Because photons that miss one detector often hit another,
   the sum R+G+B is much less noisy than the individual colors.
   So smooth the hues without smoothing the total.
 */
  for (smlast=-1, row=2; row < handle->height-2; row++) {
    while (smlast < row+2) {
      for (i=0; i < 6; i++)
	smrow[(i+5) % 6] = smrow[i];
      pix = handle->image[++smlast*handle->width+2];
      for (col=2; col < handle->width-2; col++) {
	for (c=0; c < 3; c++)
	  smrow[4][col][c] = pix[c-8]+pix[c-4]+pix[c]+pix[c+4]+pix[c+8];
	pix += 4;
      }
    }
    pix = handle->image[row*handle->width+2];
    for (col=2; col < handle->width-2; col++) {
      for (total[3]=1500, sum=60, c=0; c < 3; c++) {
	for (total[c]=i=0; i < 5; i++)
	  total[c] += smrow[i][col][c];
	total[3] += total[c];
	sum += pix[c];
      }
      j = (sum << 16) / total[3];
      for (c=0; c < 3; c++) {
	i = apply_curve (handle, (total[c] * j >> 16) - pix[c], curve1);
	i += pix[c] - 13 - (c==1);
	ipix[c] = i - apply_curve (handle, i, curve2);
      }
      sum = (ipix[0]+ipix[1]+ipix[1]+ipix[2]) >> 2;
      for (c=0; c < 3; c++) {
	i = ipix[c] - apply_curve (handle, ipix[c] - sum, curve2);
	if (i < 0) i = 0;
	pix[c] = i;
      }
      pix += 4;
    }
  }
  /* Translate the image to a different colorspace */
  for (pix=handle->image[0]; pix < handle->image[handle->height*handle->width]; pix+=4) {
    for (c=0; c < 3; c++) {
      for (i=j=0; j < 3; j++)
	i += trans[c][j] * pix[j];
      i = (i+0x1000) >> 13;
      if (i < 0)     i = 0;
      if (i > 24000) i = 24000;
      ipix[c] = i;
    }
    for (c=0; c < 3; c++)
      pix[c] = ipix[c];
  }
  /* Smooth the image bottom-to-top and save at 1/4 scale */
  shrink = calloc ((handle->width/4) * (handle->height/4), sizeof *shrink);
  merror (handle, shrink, "foveon_interpolate()");
  for (row = handle->height/4; row--; )
    for (col=0; col < handle->width/4; col++) {
      ipix[0] = ipix[1] = ipix[2] = 0;
      for (i=0; i < 4; i++)
	for (j=0; j < 4; j++)
	  for (c=0; c < 3; c++)
	    ipix[c] += handle->image[(row*4+i)*handle->width+col*4+j][c];
      for (c=0; c < 3; c++)
	if (row+2 > handle->height/4)
	  shrink[row*(handle->width/4)+col][c] = ipix[c] >> 4;
	else
	  shrink[row*(handle->width/4)+col][c] =
	    (shrink[(row+1)*(handle->width/4)+col][c]*1840 + ipix[c]*141) >> 12;
    }

  /* From the 1/4-scale image, smooth right-to-left */
  for (row=0; row < (handle->height & ~3); row++) {
    ipix[0] = ipix[1] = ipix[2] = 0;
    if ((row & 3) == 0)
      for (col = handle->width & ~3 ; col--; )
	for (c=0; c < 3; c++)
	  smrow[0][col][c] = ipix[c] =
	    (shrink[(row/4)*(handle->width/4)+col/4][c]*1485 + ipix[c]*6707) >> 13;

  /* Then smooth left-to-right */
    ipix[0] = ipix[1] = ipix[2] = 0;
    for (col=0; col < (handle->width & ~3); col++)
      for (c=0; c < 3; c++)
	smrow[1][col][c] = ipix[c] =
	  (smrow[0][col][c]*1485 + ipix[c]*6707) >> 13;

  /* Smooth top-to-bottom */
    if (row == 0)
      memcpy (smrow[2], smrow[1], sizeof **smrow * handle->width);
    else
      for (col=0; col < (handle->width & ~3); col++)
	for (c=0; c < 3; c++)
	  smrow[2][col][c] =
	    (smrow[2][col][c]*6707 + smrow[1][col][c]*1485) >> 13;

  /* Adjust the chroma toward the smooth values */
    for (col=0; col < (handle->width & ~3); col++) {
      for (i=j=60, c=0; c < 3; c++) {
	i += smrow[2][col][c];
	j += handle->image[row*handle->width+col][c];
      }
      j = (j << 16) / i;
      for (sum=c=0; c < 3; c++) {
	i = (smrow[2][col][c] * j >> 16) - handle->image[row*handle->width+col][c];
	ipix[c] = apply_curve (handle, i, curves[c]);
	sum += ipix[c];
      }
      sum >>= 3;
      for (c=0; c < 3; c++) {
	i = handle->image[row*handle->width+col][c] + ipix[c] - sum;
	if (i < 0) i = 0;
	handle->image[row*handle->width+col][c] = i;
      }
    }
  }
  free(shrink);
  free(smrow[6]);
}

/*
   Seach from the current directory up to the root looking for
   a ".badpixels" file, and fix those pixels now.
 */
void bad_pixels(dcrawhandle *handle)
{
  FILE *fp=NULL;
  char *fname, *cp, line[128];
  int len, time, row, col, r, c, rad, tot, n, fixed=0;

  if (!handle->filters) return;
  for (len=16 ; ; len *= 2) {
    fname = malloc (len);
    if (!fname) return;
    if (getcwd (fname, len-12)) break;
    free (fname);
    if (errno != ERANGE) return;
  }
  cp = fname + strlen(fname);
  if (cp[-1] == '/') cp--;
  while (*fname == '/') {
    strcpy (cp, "/.badpixels");
    if ((fp = fopen (fname, "r"))) break;
    if (cp == fname) break;
    while (*--cp != '/');
  }
  free (fname);
  if (!fp) return;
  while (1) {
    fgets (line, 128, fp);
    if (feof(fp)) break;
    cp = strchr (line, '#');
    if (cp) *cp = 0;
    if (sscanf (line, "%d %d %d", &col, &row, &time) != 3) continue;
    if ((unsigned) col >= handle->width || (unsigned) row >= handle->height) continue;
    if (time > handle->timestamp) continue;
    for (tot=n=0, rad=1; rad < 3 && n==0; rad++)
      for (r = row-rad; r <= row+rad; r++)
	for (c = col-rad; c <= col+rad; c++)
	  if ((unsigned) r < handle->height && (unsigned) c < handle->width &&
		(r != row || c != col) && FC(r,c) == FC(row,col)) {
	    tot += BAYER(r,c);
	    n++;
	  }
    BAYER(row,col) = tot/n;
    if (handle->verbose) {
      if (!fixed++)
	fprintf (stderr, "Fixed bad pixels at:");
      fprintf (stderr, " %d,%d", col, row);
    }
  }
  if (fixed) fputc ('\n', stderr);
  fclose (fp);
}

void scale_colors(dcrawhandle *handle)
{
  int row, col, c, val;
  int min[4], max[4], count[4];
  double sum[4], dmin, dmax;

  handle->rgb_max -= handle->black;
  if (handle->use_auto_wb || (handle->use_camera_wb && handle->camera_red == -1)) {
    for (c=0; c < 4; c++) {
      min[c] = INT_MAX;
      max[c] = count[c] = sum[c] = 0;
    }
    for (row=0; row < handle->height; row++)
      for (col=0; col < handle->width; col++)
	for (c=0; c < handle->colors; c++) {
	  val = handle->image[row*handle->width+col][c];
	  if (!val) continue;
	  val -= handle->black;
	  if (val > handle->rgb_max-100) continue;
	  if (val < 0) val = 0;
	  if (min[c] > val) min[c] = val;
	  if (max[c] < val) max[c] = val;
	  sum[c] += val;
	  count[c]++;
	}
    for (dmax=c=0; c < handle->colors; c++) {
      sum[c] /= count[c];
      if (dmax < sum[c]) dmax = sum[c];
    }
    for (c=0; c < handle->colors; c++)
      handle->pre_mul[c] = dmax / sum[c];
  }
  if (handle->use_camera_wb && handle->camera_red != -1) {
    for (c=0; c < 4; c++)
      count[c] = sum[c] = 0;
    for (row=0; row < 8; row++)
      for (col=0; col < 8; col++) {
	c = FC(row,col);
	if ((val = handle->white[row][col] - handle->black) > 0)
	  sum[c] += val;
	count[c]++;
      }
    for (dmin=DBL_MAX, dmax=c=0; c < handle->colors; c++) {
      sum[c] /= count[c];
      if (dmin > sum[c]) dmin = sum[c];
      if (dmax < sum[c]) dmax = sum[c];
    }
    if (dmin > 0)
      for (c=0; c < handle->colors; c++)
	handle->pre_mul[c] = dmax / sum[c];
    else if (handle->camera_red && handle->camera_blue) {
      handle->pre_mul[0] = handle->camera_red;
      handle->pre_mul[2] = handle->camera_blue;
      handle->pre_mul[1] = handle->pre_mul[3] = 1.0;
    } else
      fprintf (stderr, "%s: Cannot use camera white balance.\n", handle->ifname);
  }
  if (!handle->use_coeff) {
    handle->pre_mul[0] *= handle->red_scale;
    handle->pre_mul[2] *= handle->blue_scale;
  }
  if (handle->verbose) {
    fprintf (stderr, "Scaling with black=%d, pre_mul[] =", handle->black);
    for (c=0; c < handle->colors; c++)
      fprintf (stderr, " %f", handle->pre_mul[c]);
    fputc ('\n', stderr);
  }
  for (row=0; row < handle->height; row++)
    for (col=0; col < handle->width; col++)
      for (c=0; c < handle->colors; c++) {
	val = handle->image[row*handle->width+col][c];
	if (!val) continue;
	val -= handle->black;
	val *= handle->pre_mul[c];
	if (val < 0) val = 0;
	if (val > handle->rgb_max) val = handle->rgb_max;
	handle->image[row*handle->width+col][c] = val;
      }
}

/*
   This algorithm is officially called:

   "Interpolation using a Threshold-based variable number of gradients"

   described in http://www-ise.stanford.edu/~tingchen/algodep/vargra.html

   I've extended the basic idea to work with non-Bayer filter arrays.
   Gradients are numbered clockwise from NW=0 to W=7.
 */
void vng_interpolate(dcrawhandle *handle)
{
  static const signed char *cp, terms[] = {
    -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
    -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,0,0x01,
    -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
    -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,0,0x04, -2,+1,+0,+0,0,0x06,
    -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
    -1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
    -1,-2,+1,+0,0,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
    -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
    -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
    -1,+0,+1,-2,0,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
    -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,0,0x10, -1,+1,+1,-1,1,0x44,
    -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
    -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,0,0x04, -1,+2,+1,+1,0,0x04,
    +0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
    +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
    +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,0,0x10, +0,+0,+0,+2,1,0x08,
    +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
    +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
    +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,0,0x40, +0,+1,+2,+0,0,0x60,
    +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
    +1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
    +1,+0,+2,+1,0,0x10
  }, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
  ushort (*brow[5])[4], *pix;
  int code[8][640], *ip, gval[8], gmin, gmax, sum[4];
  int row, col, shift, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;

  for (row=0; row < 8; row++) {		/* Precalculate for bilinear */
    ip = code[row];
    for (col=1; col < 3; col++) {
      memset (sum, 0, sizeof sum);
      for (y=-1; y <= 1; y++)
	for (x=-1; x <= 1; x++) {
	  shift = (y==0) + (x==0);
	  if (shift == 2) continue;
	  color = FC(row+y,col+x);
	  *ip++ = (handle->width*y + x)*4 + color;
	  *ip++ = shift;
	  *ip++ = color;
	  sum[color] += 1 << shift;
	}
      for (c=0; c < handle->colors; c++)
	if (c != FC(row,col)) {
	  *ip++ = c;
	  *ip++ = sum[c];
	}
    }
  }
  for (row=1; row < handle->height-1; row++) {	/* Do bilinear interpolation */
    pix = handle->image[row*handle->width+1];
    for (col=1; col < handle->width-1; col++) {
      if (col & 1)
	ip = code[row & 7];
      memset (sum, 0, sizeof sum);
      for (g=8; g--; ) {
	diff = pix[*ip++];
	diff <<= *ip++;
	sum[*ip++] += diff;
      }
      for (g=handle->colors; --g; ) {
	c = *ip++;
	pix[c] = sum[c] / *ip++;
      }
      pix += 4;
    }
  }
  if (handle->quick_interpolate)
    return;

  for (row=0; row < 8; row++) {		/* Precalculate for VNG */
    ip = code[row];
    for (col=0; col < 2; col++) {
      for (cp=terms, t=0; t < 64; t++) {
	y1 = *cp++;  x1 = *cp++;
	y2 = *cp++;  x2 = *cp++;
	weight = *cp++;
	grads = *cp++;
	color = FC(row+y1,col+x1);
	if (FC(row+y2,col+x2) != color) continue;
	diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
	if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
	*ip++ = (y1*handle->width + x1)*4 + color;
	*ip++ = (y2*handle->width + x2)*4 + color;
	*ip++ = weight;
	for (g=0; g < 8; g++)
	  if (grads & 1<<g) *ip++ = g;
	*ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=chood, g=0; g < 8; g++) {
	y = *cp++;  x = *cp++;
	*ip++ = (y*handle->width + x) * 4;
	color = FC(row,col);
	if ((g & 1) == 0 &&
	    FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
	  *ip++ = (y*handle->width + x) * 8 + color;
	else
	  *ip++ = 0;
      }
    }
  }
  brow[4] = calloc (handle->width*3, sizeof **brow);
  merror (handle, brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*handle->width;
  for (row=2; row < handle->height-2; row++) {		/* Do VNG interpolation */
    pix = handle->image[row*handle->width+2];
    for (col=2; col < handle->width-2; col++) {
      if ((col & 1) == 0)
	ip = code[row & 7];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
	num = (diff = pix[g] - pix[ip[1]]) >> 31;
	gval[ip[3]] += (diff = ((diff ^ num) - num) << ip[2]);
	ip += 5;
	if ((g = ip[-1]) == -1) continue;
	gval[g] += diff;
	while ((g = *ip++) != -1)
	  gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];			/* Choose a threshold */
      for (g=1; g < 8; g++) {
	if (gmin > gval[g]) gmin = gval[g];
	if (gmax < gval[g]) gmax = gval[g];
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = FC(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
	if (gval[g] <= thold) {
	  for (c=0; c < handle->colors; c++)
	    if (c == color && ip[1])
	      sum[c] += (pix[c] + pix[ip[1]]) >> 1;
	    else
	      sum[c] += pix[ip[0] + c];
	  num++;
	}
      }
      for (c=0; c < handle->colors; c++) {		/* Save to buffer */
	t = pix[color];
	if (c != color) {
	  t += (sum[c] - sum[color])/num;
	  if (t < 0) t = 0;
	  if (t > handle->rgb_max) t = handle->rgb_max;
	}
	brow[2][col][c] = t;
      }
      pix += 4;
    }
    if (row > 3)				/* Write buffer to image */
      memcpy (handle->image[(row-2)*handle->width+2], brow[0]+2, (handle->width-4)*sizeof *handle->image);
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (handle->image[(row-2)*handle->width+2], brow[0]+2, (handle->width-4)*sizeof *handle->image);
  memcpy (handle->image[(row-1)*handle->width+2], brow[1]+2, (handle->width-4)*sizeof *handle->image);
  free(brow[4]);
}

static void tiff_parse_subifd(dcrawhandle *handle, int base)
{
  int entries, tag, type, len, val, save;

  entries = fget2(handle, handle->ifp);
  while (entries--) {
    tag  = fget2(handle, handle->ifp);
    type = fget2(handle, handle->ifp);
    len  = fget4(handle, handle->ifp);
    if (type == 3 && len < 3) {
      val = fget2(handle, handle->ifp);  fget2(handle, handle->ifp);
    } else
      val = fget4(handle, handle->ifp);
    switch (tag) {
      case 0x100:		/* ImageWidth */
	handle->raw_width = val;
	break;
      case 0x101:		/* ImageHeight */
	handle->raw_height = val;
	break;
      case 0x102:		/* Bits per sample */
	break;
      case 0x103:		/* Compression */
	handle->tiff_data_compression = val;
	break;
      case 0x106:		/* Kodak color format */
	handle->kodak_data_compression = val;
	break;
      case 0x111:		/* StripOffset */
	if (len == 1)
	  handle->data_offset = val;
	else {
	  save = ftell(handle->ifp);
	  fseek (handle->ifp, val+base, SEEK_SET);
	  handle->data_offset = fget4(handle, handle->ifp);
	  fseek (handle->ifp, save, SEEK_SET);
	}
	break;
      case 0x115:		/* SamplesPerRow */
	break;
      case 0x116:		/* RowsPerStrip */
	break;
      case 0x117:		/* StripByteCounts */
	break;
      case 0x123:
	handle->curve_offset = val;
	handle->curve_length = len;
    }
  }
}

static void nef_parse_makernote(dcrawhandle *handle)
{
  int base=0, offset=0, entries, tag, type, len, val, save;
  short sorder;
  char buf[10];

/*
   The MakerNote might have its own TIFF header (possibly with
   its own byte-order!), or it might just be a table.
 */
  sorder = handle->order;
  fread (buf, 1, 10, handle->ifp);
  if (!strcmp (buf,"Nikon")) {	/* starts with "Nikon\0\2\0\0\0" ? */
    base = ftell(handle->ifp);
    handle->order = fget2(handle, handle->ifp);		/* might differ from file-wide byteorder */
    val = fget2(handle, handle->ifp);		/* should be 42 decimal */
    offset = fget4(handle, handle->ifp);
    fseek (handle->ifp, offset-8, SEEK_CUR);
  } else if (!strcmp (buf,"OLYMP"))
    fseek (handle->ifp, -2, SEEK_CUR);
  else
    fseek (handle->ifp, -10, SEEK_CUR);

  entries = fget2(handle, handle->ifp);
  while (entries--) {
    tag  = fget2(handle, handle->ifp);
    type = fget2(handle, handle->ifp);
    len  = fget4(handle, handle->ifp);
    if (type == 3) {            /* short int */
      val = fget2(handle, handle->ifp);  fget2(handle, handle->ifp);
    } else
      val = fget4(handle, handle->ifp);
    save = ftell(handle->ifp);
    if (tag == 0xc) {
      fseek (handle->ifp, base+val, SEEK_SET);
      handle->camera_red  = fget4(handle, handle->ifp);
      handle->camera_red /= fget4(handle, handle->ifp);
      handle->camera_blue = fget4(handle, handle->ifp);
      handle->camera_blue/= fget4(handle, handle->ifp);
    }
    if (tag == 0x8c)
      handle->curve_offset = base+val + 2112;
    if (tag == 0x96)
      handle->curve_offset = base+val + 2;
    if (tag == 0x97) {
      if (!strcmp(handle->model,"NIKON D100 ")) {
	fseek (handle->ifp, base+val + 72, SEEK_SET);
	handle->camera_red  = fget2(handle, handle->ifp) / 256.0;
	handle->camera_blue = fget2(handle, handle->ifp) / 256.0;
      } else if (!strcmp(handle->model,"NIKON D2H")) {
	fseek (handle->ifp, base+val + 10, SEEK_SET);
	handle->camera_red  = fget2(handle, handle->ifp);
	handle->camera_red /= fget2(handle, handle->ifp);
	handle->camera_blue = fget2(handle, handle->ifp);
	handle->camera_blue = fget2(handle, handle->ifp) / handle->camera_blue;
      } else if (!strcmp(handle->model,"NIKON D70")) {
	fseek (handle->ifp, base+val + 20, SEEK_SET);
	handle->camera_red  = fget2(handle, handle->ifp);
	handle->camera_red /= fget2(handle, handle->ifp);
	handle->camera_blue = fget2(handle, handle->ifp);
	handle->camera_blue/= fget2(handle, handle->ifp);
      }
    }
    if (tag == 0x1017)		/* Olympus */
      handle->camera_red  = val / 256.0;
    if (tag == 0x1018)
      handle->camera_blue = val / 256.0;
    fseek (handle->ifp, save, SEEK_SET);
  }
  handle->order = sorder;
}

static void nef_parse_exif(dcrawhandle *handle)
{
  int entries, tag, type, len, val, save;

  entries = fget2(handle, handle->ifp);
  while (entries--) {
    tag  = fget2(handle, handle->ifp);
    type = fget2(handle, handle->ifp);
    len  = fget4(handle, handle->ifp);
    val  = fget4(handle, handle->ifp);
    save = ftell(handle->ifp);
    if (tag == 0x927c &&
	(!strncmp(handle->make,"NIKON",5) || !strncmp(handle->make,"OLYMPUS",7))) {
      fseek (handle->ifp, val, SEEK_SET);
      nef_parse_makernote(handle);
      fseek (handle->ifp, save, SEEK_SET);
    }
  }
}

/*
   Parse a TIFF file looking for camera model and decompress offsets.
 */
static void parse_tiff(dcrawhandle *handle, int base)
{
  int doff, entries, tag, type, len, val, save;
  char software[64];
  int wide=0, high=0, cr2_offset=0, offset=0;

  fseek (handle->ifp, base, SEEK_SET);
  handle->order = fget2(handle, handle->ifp);
  val = fget2(handle, handle->ifp);		/* Should be 42 for standard TIFF */
  while ((doff = fget4(handle, handle->ifp))) {
    fseek (handle->ifp, doff+base, SEEK_SET);
    entries = fget2(handle, handle->ifp);
    while (entries--) {
      tag  = fget2(handle, handle->ifp);
      type = fget2(handle, handle->ifp);
      len  = fget4(handle, handle->ifp);
      if (type == 3 && len < 3) {
	val = fget2(handle, handle->ifp);  fget2(handle, handle->ifp);
      } else
	val = fget4(handle, handle->ifp);
      save = ftell(handle->ifp);
      fseek (handle->ifp, val+base, SEEK_SET);
      switch (tag) {
	case 0x11:
	  handle->camera_red  = val / 256.0;
	  break;
	case 0x12:
	  handle->camera_blue = val / 256.0;
	  break;
	case 0x100:			/* ImageWidth */
	  wide = val;
	  break;
	case 0x101:			/* ImageHeight */
	  high = val;
	  break;
	case 0x10f:			/* Make tag */
	  fgets (handle->make, 64, handle->ifp);
	  break;
	case 0x110:			/* Model tag */
	  fgets (handle->model, 64, handle->ifp);
	  break;
	case 0x111:			/* StripOffset */
	  cr2_offset = val;
	  offset = fget4(handle, handle->ifp);
	  break;
	case 0x123:
	  handle->curve_offset = val;
	  handle->curve_length = len;
	  break;
	case 0x827d:			/* Model2 tag */
	  fgets (handle->model2, 64, handle->ifp);
	  break;
	case 0x131:			/* Software tag */
	  fgets (software, 64, handle->ifp);
	  if (!strncmp(software,"Adobe",5))
	    handle->make[0] = 0;
	  break;
	case 0x144:
	  strcpy (handle->make, "Leaf");
	  handle->raw_width  = wide;
	  handle->raw_height = high;
	  if (len > 1)
	    handle->data_offset = fget4(handle, handle->ifp);
	  else
	    handle->data_offset = val;
	  break;
	case 0x14a:			/* SubIFD tag */
	  if (len > 2 && !strcmp(handle->make,"Kodak"))
	      len = 2;
	  if (len > 1)
	    while (len--) {
	      fseek (handle->ifp, val+base, SEEK_SET);
	      fseek (handle->ifp, fget4(handle, handle->ifp)+base, SEEK_SET);
	      tiff_parse_subifd(handle, base);
	      val += 4;
	    }
	  else
	    tiff_parse_subifd(handle, base);
	  break;
	case 0x8769:			/* Nikon EXIF tag */
	  nef_parse_exif(handle);
	  break;
      }
      fseek (handle->ifp, save, SEEK_SET);
    }
  }
  if (!strncmp(handle->make,"OLYMPUS",7)) {
    handle->make[7] = 0;
    handle->raw_width = wide;
    handle->raw_height = - (-high & -2);
    handle->data_offset = offset;
  }
  if (!strcmp(handle->model,"Canon EOS-1D Mark II"))
    handle->data_offset = cr2_offset;

  if (handle->make[0] == 0 && wide == 680 && high == 680) {
    strcpy (handle->make, "Imacon");
    strcpy (handle->model, "Ixpress");
  }
}

/*
   CIFF block 0x1030 contains an 8x8 white sample.
   Load this into white[][] for use in scale_colors().
 */
static void ciff_block_1030(dcrawhandle *handle)
{
  static const ushort key[] = { 0x410, 0x45f3 };
  int i, bpp, row, col, vbits=0;
  unsigned long bitbuf=0;

  fget2(handle, handle->ifp);
  if (fget4(handle, handle->ifp) != 0x80008) return;
  if (fget4(handle, handle->ifp) == 0) return;
  bpp = fget2(handle, handle->ifp);
  if (bpp != 10 && bpp != 12) return;
  for (i=row=0; row < 8; row++)
    for (col=0; col < 8; col++) {
      if (vbits < bpp) {
	bitbuf = bitbuf << 16 | (fget2(handle, handle->ifp) ^ key[i++ & 1]);
	vbits += 16;
      }
      handle->white[row][col] =
	bitbuf << (LONG_BIT - vbits) >> (LONG_BIT - bpp) << (14-bpp);
      vbits -= bpp;
    }
}

/*
   Parse the CIFF structure looking for two pieces of information:
   The camera model, and the decode table number.
 */
static void parse_ciff(dcrawhandle *handle, int offset, int length)
{
  int tboff, nrecs, i, type, len, roff, aoff, save, wbi=-1;
  static const int remap[] = { 1,2,3,4,5,1 };
  static const int remap_10d[] = { 0,1,3,4,5,6,0,0,2,8 };

  fseek (handle->ifp, offset+length-4, SEEK_SET);
  tboff = fget4(handle, handle->ifp) + offset;
  fseek (handle->ifp, tboff, SEEK_SET);
  nrecs = fget2(handle, handle->ifp);
  for (i = 0; i < nrecs; i++) {
    type = fget2(handle, handle->ifp);
    len  = fget4(handle, handle->ifp);
    roff = fget4(handle, handle->ifp);
    aoff = offset + roff;
    save = ftell(handle->ifp);
    if (type == 0x080a) {		/* Get the camera make and model */
      fseek (handle->ifp, aoff, SEEK_SET);
      fread (handle->make, 64, 1, handle->ifp);
      fseek (handle->ifp, aoff+strlen(handle->make)+1, SEEK_SET);
      fread (handle->model, 64, 1, handle->ifp);
    }
    if (type == 0x102a) {		/* Find the White Balance index */
      fseek (handle->ifp, aoff+14, SEEK_SET);	/* 0=auto, 1=daylight, 2=cloudy ... */
      wbi = fget2(handle, handle->ifp);
      if (((!strcmp(handle->model,"Canon EOS DIGITAL REBEL") ||
	    !strcmp(handle->model,"Canon EOS 300D DIGITAL"))) && wbi == 6)
	wbi++;
    }
    if (type == 0x102c) {		/* Get white balance (G2) */
      if (!strcmp(handle->model,"Canon PowerShot G1") ||
	  !strcmp(handle->model,"Canon PowerShot Pro90 IS")) {
	fseek (handle->ifp, aoff+120, SEEK_SET);
	handle->white[0][1] = fget2(handle, handle->ifp) << 4;
	handle->white[0][0] = fget2(handle, handle->ifp) << 4;
	handle->white[1][0] = fget2(handle, handle->ifp) << 4;
	handle->white[1][1] = fget2(handle, handle->ifp) << 4;
      } else {
	fseek (handle->ifp, aoff+100, SEEK_SET);
	if (wbi == 6 && fget4(handle, handle->ifp) == 0)
	  wbi = 15;
	else {
	  fseek (handle->ifp, aoff+100, SEEK_SET);
	  goto common;
	}
      }
    }
    if (type == 0x0032) {		/* Get white balance (D30 & G3) */
      if (!strcmp(handle->model,"Canon EOS D30")) {
	fseek (handle->ifp, aoff+72, SEEK_SET);
common:
	handle->camera_red   = fget2(handle, handle->ifp);
	handle->camera_red   = fget2(handle, handle->ifp) / handle->camera_red;
	handle->camera_blue  = fget2(handle, handle->ifp);
	handle->camera_blue /= fget2(handle, handle->ifp);
      } else {
	fseek (handle->ifp, aoff+80 + (wbi < 6 ? remap[wbi]*8 : 0), SEEK_SET);
        if (!handle->camera_red)
	  goto common;
      }
    }
    if (type == 0x10a9) {		/* Get white balance (D60) */
      if (!strcmp(handle->model,"Canon EOS 10D"))
	wbi = remap_10d[wbi];
      fseek (handle->ifp, aoff+2 + wbi*8, SEEK_SET);
      handle->camera_red  = fget2(handle, handle->ifp);
      handle->camera_red /= fget2(handle, handle->ifp);
      handle->camera_blue = fget2(handle, handle->ifp);
      handle->camera_blue = fget2(handle, handle->ifp) / handle->camera_blue;
    }
    if (type == 0x1030 && wbi > 14) {	/* Get white sample */
      fseek (handle->ifp, aoff, SEEK_SET);
      ciff_block_1030(handle);
    }
    if (type == 0x1031) {		/* Get the raw width and height */
      fseek (handle->ifp, aoff+2, SEEK_SET);
      handle->raw_width  = fget2(handle, handle->ifp);
      handle->raw_height = fget2(handle, handle->ifp);
    }
    if (type == 0x180e) {		/* Get the timestamp */
      fseek (handle->ifp, aoff, SEEK_SET);
      handle->timestamp = fget4(handle, handle->ifp);
    }
    if (type == 0x1835) {		/* Get the decoder table */
      fseek (handle->ifp, aoff, SEEK_SET);
      crw_init_tables (handle, fget4(handle, handle->ifp));
    }
    if (type >> 8 == 0x28 || type >> 8 == 0x30)	/* Get sub-tables */
      parse_ciff(handle, aoff, len);
    fseek (handle->ifp, save, SEEK_SET);
  }
  if (wbi == 0 && !strcmp(handle->model,"Canon EOS D30"))
    handle->camera_red = -1;			/* Use my auto WB for this photo */
}

static void parse_rollei(dcrawhandle *handle)
{
  char line[128], *val;
  int tx=0, ty=0;

  fseek (handle->ifp, 0, SEEK_SET);
  do {
    fgets (line, 128, handle->ifp);
    if ((val = strchr(line,'=')))
      *val++ = 0;
    else
      val = line + strlen(line);
    if (!strcmp(line,"HDR"))
      handle->data_offset = atoi(val);
    if (!strcmp(line,"X  "))
      handle->raw_width = atoi(val);
    if (!strcmp(line,"Y  "))
      handle->raw_height = atoi(val);
    if (!strcmp(line,"TX "))
      tx = atoi(val);
    if (!strcmp(line,"TY "))
      ty = atoi(val);
  } while (strncmp(line,"EOHD",4));
  handle->data_offset += tx * ty * 2;
  strcpy (handle->make, "Rollei");
  strcpy (handle->model, "d530flex");
}

static void parse_foveon(dcrawhandle *handle)
{
  char *buf, *bp, *np;
  int off1, off2, len, i;

  handle->order = 0x4949;			/* Little-endian */
  fseek (handle->ifp, -4, SEEK_END);
  off2 = fget4(handle, handle->ifp);
  fseek (handle->ifp, off2, SEEK_SET);
  while (fget4(handle, handle->ifp) != 0x464d4143)	/* Search for "CAMF" */
    if (feof(handle->ifp)) return;
  off1 = fget4(handle, handle->ifp);
  fseek (handle->ifp, off1+8, SEEK_SET);
  off1 += (fget4(handle, handle->ifp)+3) * 8;
  len = (off2 - off1)/2;
  fseek (handle->ifp, off1, SEEK_SET);
  buf = malloc (len);
  merror (handle, buf, "parse_foveon()");
  for (i=0; i < len; i++)		/* Convert Unicode to ASCII */
    buf[i] = fget2(handle, handle->ifp);
  for (bp=buf; bp < buf+len; bp=np) {
    np = bp + strlen(bp) + 1;
    if (!strcmp(bp,"CAMMANUF"))
      strcpy (handle->make, np);
    if (!strcmp(bp,"CAMMODEL"))
      strcpy (handle->model, np);
  }
  fseek (handle->ifp, 248, SEEK_SET);
  handle->raw_width  = fget4(handle, handle->ifp);
  handle->raw_height = fget4(handle, handle->ifp);
  free(buf);
}

static void foveon_coeff(dcrawhandle *handle)
{
  static const float foveon[3][3] = {
    {  2.0343955, -0.727533, -0.3067457 },
    { -0.2287194,  1.231793, -0.0028293 },
    { -0.0086152, -0.153336,  1.1617814 }
  };
  int i, j;

  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      handle->coeff[i][j] = foveon[i][j] * handle->pre_mul[i];
  handle->use_coeff = 1;
}

#ifdef CUSTOM
/*
   The grass is always greener in my PowerShot G2 when this
   function is called.  Use at your own risk!
 */
static void canon_rgb_coeff (dcrawhandle *handle, float juice)
{
  static const float my_coeff[3][3] =
  { {  1.116187, -0.107427, -0.008760 },
    { -1.551374,  4.157144, -1.605770 },
    {  0.090939, -0.399727,  1.308788 } };
  int i, j;

  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      handle->coeff[i][j] = my_coeff[i][j] * juice + (i==j) * (1-juice);
  handle->use_coeff = 1;
}
#endif

static void nikon_e950_coeff(dcrawhandle *handle)
{
  int r, g;
  static const float my_coeff[3][4] =
  { { -1.936280,  1.800443, -1.448486,  2.584324 },
    {  1.405365, -0.524955, -0.289090,  0.408680 },
    { -1.204965,  1.082304,  2.941367, -1.818705 } };

  for (r=0; r < 3; r++)
    for (g=0; g < 4; g++)
      handle->coeff[r][g] = my_coeff[r][g];
  handle->use_coeff = 1;
}

/*
   Given a matrix that converts RGB to GMCY, create a matrix to do
   the opposite.  Only square matrices can be inverted, so I create
   four 3x3 matrices by omitting a different GMCY color in each one.
   The final coeff[][] matrix is the sum of these four.
 */
static void gmcy_coeff(dcrawhandle *handle)
{
  static const float gmcy[4][3] = {
/*    red  green  blue			   */
    { 0.11, 0.86, 0.08 },	/* green   */
    { 0.50, 0.29, 0.51 },	/* magenta */
    { 0.11, 0.92, 0.75 },	/* cyan    */
    { 0.81, 0.98, 0.08 }	/* yellow  */
  };
  double invert[3][6], num;
  int ignore, i, j, k, r, g;

  memset (handle->coeff, 0, sizeof handle->coeff);
  for (ignore=0; ignore < 4; ignore++) {
    for (j=0; j < 3; j++) {
      g = (j < ignore) ? j : j+1;
      for (r=0; r < 3; r++) {
	invert[j][r] = gmcy[g][r];	/* 3x3 matrix to invert */
	invert[j][r+3] = (r == j);	/* Identity matrix	*/
      }
    }
    for (j=0; j < 3; j++) {
      num = invert[j][j];		/* Normalize this row	*/
      for (i=0; i < 6; i++)
	invert[j][i] /= num;
      for (k=0; k < 3; k++) {		/* Subtract it from the other rows */
	if (k==j) continue;
	num = invert[k][j];
	for (i=0; i < 6; i++)
	  invert[k][i] -= invert[j][i] * num;
      }
    }
    for (j=0; j < 3; j++) {		/* Add the result to coeff[][] */
      g = (j < ignore) ? j : j+1;
      for (r=0; r < 3; r++)
	handle->coeff[r][g] += invert[r][j+3];
    }
  }
  for (r=0; r < 3; r++) {		/* Normalize such that:		*/
    for (num=g=0; g < 4; g++)		/* (1,1,1,1) x coeff = (1,1,1) */
      num += handle->coeff[r][g];
    for (g=0; g < 4; g++)
      handle->coeff[r][g] /= num;
  }
  handle->use_coeff = 1;
}

/*
   Identify which camera created this file, and set global variables
   accordingly.  Return nonzero if the file cannot be decoded.
 */
int identify(dcrawhandle *handle)
{
  char head[32], *c;
  unsigned hlen, fsize, i;
  static const struct {
    int fsize;
    char make[12], model[16];
  } table[] = {
    {    62464, "Kodak",    "DC20" },
    {   124928, "Kodak",    "DC20" },
    {  2465792, "NIKON",    "E950" },
    {  2940928, "NIKON",    "E2100" },
    {  4771840, "NIKON",    "E990" },
    {  5865472, "NIKON",    "E4500" },
    {  5869568, "NIKON",    "E4300" },
    {   787456, "Creative", "PC-CAM 600" },
    {  1976352, "Casio",    "QV-2000UX" },
    {  3217760, "Casio",    "QV-3*00EX" },
    {  6218368, "Casio",    "QV-5700" },
    {  7684000, "Casio",    "QV-4000" },
    {  9313536, "Casio",    "EX-P600" },
    {  4841984, "Pentax",   "Optio S" },
    {  6114240, "Pentax",   "Optio S4" },
    { 12582980, "Sinar",    "" } };

/*  What format is this file?  Set make[] if we recognize it. */

  handle->raw_height = handle->raw_width = 0;
  handle->make[0] = handle->model[0] = handle->model2[0] = 0;
  memset (handle->white, 0, sizeof handle->white);
  handle->camera_red = handle->camera_blue = handle->timestamp = 0;
  handle->data_offset = handle->curve_offset = handle->tiff_data_compression = 0;
  handle->zero_after_ff = 0;

  handle->order = fget2(handle, handle->ifp);
  hlen = fget4(handle, handle->ifp);
  fseek (handle->ifp, 0, SEEK_SET);
  fread (head, 1, 32, handle->ifp);
  fseek (handle->ifp, 0, SEEK_END);
  fsize = ftell(handle->ifp);
  if ((c = memmem (head, 32, "MMMMRawT", 8))) {
    strcpy (handle->make, "Phase One");
    handle->data_offset = c - head;
    fseek (handle->ifp, handle->data_offset + 8, SEEK_SET);
    fseek (handle->ifp, fget4(handle, handle->ifp) + 136, SEEK_CUR);
    handle->raw_width = fget4(handle, handle->ifp);
    fseek (handle->ifp, 12, SEEK_CUR);
    handle->raw_height = fget4(handle, handle->ifp);
  } else if (handle->order == 0x4949 || handle->order == 0x4d4d) {
    if (!memcmp (head+6, "HEAPCCDR", 8)) {
      handle->data_offset = hlen;
      parse_ciff (handle, hlen, fsize - hlen);
    } else {
      parse_tiff(handle, 0);
      if (!strncmp(handle->make,"NIKON",5) && handle->raw_width == 0)
	handle->make[0] = 0;
    }
  } else if (!memcmp (head, "\0MRM", 4)) {
    parse_tiff(handle, 48);
    fseek (handle->ifp, 4, SEEK_SET);
    handle->data_offset = fget4(handle, handle->ifp) + 8;
    fseek (handle->ifp, 24, SEEK_SET);
    handle->raw_height = fget2(handle, handle->ifp);
    handle->raw_width  = fget2(handle, handle->ifp);
    fseek (handle->ifp, 12, SEEK_SET);			/* PRD */
    fseek (handle->ifp, fget4(handle, handle->ifp) +  4, SEEK_CUR);	/* TTW */
    fseek (handle->ifp, fget4(handle, handle->ifp) + 12, SEEK_CUR);	/* WBG */
    handle->camera_red  = fget2(handle, handle->ifp);
    handle->camera_red /= fget2(handle, handle->ifp);
    handle->camera_blue = fget2(handle, handle->ifp);
    handle->camera_blue = fget2(handle, handle->ifp) / handle->camera_blue;
  } else if (!memcmp (head, "\xff\xd8\xff\xe1", 4) &&
	     !memcmp (head+6, "Exif", 4)) {
    fseek (handle->ifp, 4, SEEK_SET);
    fseek (handle->ifp, 4 + fget2(handle, handle->ifp), SEEK_SET);
    if (fgetc(handle->ifp) != 0xff)
      parse_tiff(handle, 12);
  } else if (!memcmp (head, "BM", 2)) {
    handle->data_offset = 0x1000;
    handle->order = 0x4949;
    fseek (handle->ifp, 38, SEEK_SET);
    if (fget4(handle, handle->ifp) == 2834 && fget4(handle, handle->ifp) == 2834) {
      strcpy (handle->model, "BMQ");
      goto nucore;
    }
  } else if (!memcmp (head, "BR", 2)) {
    strcpy (handle->model, "RAW");
nucore:
    strcpy (handle->make, "Nucore");
    handle->order = 0x4949;
    fseek (handle->ifp, 10, SEEK_SET);
    handle->data_offset += fget4(handle, handle->ifp);
    fget4(handle, handle->ifp);
    handle->raw_width  = fget4(handle, handle->ifp);
    handle->raw_height = fget4(handle, handle->ifp);
    if (handle->model[0] == 'B' && handle->raw_width == 2597) {
      handle->raw_width++;
      handle->data_offset -= 0x1000;
    }
  } else if (!memcmp (head+25, "ARECOYK", 7)) {
    strcpy (handle->make, "CONTAX");
    strcpy (handle->model, "N DIGITAL");
  } else if (!memcmp (head, "FUJIFILM", 8)) {
    fseek (handle->ifp, 84, SEEK_SET);
    parse_tiff (handle, fget4(handle, handle->ifp)+12);
    handle->order = 0x4d4d;
    fseek (handle->ifp, 100, SEEK_SET);
    handle->data_offset = fget4(handle, handle->ifp);
  } else if (!memcmp (head, "DSC-Image", 9))
    parse_rollei(handle);
  else if (!memcmp (head, "FOVb", 4))
    parse_foveon(handle);
  else
    for (i=0; i < sizeof table / sizeof *table; i++)
      if (fsize == table[i].fsize) {
	strcpy (handle->make,  table[i].make );
	strcpy (handle->model, table[i].model);
      }

  /* Remove excess wordage */
  if (!strncmp(handle->make,"NIKON",5) || !strncmp(handle->make,"Canon",5))
    handle->make[5] = 0;
  if (!strncmp(handle->make,"PENTAX",6))
    handle->make[6] = 0;
  if (strstr(handle->make,"Minolta"))
    strcpy (handle->make, "Minolta");
  if (!strncmp(handle->make,"KODAK",5))
    handle->make[16] = handle->model[16] = 0;
  if (!strcmp(handle->make,"Eastman Kodak Company"))
    strcpy (handle->make, "Kodak");
  i = strlen(handle->make);
  if (!strncmp(handle->model,handle->make,i++))
    memmove (handle->model, handle->model+i, 64-i);

  /* Remove trailing spaces */
  c = handle->make + strlen(handle->make);
  while (*--c == ' ') *c = 0;
  c = handle->model + strlen(handle->model);
  while (*--c == ' ') *c = 0;

  if (handle->make[0] == 0) {
	  if( handle->verbose ) fprintf (stderr, "%s: unsupported file format.\n", handle->ifname);
    return 1;
  }

/*  File format is OK.  Do we support this camera? */
/*  Start with some useful defaults:		   */

  handle->load_raw = NULL;
  handle->height = handle->raw_height;
  handle->width  = handle->raw_width;
  handle->top_margin = handle->left_margin = 0;
  handle->colors = 3;
  handle->filters = 0x94949494;
  handle->black = handle->is_cmy = handle->is_foveon = handle->use_coeff = 0;
  handle->pre_mul[0] = handle->pre_mul[1] = handle->pre_mul[2] = handle->pre_mul[3] = handle->ymag = 1;
  handle->rgb_max = 0x4000;

/*  We'll try to decode anything from Canon or Nikon. */

  if ((handle->is_canon = !strcmp(handle->make,"Canon"))) {
    if (memcmp (head+6, "HEAPCCDR", 8)) {
      handle->filters = 0x61616161;
      handle->load_raw = lossless_jpeg_load_raw;
    } else
      handle->load_raw = canon_compressed_load_raw;
  }
  if (!strcmp(handle->make,"NIKON"))
    handle->load_raw = nikon_is_compressed(handle) ?
	nikon_compressed_load_raw : nikon_load_raw;

  if (!strcmp(handle->model,"PowerShot 600")) {
    handle->height = 613;
    handle->width  = 854;
    handle->colors = 4;
    handle->filters = 0xe1e4e1e4;
    handle->load_raw = canon_600_load_raw;
    handle->pre_mul[0] = 1.137;
    handle->pre_mul[1] = 1.257;
  } else if (!strcmp(handle->model,"PowerShot A5")) {
    handle->height = 776;
    handle->width  = 960;
    handle->raw_width = 992;
    handle->colors = 4;
    handle->filters = 0x1e4e1e4e;
    handle->load_raw = canon_a5_load_raw;
    handle->pre_mul[0] = 1.5842;
    handle->pre_mul[1] = 1.2966;
    handle->pre_mul[2] = 1.0419;
  } else if (!strcmp(handle->model,"PowerShot A50")) {
    handle->height =  968;
    handle->width  = 1290;
    handle->raw_width = 1320;
    handle->colors = 4;
    handle->filters = 0x1b4e4b1e;
    handle->load_raw = canon_a5_load_raw;
    handle->pre_mul[0] = 1.750;
    handle->pre_mul[1] = 1.381;
    handle->pre_mul[3] = 1.182;
  } else if (!strcmp(handle->model,"PowerShot Pro70")) {
    handle->height = 1024;
    handle->width  = 1552;
    handle->colors = 4;
    handle->filters = 0x1e4b4e1b;
    handle->load_raw = canon_a5_load_raw;
    handle->pre_mul[0] = 1.389;
    handle->pre_mul[1] = 1.343;
    handle->pre_mul[3] = 1.034;
  } else if (!strcmp(handle->model,"PowerShot Pro90 IS")) {
    handle->width  = 1896;
    handle->colors = 4;
    handle->filters = 0xb4b4b4b4;
    handle->pre_mul[0] = 1.496;
    handle->pre_mul[1] = 1.509;
    handle->pre_mul[3] = 1.009;
  } else if (handle->is_canon && handle->raw_width == 2144) {
    handle->height = 1550;
    handle->width  = 2088;
    handle->top_margin  = 8;
    handle->left_margin = 4;
    if (!strcmp(handle->model,"PowerShot G1")) {
      handle->colors = 4;
      handle->filters = 0xb4b4b4b4;
      handle->pre_mul[0] = 1.446;
      handle->pre_mul[1] = 1.405;
      handle->pre_mul[2] = 1.016;
    } else {
      handle->pre_mul[0] = 1.785;
      handle->pre_mul[2] = 1.266;
    }
  } else if (handle->is_canon && handle->raw_width == 2224) {
    handle->height = 1448;
    handle->width  = 2176;
    handle->top_margin  = 6;
    handle->left_margin = 48;
    handle->pre_mul[0] = 1.592;
    handle->pre_mul[2] = 1.261;
  } else if (handle->is_canon && handle->raw_width == 2376) {
    handle->height = 1720;
    handle->width  = 2312;
    handle->top_margin  = 6;
    handle->left_margin = 12;
#ifdef CUSTOM
    if (handle->write_fun == write_ppm)		/* Pro users may not want my matrix */
      canon_rgb_coeff (0.1);
#endif
    if (!strcmp(handle->model,"PowerShot G2") ||
	!strcmp(handle->model,"PowerShot S40")) {
      handle->pre_mul[0] = 1.965;
      handle->pre_mul[2] = 1.208;
    } else {				/* G3 and S45 */
      handle->pre_mul[0] = 1.855;
      handle->pre_mul[2] = 1.339;
    }
  } else if (handle->is_canon && handle->raw_width == 2672) {
    handle->height = 1960;
    handle->width  = 2616;
    handle->top_margin  = 6;
    handle->left_margin = 12;
    handle->pre_mul[0] = 1.895;
    handle->pre_mul[2] = 1.403;
  } else if (handle->is_canon && handle->raw_width == 3152) {
    handle->height = 2056;
    handle->width  = 3088;
    handle->top_margin  = 12;
    handle->left_margin = 64;
    handle->pre_mul[0] = 2.242;
    handle->pre_mul[2] = 1.245;
    if (!strcmp(handle->model,"EOS Kiss Digital")) {
      handle->pre_mul[0] = 1.882;
      handle->pre_mul[2] = 1.094;
    }
    handle->rgb_max = 16000;
  } else if (handle->is_canon && handle->raw_width == 3344) {
    handle->height = 2472;
    handle->width  = 3288;
    handle->top_margin  = 6;
    handle->left_margin = 4;
    handle->pre_mul[0] = 1.621;
    handle->pre_mul[2] = 1.528;
  } else if (!strcmp(handle->model,"EOS-1D")) {
    handle->height = 1662;
    handle->width  = 2496;
    handle->data_offset = 288912;
    handle->pre_mul[0] = 1.976;
    handle->pre_mul[2] = 1.282;
  } else if (!strcmp(handle->model,"EOS-1DS")) {
    handle->height = 2718;
    handle->width  = 4082;
    handle->data_offset = 289168;
    handle->pre_mul[0] = 1.66;
    handle->pre_mul[2] = 1.13;
    handle->rgb_max = 14464;
  } else if (!strcmp(handle->model,"EOS-1D Mark II")) {
    handle->raw_height = 2360;
    handle->raw_width  = 3596;
    handle->top_margin  = 12;
    handle->left_margin = 74;
    handle->height = handle->raw_height - handle->top_margin;
    handle->width  = handle->raw_width - handle->left_margin;
    handle->filters = 0x94949494;
    handle->pre_mul[0] = 2.248;
    handle->pre_mul[2] = 1.174;
  } else if (!strcmp(handle->model,"EOS D2000C")) {
    handle->black = 800;
    handle->pre_mul[2] = 1.25;
  } else if (!strcmp(handle->model,"D1")) {
    handle->filters = 0x16161616;
    handle->pre_mul[0] = 0.838;
    handle->pre_mul[2] = 1.095;
  } else if (!strcmp(handle->model,"D1H")) {
    handle->filters = 0x16161616;
    handle->pre_mul[0] = 2.301;
    handle->pre_mul[2] = 1.129;
  } else if (!strcmp(handle->model,"D1X")) {
    handle->width  = 4024;
    handle->filters = 0x16161616;
    handle->ymag = 2;
    handle->pre_mul[0] = 1.910;
    handle->pre_mul[2] = 1.220;
  } else if (!strcmp(handle->model,"D100")) {
    if (handle->tiff_data_compression == 34713 && handle->load_raw == nikon_load_raw)
      handle->raw_width = (handle->width += 3) + 3;
    handle->filters = 0x61616161;
    handle->pre_mul[0] = 2.374;
    handle->pre_mul[2] = 1.677;
    handle->rgb_max = 15632;
  } else if (!strcmp(handle->model,"D2H")) {
    handle->width  = 2482;
    handle->left_margin = 6;
    handle->filters = 0x49494949;
    handle->pre_mul[0] = 2.8;
    handle->pre_mul[2] = 1.2;
  } else if (!strcmp(handle->model,"D70")) {
    handle->filters = 0x16161616;
    handle->pre_mul[0] = 2.8;
    handle->pre_mul[2] = 1.2;
  } else if (!strcmp(handle->model,"E950")) {
    handle->height = 1203;
    handle->width  = 1616;
    handle->filters = 0x4b4b4b4b;
    handle->colors = 4;
    handle->load_raw = nikon_e950_load_raw;
    nikon_e950_coeff(handle);
    handle->pre_mul[0] = 1.18193;
    handle->pre_mul[2] = 1.16452;
    handle->pre_mul[3] = 1.17250;
  } else if (!strcmp(handle->model,"E990")) {
    handle->height = 1540;
    handle->width  = 2064;
    handle->colors = 4;
    if (nikon_e990(handle)) {
      handle->filters = 0xb4b4b4b4;
      nikon_e950_coeff(handle);
      handle->pre_mul[0] = 1.196;
      handle->pre_mul[1] = 1.246;
      handle->pre_mul[2] = 1.018;
    } else {
      strcpy (handle->model, "E995");
      handle->filters = 0xe1e1e1e1;
      handle->pre_mul[0] = 1.253;
      handle->pre_mul[1] = 1.178;
      handle->pre_mul[3] = 1.035;
    }
  } else if (!strcmp(handle->model,"E2100")) {
    handle->width = 1616;
    if (nikon_e2100(handle)) {
      handle->height = 1206;
      handle->load_raw = nikon_e2100_load_raw;
      handle->pre_mul[0] = 1.945;
      handle->pre_mul[2] = 1.040;
    } else {
      strcpy (handle->model, "E2500");
      handle->height = 1204;
      handle->filters = 0x4b4b4b4b;
      goto coolpix;
    }
  } else if (!strcmp(handle->model,"E4300")) {
    handle->height = 1710;
    handle->width  = 2288;
    handle->filters = 0x16161616;
  } else if (!strcmp(handle->model,"E4500")) {
    handle->height = 1708;
    handle->width  = 2288;
    handle->filters = 0xb4b4b4b4;
    goto coolpix;
  } else if (!strcmp(handle->model,"E5000") || !strcmp(handle->model,"E5700")) {
    handle->filters = 0xb4b4b4b4;
coolpix:
    handle->colors = 4;
    handle->pre_mul[0] = 1.300;
    handle->pre_mul[1] = 1.300;
    handle->pre_mul[3] = 1.148;
  } else if (!strcmp(handle->model,"E5400")) {
    handle->filters = 0x16161616;
    handle->pre_mul[0] = 1.700;
    handle->pre_mul[2] = 1.344;
  } else if (!strcmp(handle->model,"E8700")) {
    handle->filters = 0x16161616;
    handle->pre_mul[0] = 2.131;
    handle->pre_mul[2] = 1.300;
  } else if (!strcmp(handle->model,"FinePixS2Pro")) {
    handle->height = 3584;
    handle->width  = 3583;
    handle->filters = 0x61616161;
    handle->load_raw = fuji_s2_load_raw;
    handle->black = 512;
    handle->pre_mul[0] = 1.424;
    handle->pre_mul[2] = 1.718;
  } else if (!strcmp(handle->model,"FinePix S5000")) {
    handle->height = 2499;
    handle->width  = 2500;
    handle->filters = 0x49494949;
    handle->load_raw = fuji_s5000_load_raw;
    handle->pre_mul[0] = 1.639;
    handle->pre_mul[2] = 1.438;
    handle->rgb_max = 0xf7ff;
  } else if (!strcmp(handle->model,"FinePix S7000")) {
    handle->height = 3587;
    handle->width  = 3588;
    handle->filters = 0x49494949;
    handle->load_raw = fuji_s7000_load_raw;
    handle->pre_mul[0] = 1.81;
    handle->pre_mul[2] = 1.38;
    handle->rgb_max = 0xf7ff;
  } else if (!strcmp(handle->model,"FinePix F700")) {
    handle->height = 2523;
    handle->width  = 2524;
    handle->filters = 0x49494949;
    handle->load_raw = fuji_f700_load_raw;
    handle->pre_mul[0] = 1.639;
    handle->pre_mul[2] = 1.438;
    handle->rgb_max = 14000;
  } else if (!strcmp(handle->make,"Minolta")) {
    handle->load_raw = be_low_12_load_raw;
    if (!strncmp(handle->model,"DiMAGE A",8))
      handle->load_raw = packed_12_load_raw;
    else if (!strncmp(handle->model,"DiMAGE G",8)) {
      handle->load_raw = be_low_10_load_raw;
      handle->height = 1956;
      handle->width  = 2607;
      handle->raw_width = 2624;
      handle->filters = 0x61616161;
      handle->data_offset = 4016;
      handle->rgb_max = 15856;
      handle->order = 0x4d4d;
      fseek (handle->ifp, 1936, SEEK_SET);
      handle->camera_red   = fget2(handle, handle->ifp);
      handle->camera_blue  = fget2(handle, handle->ifp);
      handle->camera_red  /= fget2(handle, handle->ifp);
      handle->camera_blue /= fget2(handle, handle->ifp);
    }
    handle->pre_mul[0] = 2.00;
    handle->pre_mul[2] = 1.25;
  } else if (!strcmp(handle->model,"*ist D")) {
    handle->height = 2024;
    handle->width  = 3040;
    handle->data_offset = 0x10000;
    handle->load_raw = be_low_12_load_raw;
    handle->pre_mul[0] = 1.76;
    handle->pre_mul[1] = 1.07;
  } else if (!strcmp(handle->model,"Optio S")) {
    handle->height = 1544;
    handle->width  = 2068;
    handle->raw_width = 3136;
    handle->load_raw = packed_12_load_raw;
    handle->pre_mul[0] = 1.506;
    handle->pre_mul[2] = 1.152;
  } else if (!strcmp(handle->model,"Optio S4")) {
    handle->height = 1737;
    handle->width  = 2324;
    handle->raw_width = 3520;
    handle->load_raw = packed_12_load_raw;
    handle->pre_mul[0] = 1.308;
    handle->pre_mul[2] = 1.275;
  } else if (!strcmp(handle->make,"Phase One")) {
    switch (handle->raw_height) {
      case 2060:
	strcpy (handle->model, "LightPhase");
	handle->height = 2048;
	handle->width  = 3080;
	handle->top_margin  = 5;
	handle->left_margin = 22;
	handle->pre_mul[0] = 1.331;
	handle->pre_mul[2] = 1.154;
	break;
      case 2682:
	strcpy (handle->model, "H10");
	handle->height = 2672;
	handle->width  = 4012;
	handle->top_margin  = 5;
	handle->left_margin = 26;
	break;
      case 4128:
	strcpy (handle->model, "H20");
	handle->height = 4098;
	handle->width  = 4098;
	handle->top_margin  = 20;
	handle->left_margin = 26;
	handle->pre_mul[0] = 1.963;
	handle->pre_mul[2] = 1.430;
	break;
      case 5488:
	strcpy (handle->model, "H25");
	handle->height = 5458;
	handle->width  = 4098;
	handle->top_margin  = 20;
	handle->left_margin = 26;
	handle->pre_mul[0] = 2.80;
	handle->pre_mul[2] = 1.20;
    }
    handle->filters = handle->top_margin & 1 ? 0x94949494 : 0x49494949;
    handle->load_raw = phase_one_load_raw;
    handle->rgb_max = 0xffff;
  } else if (!strcmp(handle->model,"Ixpress")) {
    handle->height = 4084;
    handle->width  = 4080;
    handle->filters = 0x49494949;
    handle->load_raw = ixpress_load_raw;
    handle->pre_mul[0] = 1.963;
    handle->pre_mul[2] = 1.430;
    handle->rgb_max = 0xffff;
  } else if (!strcmp(handle->make,"Sinar") && !memcmp(head,"8BPS",4)) {
    fseek (handle->ifp, 14, SEEK_SET);
    handle->height = fget4(handle, handle->ifp);
    handle->width  = fget4(handle, handle->ifp);
    handle->filters = 0x61616161;
    handle->data_offset = 68;
    handle->load_raw = be_16_load_raw;
    handle->rgb_max = 0xffff;
  } else if (!strcmp(handle->make,"Leaf")) {
    if (handle->height > handle->width)
      handle->filters = 0x16161616;
    handle->load_raw = be_16_load_raw;
    handle->pre_mul[0] = 1.1629;
    handle->pre_mul[2] = 1.3556;
    handle->rgb_max = 0xffff;
  } else if (!strcmp(handle->model,"DIGILUX 2") || !strcmp(handle->model,"DMC-LC1")) {
    handle->height = 1928;
    handle->width  = 2568;
    handle->data_offset = 1024;
    handle->load_raw = le_high_12_load_raw;
    handle->pre_mul[0] = 1.883;
    handle->pre_mul[2] = 1.367;
  } else if (!strcmp(handle->model,"E-1")) {
    handle->filters = 0x61616161;
    handle->load_raw = le_high_12_load_raw;
    handle->pre_mul[0] = 1.57;
    handle->pre_mul[2] = 1.48;
  } else if (!strcmp(handle->model,"E-10")) {
    handle->load_raw = be_high_12_load_raw;
    handle->pre_mul[0] = 1.43;
    handle->pre_mul[2] = 1.77;
  } else if (!strncmp(handle->model,"E-20",4)) {
    handle->load_raw = be_high_12_load_raw;
    handle->black = 640;
    handle->pre_mul[0] = 1.43;
    handle->pre_mul[2] = 1.77;
  } else if (!strcmp(handle->model,"C5050Z")) {
    handle->filters = 0x16161616;
    handle->load_raw = olympus_cseries_load_raw;
    handle->pre_mul[0] = 1.533;
    handle->pre_mul[2] = 1.880;
  } else if (!strcmp(handle->model,"C5060WZ")) {
    handle->load_raw = olympus_cseries_load_raw;
    handle->pre_mul[0] = 2.285;
    handle->pre_mul[2] = 1.023;
  } else if (!strcmp(handle->model,"C8080WZ")) {
    handle->filters = 0x16161616;
    handle->load_raw = olympus_cseries_load_raw;
    handle->pre_mul[0] = 2.335;
    handle->pre_mul[2] = 1.323;
  } else if (!strcmp(handle->model,"N DIGITAL")) {
    handle->height = 2047;
    handle->width  = 3072;
    handle->filters = 0x61616161;
    handle->data_offset = 0x1a00;
    handle->load_raw = packed_12_load_raw;
    handle->pre_mul[0] = 1.366;
    handle->pre_mul[2] = 1.251;
  } else if (!strcmp(handle->model,"DSC-F828")) {
    handle->height = 2460;
    handle->width = 3288;
    handle->raw_width = 3360;
    handle->left_margin = 5;
    handle->load_raw = sony_load_raw;
    sony_rgbe_coeff(handle);
    handle->filters = 0xb4b4b4b4;
    handle->colors = 4;
    handle->pre_mul[0] = 1.512;
    handle->pre_mul[1] = 1.020;
    handle->pre_mul[2] = 1.405;
  } else if (!strcasecmp(handle->make,"KODAK")) {
    handle->filters = 0x61616161;
    if (!strcmp(handle->model,"NC2000F")) {
      handle->width -= 4;
      handle->left_margin = 1;
      handle->curve_length = 176;
      handle->pre_mul[0] = 1.509;
      handle->pre_mul[2] = 2.686;
    } else if (!strcmp(handle->model,"EOSDCS3B")) {
      handle->width -= 4;
      handle->left_margin = 2;
      handle->pre_mul[0] = 1.629;
      handle->pre_mul[2] = 2.767;
    } else if (!strcmp(handle->model,"EOSDCS1")) {
      handle->width -= 4;
      handle->left_margin = 2;
      handle->pre_mul[0] = 1.386;
      handle->pre_mul[2] = 2.405;
    } else if (!strcmp(handle->model,"DCS315C")) {
      handle->black = 32;
      handle->pre_mul[1] = 1.068;
      handle->pre_mul[2] = 1.036;
    } else if (!strcmp(handle->model,"DCS330C")) {
      handle->black = 32;
      handle->pre_mul[1] = 1.012;
      handle->pre_mul[2] = 1.804;
    } else if (!strcmp(handle->model,"DCS420")) {
      handle->width -= 4;
      handle->left_margin = 2;
      handle->pre_mul[0] = 1.327;
      handle->pre_mul[2] = 2.074;
    } else if (!strcmp(handle->model,"DCS460")) {
      handle->width -= 4;
      handle->left_margin = 2;
      handle->pre_mul[0] = 1.724;
      handle->pre_mul[2] = 2.411;
    } else if (!strcmp(handle->model,"DCS460A")) {
      handle->width -= 4;
      handle->left_margin = 2;
      handle->colors = 1;
      handle->filters = 0;
    } else if (!strcmp(handle->model,"DCS520C")) {
      handle->black = 720;
      handle->pre_mul[0] = 1.006;
      handle->pre_mul[2] = 1.858;
    } else if (!strcmp(handle->model,"DCS560C")) {
      handle->black = 750;
      handle->pre_mul[1] = 1.053;
      handle->pre_mul[2] = 1.703;
    } else if (!strcmp(handle->model,"DCS620C")) {
      handle->black = 720;
      handle->pre_mul[1] = 1.002;
      handle->pre_mul[2] = 1.818;
    } else if (!strcmp(handle->model,"DCS620X")) {
      handle->black = 740;
      handle->pre_mul[0] = 1.486;
      handle->pre_mul[2] = 1.280;
      handle->is_cmy = 1;
    } else if (!strcmp(handle->model,"DCS660C")) {
      handle->black = 855;
      handle->pre_mul[0] = 1.156;
      handle->pre_mul[2] = 1.626;
    } else if (!strcmp(handle->model,"DCS660M")) {
      handle->black = 855;
      handle->colors = 1;
      handle->filters = 0;
    } else if (!strcmp(handle->model,"DCS720X")) {
      handle->pre_mul[0] = 1.35;
      handle->pre_mul[2] = 1.18;
      handle->is_cmy = 1;
    } else if (!strcmp(handle->model,"DCS760C")) {
      handle->pre_mul[0] = 1.06;
      handle->pre_mul[2] = 1.72;
    } else if (!strcmp(handle->model,"DCS760M")) {
      handle->colors = 1;
      handle->filters = 0;
    } else if (!strcmp(handle->model,"ProBack")) {
      handle->pre_mul[0] = 1.06;
      handle->pre_mul[2] = 1.385;
    } else if (!strncmp(handle->model2,"PB645C",6)) {
      handle->pre_mul[0] = 1.0497;
      handle->pre_mul[2] = 1.3306;
    } else if (!strncmp(handle->model2,"PB645H",6)) {
      handle->pre_mul[0] = 1.2010;
      handle->pre_mul[2] = 1.5061;
    } else if (!strncmp(handle->model2,"PB645M",6)) {
      handle->pre_mul[0] = 1.01755;
      handle->pre_mul[2] = 1.5424;
    } else if (!strcasecmp(handle->model,"DCS Pro 14n")) {
      handle->pre_mul[1] = 1.0323;
      handle->pre_mul[2] = 1.258;
    } else if (!strcasecmp(handle->model,"DCS Pro 14nx")) {
      handle->pre_mul[0] = 1.336;
      handle->pre_mul[2] = 1.3155;
    } else if (!strcasecmp(handle->model,"DCS Pro SLR/c")) {
      handle->pre_mul[0] = 1.425;
      handle->pre_mul[2] = 1.293;
    } else if (!strcasecmp(handle->model,"DCS Pro SLR/n")) {
      handle->pre_mul[0] = 1.324;
      handle->pre_mul[2] = 1.483;
    }
    switch (handle->tiff_data_compression) {
      case 0:				/* No compression */
      case 1:
	handle->load_raw = kodak_easy_load_raw;
	break;
      case 7:				/* Lossless JPEG */
	handle->load_raw = lossless_jpeg_load_raw;
      case 32867:
	break;
      case 65000:			/* Kodak DCR compression */
	if (handle->kodak_data_compression == 32803)
	  handle->load_raw = kodak_compressed_load_raw;
	else {
	  handle->load_raw = kodak_yuv_load_raw;
	  handle->height = (handle->height+1) & -2;
	  handle->width  = (handle->width +1) & -2;
	  handle->filters = 0;
	}
	break;
      default:
	if( handle->verbose ) fprintf (stderr, "%s: %s %s uses unsupported compression method %d.\n",
		handle->ifname, handle->make, handle->model, handle->tiff_data_compression);
	return 1;
    }
    if (!strcmp(handle->model,"DC20")) {
      handle->height = 242;
      if (fsize < 100000) {
	handle->width = 249;
	handle->raw_width = 256;
      } else {
	handle->width = 501;
	handle->raw_width = 512;
      }
      handle->data_offset = handle->raw_width + 1;
      handle->colors = 4;
      handle->filters = 0x8d8d8d8d;
      kodak_dc20_coeff (handle, 0.5);
      handle->pre_mul[1] = 1.179;
      handle->pre_mul[2] = 1.209;
      handle->pre_mul[3] = 1.036;
      handle->load_raw = kodak_easy_load_raw;
    } else if (strstr(handle->model,"DC25")) {
      strcpy (handle->model, "DC25");
      handle->height = 242;
      if (fsize < 100000) {
	handle->width = 249;
	handle->raw_width = 256;
	handle->data_offset = 15681;
      } else {
	handle->width = 501;
	handle->raw_width = 512;
	handle->data_offset = 15937;
      }
      handle->colors = 4;
      handle->filters = 0xb4b4b4b4;
      handle->load_raw = kodak_easy_load_raw;
    } else if (!strcmp(handle->model,"Digital Camera 40")) {
      strcpy (handle->model, "DC40");
      handle->height = 512;
      handle->width = 768;
      handle->data_offset = 1152;
      handle->load_raw = kodak_radc_load_raw;
    } else if (strstr(handle->model,"DC50")) {
      strcpy (handle->model, "DC50");
      handle->height = 512;
      handle->width = 768;
      handle->data_offset = 19712;
      handle->load_raw = kodak_radc_load_raw;
    } else if (strstr(handle->model,"DC120")) {
      strcpy (handle->model, "DC120");
      handle->height = 976;
      handle->width = 848;
      if (handle->tiff_data_compression == 7)
	handle->load_raw = kodak_jpeg_load_raw;
      else
	handle->load_raw = kodak_dc120_load_raw;
    }
  } else if (!strcmp(handle->make,"Rollei")) {
    switch (handle->raw_width) {
      case 1316:
	handle->height = 1030;
	handle->width  = 1300;
	handle->top_margin  = 1;
	handle->left_margin = 6;
	break;
      case 2568:
	handle->height = 1960;
	handle->width  = 2560;
	handle->top_margin  = 2;
	handle->left_margin = 8;
    }
    handle->filters = 0x16161616;
    handle->load_raw = rollei_load_raw;
    handle->pre_mul[0] = 1.8;
    handle->pre_mul[2] = 1.3;
  } else if (!strcmp(handle->make,"SIGMA")) {
    switch (handle->raw_height) {
      case  763:  handle->height =  756;  handle->top_margin =  2;  break;
      case 1531:  handle->height = 1514;  handle->top_margin =  7;  break;
    }
    switch (handle->raw_width) {
      case 1152:  handle->width = 1136;  handle->left_margin =  8;  break;
      case 2304:  handle->width = 2271;  handle->left_margin = 17;  break;
    }
    if (handle->height*2 < handle->width) handle->ymag = 2;
    handle->filters = 0;
    handle->load_raw = foveon_load_raw;
    handle->is_foveon = 1;
    handle->pre_mul[0] = 1.179;
    handle->pre_mul[2] = 0.713;
    if (!strcmp(handle->model,"SD10")) {
      handle->pre_mul[0] *= 2.07;
      handle->pre_mul[2] *= 2.30;
    }
    foveon_coeff(handle);
    handle->rgb_max = 5600;
  } else if (!strcmp(handle->model,"PC-CAM 600")) {
    handle->height = 768;
    handle->data_offset = handle->width = 1024;
    handle->filters = 0x49494949;
    handle->load_raw = eight_bit_load_raw;
    handle->pre_mul[0] = 1.14;
    handle->pre_mul[2] = 2.73;
  } else if (!strcmp(handle->model,"QV-2000UX")) {
    handle->height = 1208;
    handle->width  = 1632;
    handle->data_offset = handle->width * 2;
    handle->load_raw = eight_bit_load_raw;
  } else if (!strcmp(handle->model,"QV-3*00EX")) {
    handle->height = 1546;
    handle->width  = 2070;
    handle->raw_width = 2080;
    handle->load_raw = eight_bit_load_raw;
  } else if (!strcmp(handle->model,"QV-4000")) {
    handle->height = 1700;
    handle->width  = 2260;
    handle->load_raw = be_high_12_load_raw;
  } else if (!strcmp(handle->model,"QV-5700")) {
    handle->height = 1924;
    handle->width  = 2576;
    handle->load_raw = casio_qv5700_load_raw;
  } else if (!strcmp(handle->model,"EX-P600")) {
    handle->height = 2142;
    handle->width  = 2844;
    handle->raw_width = 4288;
    handle->load_raw = packed_12_load_raw;
    handle->pre_mul[0] = 2.356;
    handle->pre_mul[1] = 1.069;
  } else if (!strcmp(handle->make,"Nucore")) {
    handle->filters = 0x61616161;
    handle->load_raw = nucore_load_raw;
  }
  if (!handle->load_raw) {
    if( handle->verbose ) fprintf (stderr, "%s: %s %s is not yet supported.\n",
	handle->ifname, handle->make, handle->model);
    return 1;
  }
#ifdef NO_JPEG
  if (handle->load_raw == kodak_jpeg_load_raw) {
    if( handle->verbose ) fprintf (stderr, "%s: decoder was not linked with libjpeg.\n", handle->ifname);
    return 1;
  }
#endif
  if (!handle->raw_height) handle->raw_height = handle->height;
  if (!handle->raw_width ) handle->raw_width  = handle->width;
  if (handle->colors == 4 && !handle->use_coeff)
    gmcy_coeff(handle);
  if (handle->use_coeff)		 /* Apply user-selected color balance */
    for (i=0; i < handle->colors; i++) {
      handle->coeff[0][i] *= handle->red_scale;
      handle->coeff[2][i] *= handle->blue_scale;
    }
  if (handle->four_color_rgb && handle->filters && handle->colors == 3) {
    for (i=0; i < 32; i+=4) {
      if ((handle->filters >> i & 15) == 9)
	handle->filters |= 2 << i;
      if ((handle->filters >> i & 15) == 6)
	handle->filters |= 8 << i;
    }
    handle->colors++;
    if (handle->use_coeff)
      for (i=0; i < 3; i++)
	handle->coeff[i][3] = handle->coeff[i][1] /= 2;
  }
  fseek (handle->ifp, handle->data_offset, SEEK_SET);
  return 0;
}

/*
   Convert the entire image to RGB colorspace and build a histogram.
 */
void convert_to_rgb(dcrawhandle *handle)
{
  int row, col, r, g, c=0;
  ushort *img;
  float rgb[3], mag;

  if (handle->document_mode)
    handle->colors = 1;
  memset (handle->histogram, 0, sizeof handle->histogram);
  for (row = handle->trim; row < handle->height-handle->trim; row++)
    for (col = handle->trim; col < handle->width-handle->trim; col++) {
      img = handle->image[row*handle->width+col];
      if (handle->document_mode)
	c = FC(row,col);
      if (handle->colors == 4 && !handle->use_coeff)	/* Recombine the greens */
	img[1] = (img[1] + img[3]) >> 1;
      if (handle->colors == 1)			/* RGB from grayscale */
	for (r=0; r < 3; r++)
	  rgb[r] = img[c];
      else if (handle->use_coeff) {		/* RGB from GMCY or Foveon */
	for (r=0; r < 3; r++)
	  for (rgb[r]=g=0; g < handle->colors; g++)
	    rgb[r] += img[g] * handle->coeff[r][g];
      } else if (handle->is_cmy) {		/* RGB from CMY */
	rgb[0] = img[0] + img[1] - img[2];
	rgb[1] = img[1] + img[2] - img[0];
	rgb[2] = img[2] + img[0] - img[1];
      } else				/* RGB from RGB (easy) */
	goto norgb;
      for (r=0; r < 3; r++) {
	if (rgb[r] < 0)
	    rgb[r] = 0;
	if (rgb[r] > handle->rgb_max)
	    rgb[r] = handle->rgb_max;
	img[r] = rgb[r];
      }
norgb:
      if (handle->write_fun == write_ppm) {
	for (mag=r=0; r < 3; r++)
	  mag += (unsigned) img[r]*img[r];
	mag = sqrt(mag)/2;
	if (mag > 0xffff)
	    mag = 0xffff;
	img[3] = mag;
	handle->histogram[img[3] >> 3]++;
      }
    }
}

/*
   Write the image to a 24-bpp PPM file.
 */
void write_ppm(dcrawhandle *handle, FILE *ofp)
{
  int row, col, i, c, val, total;
  float max, mul, scale[0x10000];
  ushort *rgb;
  uchar (*ppm)[3];

/*
   Set the white point to the 99th percentile
 */
  i = handle->width * handle->height * (strcmp(handle->make,"FUJIFILM") ? 0.01 : 0.005);
  for (val=0x2000, total=0; --val; )
    if ((total += handle->histogram[val]) > i) break;
  max = val << 4;

  fprintf (ofp, "P6\n%d %d\n255\n",
	handle->width-handle->trim*2, handle->ymag*(handle->height-handle->trim*2));

  ppm = calloc (handle->width-handle->trim*2, 3);
  merror (handle, ppm, "write_ppm()");
  mul = handle->bright * 442 / max;
  scale[0] = 0;
  for (i=1; i < 0x10000; i++)
    scale[i] = mul * pow (i*2/max, handle->gamma_val-1);

  for (row=handle->trim; row < handle->height-handle->trim; row++) {
    for (col=handle->trim; col < handle->width-handle->trim; col++) {
      rgb = handle->image[row*handle->width+col];
      for (c=0; c < 3; c++) {
	val = rgb[c] * scale[rgb[3]];
	if (val > 255) val=255;
	ppm[col-handle->trim][c] = val;
      }
    }
    for (i=0; i < handle->ymag; i++)
      fwrite (ppm, handle->width-handle->trim*2, 3, ofp);
  }
  free(ppm);
}

/*
   Write the image to a 48-bpp Photoshop file.
 */
void write_psd(dcrawhandle *handle, FILE *ofp)
{
  char head[] = {
    '8','B','P','S',		/* signature */
    0,1,0,0,0,0,0,0,		/* version and reserved */
    0,3,			/* number of channels */
    0,0,0,0,			/* height, big-endian */
    0,0,0,0,			/* width, big-endian */
    0,16,			/* 16-bit color */
    0,3,			/* mode (1=grey, 3=rgb) */
    0,0,0,0,			/* color mode data */
    0,0,0,0,			/* image resources */
    0,0,0,0,			/* layer/mask info */
    0,0				/* no compression */
  };
  int hw[2], psize, row, col, c, val;
  ushort *buffer, *pred, *rgb;

  hw[0] = htonl(handle->height-handle->trim*2);	/* write the header */
  hw[1] = htonl(handle->width-handle->trim*2);
  memcpy (head+14, hw, sizeof hw);
  fwrite (head, 40, 1, ofp);

  psize = (handle->height-handle->trim*2) * (handle->width-handle->trim*2);
  buffer = calloc (6, psize);
  merror (handle, buffer, "write_psd()");
  pred = buffer;

  for (row = handle->trim; row < handle->height-handle->trim; row++) {
    for (col = handle->trim; col < handle->width-handle->trim; col++) {
      rgb = handle->image[row*handle->width+col];
      for (c=0; c < 3; c++) {
	val = rgb[c] * handle->bright;
	if (val > 0xffff)
	    val = 0xffff;
	pred[c*psize] = htons(val);
      }
      pred++;
    }
  }
  fwrite(buffer, psize, 6, ofp);
  free(buffer);
}

/*
   Write the image to a 48-bpp PPM file.
 */
void write_ppm16(dcrawhandle *handle, FILE *ofp)
{
  int row, col, c, val;
  ushort *rgb, (*ppm)[3];

  val = handle->rgb_max *handle-> bright;
  if (val < 256)
      val = 256;
  if (val > 0xffff)
      val = 0xffff;
  fprintf (ofp, "P6\n%d %d\n%d\n",
	handle->width-handle->trim*2, handle->height-handle->trim*2, val);

  ppm = calloc (handle->width-handle->trim*2, 6);
  merror (handle, ppm, "write_ppm16()");

  for (row = handle->trim; row < handle->height-handle->trim; row++) {
    for (col = handle->trim; col < handle->width-handle->trim; col++) {
      rgb = handle->image[row*handle->width+col];
      for (c=0; c < 3; c++) {
	val = rgb[c] * handle->bright;
	if (val > 0xffff)
	    val = 0xffff;
	ppm[col-handle->trim][c] = htons(val);
      }
    }
    fwrite (ppm, handle->width-handle->trim*2, 6, ofp);
  }
  free(ppm);
}

void (*fool_the_compiler)(dcrawhandle *handle)=foveon_interpolate;

/*
int main(int argc, char **argv)
{
  int arg, status=0, identify_only=0, write_to_stdout=0, half_size=0;
  dcrawhandle *handle;
  char opt, *ofname, *cp;
  const char *write_ext = ".ppm";
  FILE *ofp = stdout;

  if (argc == 1)
  {
    fprintf (stderr,
    "\nRaw Photo Decoder \"dcraw\" v5.87"
    "\nby Dave Coffin, dcoffin a cybercom o net"
    "\n\nUsage:  %s [options] file1 file2 ...\n"
    "\nValid options:"
    "\n-i        Identify files but don't decode them"
    "\n-c        Write to standard output"
    "\n-v        Print verbose messages while decoding"
    "\n-f        Interpolate RGBG as four colors"
    "\n-d        Document Mode (no color, no interpolation)"
    "\n-q        Quick, low-quality color interpolation"
    "\n-h        Half-size color image (3x faster than -q)"
    "\n-g <num>  Set gamma      (0.6 by default, only for 24-bpp output)"
    "\n-b <num>  Set brightness (1.0 by default)"
    "\n-a        Use automatic white balance"
    "\n-w        Use camera white balance, if possible"
    "\n-r <num>  Set red  multiplier (daylight = 1.0)"
    "\n-l <num>  Set blue multiplier (daylight = 1.0)"
    "\n-2        Write 24-bpp PPM (default)"
    "\n-3        Write 48-bpp PSD (Adobe Photoshop)"
    "\n-4        Write 48-bpp PPM"
    "\n\n", argv[0]);
    return 1;
  }
  handle=dcraw_createhandle ();
  merror (handle, "Couldn't allocate variable space", "main()");

  argv[argc] = "";
  for (arg=1; argv[arg][0] == '-'; ) {
    opt = argv[arg++][1];
    if (strchr ("gbrl", opt) && !isdigit(argv[arg][0])) {
      fprintf (stderr, "\"-%c\" requires a numeric argument.\n", opt);
      return 1;
    }
    switch (opt)
    {
      case 'g':  handle->gamma_val   = atof(argv[arg++]);  break;
      case 'b':  handle->bright      = atof(argv[arg++]);  break;
      case 'r':  handle->red_scale   = atof(argv[arg++]);  break;
      case 'l':  handle->blue_scale  = atof(argv[arg++]);  break;

      case 'i':  identify_only     = 1;  break;
      case 'c':  write_to_stdout   = 1;  break;
      case 'v':  handle->verbose           = 1;  break;
      case 'h':  half_size         = 1;
      case 'f':  handle->four_color_rgb    = 1;  break;
      case 'd':  handle->document_mode     = 1;  break;
      case 'q':  handle->quick_interpolate = 1;  break;
      case 'a':  handle->use_auto_wb       = 1;  break;
      case 'w':  handle->use_camera_wb     = 1;  break;

      case '2':  handle->write_fun = write_ppm;   write_ext = ".ppm";  break;
      case '3':  handle->write_fun = write_psd;   write_ext = ".psd";  break;
      case '4':  handle->write_fun = write_ppm16; write_ext = ".ppm";  break;

      default:
	fprintf (stderr, "Unknown option \"-%c\".\n", opt);
	return 1;
    }
  }
  if (arg == argc) {
    fprintf (stderr, "No files to process.\n");
    return 1;
  }
  if (write_to_stdout) {
    if (isatty(1)) {
      fprintf (stderr, "Will not write an image to the terminal!\n");
      return 1;
    }
#if defined(WIN32) || defined(DJGPP)
    if (setmode(1,O_BINARY) < 0) {
      perror("setmode()");
      return 1;
    }
#endif
  }

  for ( ; arg < argc; arg++)
  {
    status = 1;
    handle->image = NULL;
    if (setjmp (handle->failure)) {
      if (fileno(handle->ifp) > 2) fclose(handle->ifp);
      if (fileno(ofp) > 2) fclose(ofp);
      if (handle->image) free(handle->image);
      status = 1;
      continue;
    }
    handle->ifname = argv[arg];
    if (!(handle->ifp = fopen (handle->ifname, "rb"))) {
      perror (handle->ifname);
      continue;
    }
    if ((status = identify(handle))) {
      fclose(handle->ifp);
      continue;
    }
    if (identify_only) {
      fprintf (stderr, "%s is a %s %s image.\n", handle->ifname, handle->make, handle->model);
      fclose(handle->ifp);
      continue;
    }
    handle->shrink = half_size && handle->filters;
    handle->iheight = (handle->height + handle->shrink) >> handle->shrink;
    handle->iwidth  = (handle->width  + handle->shrink) >> handle->shrink;
    handle->image = calloc (handle->iheight * handle->iwidth, sizeof *(handle->image));
    merror (handle, handle->image, "main()");
    if (handle->verbose)
      fprintf (stderr,
	"Loading %s %s image from %s...\n", handle->make, handle->model, handle->ifname);
    (*handle->load_raw)(handle);
    fclose(handle->ifp);
    bad_pixels(handle);
    handle->height = handle->iheight;
    handle->width  = handle->iwidth;
    if (handle->is_foveon) {
      if (handle->verbose)
	fprintf (stderr, "Foveon interpolation...\n");
      foveon_interpolate(handle);
    } else {
      scale_colors(handle);
    }
    if (handle->shrink) handle->filters = 0;
    handle->trim = 0;
    if (handle->filters && !handle->document_mode) {
      handle->trim = 1;
      if (handle->verbose)
	fprintf (stderr, "%s interpolation...\n",
	  handle->quick_interpolate ? "Bilinear":"VNG");
      vng_interpolate(handle);
    }
    if (handle->verbose)
      fprintf (stderr, "Converting to RGB colorspace...\n");
    convert_to_rgb(handle);
    ofname = malloc (strlen(handle->ifname) + 16);
    merror (handle, ofname, "main()");
    if (write_to_stdout)
      strcpy (ofname, "standard output");
    else {
      strcpy (ofname, handle->ifname);
      if ((cp = strrchr (ofname, '.'))) *cp = 0;
      strcat (ofname, write_ext);
      ofp = fopen (ofname, "wb");
      if (!ofp) {
	status = 1;
	perror(ofname);
	goto cleanup;
      }
    }
    if (handle->verbose)
      fprintf (stderr, "Writing data to %s...\n", ofname);
    (*handle->write_fun)(handle, ofp);
    if (ofp != stdout)
      fclose(ofp);
cleanup:
    free(ofname);
    free(handle->image);
	free(handle);
  }
  return status;
}
*/
#endif
