#undef floor
double floor( double x )
{
	long i=x;
	return( (double)i );
}

void gdImageCopyResampled ( struct image *dst,
		      struct image *src,
		      int dstX, int dstY,
		      int srcX, int srcY,
		      int dstW, int dstH,
		      int srcW, int srcH,
			  BOOL apply_alpha)
{
  int x, y;
  float sx, sy;
  struct color col;
  col.opacity=255;
  for (y = dstY; (y < dstY + dstH); y++)
    {
      for (x = dstX; (x < dstX + dstW); x++)
	{
	  float sy1, sy2, sx1, sx2;
	  float spixels = 0;
	  float red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
	  sy1 = ((float) y - (float) dstY) * (float) srcH /
	    (float) dstH;
	  sy2 = ((float) (y + 1) - (float) dstY) * (float) srcH /
	    (float) dstH;
	  sy = sy1;
	  do
	    {
	      float yportion;
	      if (floor (sy) == floor (sy1))
		{
		  yportion = 1.0 - (sy - floor (sy));
		  if (yportion > sy2 - sy1)
		    {
		      yportion = sy2 - sy1;
		    }
		  sy = floor (sy);
		}
	      else if (sy == floor (sy2))
		{
		  yportion = sy2 - floor (sy2);
		}
	      else
		{
		  yportion = 1.0;
		}
	      sx1 = ((float) x - (float) dstX) * (float) srcW /
		dstW;
	      sx2 = ((float) (x + 1) - (float) dstX) * (float) srcW /
		dstW;
	      sx = sx1;
	      do
		{
		  float xportion;
		  float pcontribution;
		  //int p;
		  if (floor (sx) == floor (sx1))
		    {
		      xportion = 1.0 - (sx - floor (sx));
		      if (xportion > sx2 - sx1)
			{
			  xportion = sx2 - sx1;
			}
		      sx = floor (sx);
		    }
		  else if (sx == floor (sx2))
		    {
		      xportion = sx2 - floor (sx2);
		    }
		  else
		    {
		      xportion = 1.0;
		    }
		  pcontribution = xportion * yportion;
		  gfx_readpixel( src, sx, sy, &col );
		  red += col.r * pcontribution;
		  green += col.g * pcontribution;
		  blue += col.b * pcontribution;
		  alpha += col.opacity * pcontribution;
		  spixels += xportion * yportion;
		  sx += 1.0;
		}
	      while (sx < sx2);
	      sy += 1.0;
	    }
	  while (sy < sy2);
	  if (spixels != 0.0)
	    {
	      red /= spixels;
	      green /= spixels;
	      blue /= spixels;
	      alpha /= spixels;
	    }
	  /* Clamping to allow for rounding errors above */
	  if (red > 255.0)
	    {
	      red = 255.0;
	    }
	  if (green > 255.0)
	    {
	      green = 255.0;
	    }
	  if (blue > 255.0)
	    {
	      blue = 255.0;
	    }
	  if (alpha > 255)
	    {
	      alpha = 255;
	    }
	  col.r=red;
	  col.g=green;
	  col.b=blue;
	  col.opacity=alpha;
	  if( apply_alpha ) gfx_mixpixel( dst, x, y, &col );
	  else gfx_writepixel( dst, x, y, &col );
	}
    }
}
