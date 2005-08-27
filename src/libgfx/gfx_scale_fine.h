/* gfx_scale_fine.h - A presumeably fast scaling routine by Björn Östberg.
 * Doesn't work at the moment.
 *
 * GFXIndex (c) 1999-2004 Fredrik Rambris <fredrik@rambris.com>.
 * All rights reserved.
 *
 * GFXIndex is a tool that creates thumbnails and HTML-indexes of your images. 
 *
 * This is licensed under GNU GPL.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

void gfx_scale_fine( struct image *src_img, int src_x, int src_y, int src_width, int src_height, struct image *dst_img, int dst_x, int dst_y, int dst_width, int dst_height )
{
	float flatio_x, flatio_y, flatio_xy;
	unsigned int ratio_x, ratio_y, px=0, py=0, old_px, old_py, x=0, y=0, t;
	unsigned int r0,g0,b0, r1,g1,b1, m16=16777216, dx=0,dy=0, ax0=0, ax1=0, ay0=0, ay1=0;
	struct color col={0,0,0,255};
	
	flatio_x=src_width/dst_width; ratio_x=flatio_x*m16;
	flatio_y=src_height/dst_height; ratio_y=flatio_y*m16;
	flatio_xy=flatio_x*flatio_y;

	for(;;)
	{
		old_px=px;
		old_py=py;
		px+=ratio_x; if(px>16777215) { t=px/m16; x+=t; px-=t*m16; }
		py+=ratio_y; if(py>16777215) { t=py/m16; y+=t; py-=t*m16; }
		
		ax0=m16-px; ax1=px;
		ay0=m16-py; ay1=py;
		
		gfx_readpixel( src_img, x, y, &col );
		r0=(col.r*ax0)/m16;
		g0=(col.g*ax0)/m16;
		b0=(col.b*ax0)/m16;
		
		gfx_readpixel( src_img, x+1, y, &col );
		r0+=(col.r*ax1)/m16;
		g0+=(col.g*ax1)/m16;
		b0+=(col.b*ax1)/m16;
		
		gfx_readpixel( src_img, x, y+1, &col );
		r1=(col.r*ax0)/m16;
		g1=(col.g*ax0)/m16;
		b1=(col.b*ax0)/m16;
		
		gfx_readpixel( src_img, x+1, y+1, &col );
		r1+=(col.r*ax1)/m16;
		g1+=(col.g*ax1)/m16;
		b1+=(col.b*ax1)/m16;

		col.r=((r0*ay0)/m16+(r1*ay1)/m16);
		col.g=((g0*ay0)/m16+(g1*ay1)/m16);
		col.b=((b0*ay0)/m16+(b1*ay1)/m16);
		
		gfx_writepixel( dst_img, dx, dy, &col );
		dx++; if(dx>=dst_width) { dx=0; dy++;}
		if(dy>=dst_height) break;
	}
}
