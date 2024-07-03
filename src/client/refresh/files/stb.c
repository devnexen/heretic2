/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2015 Daniel Gibson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * File formats supported by stb_image, for now only tga, png, jpg
 * See also https://github.com/nothings/stb
 *
 * =======================================================================
 */

#include <stdlib.h>

#include "../ref_shared.h"

// include resize implementation
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

/*
 * Add extension to file name
 */
void
FixFileExt(const char *origname, const char *ext, char *filename, size_t size)
{
	Q_strlcpy(filename, origname, size);

	/* Add the extension */
	if (strcmp(COM_FileExtension(filename), ext))
	{
		Q_strlcat(filename, ".", size);
		Q_strlcat(filename, ext, size);
	}
}

/*
 * origname: the filename to be opened, might be without extension
 * type: extension of the type we wanna open ("jpg", "png" or "tga")
 * pic: pointer RGBA pixel data will be assigned to
 */
qboolean
LoadSTB(const char *origname, const char* type, byte **pic, int *width, int *height)
{
	int w, h, bytesPerPixel;
	char filename[256];
	byte* data = NULL;

	FixFileExt(origname, type, filename, sizeof(filename));

	*pic = NULL;

	ri.VID_ImageDecode(filename, &data, NULL, &w, &h, &bytesPerPixel);
	if (data == NULL)
	{
		return false;
	}

	if (bytesPerPixel != 4)
	{
		free(data);

		R_Printf(PRINT_ALL, "%s unexpected file format of %s with %d bytes per pixel!\n",
			__func__, filename, bytesPerPixel);

		return false;
	}

	R_Printf(PRINT_DEVELOPER, "%s() loaded: %s\n", __func__, filename);

	*pic = data;
	*width = w;
	*height = h;
	return true;
}

qboolean
ResizeSTB(const byte *input_pixels, int input_width, int input_height,
			  byte *output_pixels, int output_width, int output_height)
{
	if (stbir_resize_uint8(input_pixels, input_width, input_height, 0,
			       output_pixels, output_width, output_height, 0, 4))
	{
		return true;
	}

	return false;
}

// We have 16 color palette, 256 / 16 should be enough
#define COLOR_DISTANCE 16

void
SmoothColorImage(unsigned *dst, size_t size, size_t rstep)
{
	const unsigned *full_size;
	unsigned last_color;
	unsigned *last_diff;

	// maximum step for apply
	if (rstep < 2)
	{
		return;
	}

	// step one pixel back as with check one pixel more
	full_size = dst + size - rstep - 1;
	last_diff = dst;
	last_color = *dst;

	// skip current point
	dst ++;

	while (dst < full_size)
	{
		if (last_color != *dst)
		{
			int step = dst - last_diff;
			if (step > 1)
			{
				int a_beg, b_beg, c_beg, d_beg;
				int a_end, b_end, c_end, d_end;
				int a_step, b_step, c_step, d_step;
				int k;

				// minimize effect size to rstep
				if (step > rstep)
				{
					// change place for start effect
					last_diff += (step - rstep);
					step = rstep;
				}

				// compare next pixels
				for(k = 1; k <= step; k ++)
				{
					if (dst[k] != dst[0])
					{
						break;
					}
				}

				// step back as pixel different after previous step
				k --;

				// mirror steps
				if (k < step)
				{
					// R_Printf(PRINT_ALL, "%s %d -> %d\n", __func__, k, step);
					// change place for start effect
					last_diff += (step - k);
					step = k;
				}

				// update step to correct value
				step += k;
				dst += k;

				// get colors
				a_beg = (last_color >> 0 ) & 0xff;
				b_beg = (last_color >> 8 ) & 0xff;
				c_beg = (last_color >> 16) & 0xff;
				d_beg = (last_color >> 24) & 0xff;

				a_end = (*dst >> 0 ) & 0xff;
				b_end = (*dst >> 8 ) & 0xff;
				c_end = (*dst >> 16) & 0xff;
				d_end = (*dst >> 24) & 0xff;

				a_step = a_end - a_beg;
				b_step = b_end - b_beg;
				c_step = c_end - c_beg;
				d_step = d_end - d_beg;

				if ((abs(a_step) <= COLOR_DISTANCE) &&
				    (abs(b_step) <= COLOR_DISTANCE) &&
				    (abs(c_step) <= COLOR_DISTANCE) &&
				    (abs(d_step) <= COLOR_DISTANCE) &&
				    step > 0)
				{
					// generate color change steps
					a_step = (a_step << 16) / step;
					b_step = (b_step << 16) / step;
					c_step = (c_step << 16) / step;
					d_step = (d_step << 16) / step;

					// apply color changes
					for (k=0; k < step; k++)
					{
						*last_diff = (((a_beg + ((a_step * k) >> 16)) << 0) & 0x000000ff) |
									 (((b_beg + ((b_step * k) >> 16)) << 8) & 0x0000ff00) |
									 (((c_beg + ((c_step * k) >> 16)) << 16) & 0x00ff0000) |
									 (((d_beg + ((d_step * k) >> 16)) << 24) & 0xff000000);
						last_diff++;
					}
				}
			}
			last_color = *dst;
			last_diff = dst;
		}
		dst ++;
	}
}

/* https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms */

void
scale2x(const byte *src, byte *dst, int width, int height)
{
	/*
		EPX/Scale2×/AdvMAME2×

		x A x
		C P B -> 1 2
		x D x    3 4

		1=P; 2=P; 3=P; 4=P;
		IF C==A AND C!=D AND A!=B => 1=A
		IF A==B AND A!=C AND B!=D => 2=B
		IF D==C AND D!=B AND C!=A => 3=C
		IF B==D AND B!=A AND D!=C => 4=D
	*/
	{
		const byte *in_buff = src;
		byte *out_buff = dst;
		const byte *out_buff_full = dst + ((width * height) << 2);

		while (out_buff < out_buff_full)
		{
			int x;
			for (x = 0; x < width; x ++)
			{
				// copy one source byte to two destinatuion bytes
				*out_buff = *in_buff;
				out_buff ++;
				*out_buff = *in_buff;
				out_buff ++;

				// next source pixel
				in_buff ++;
			}
			// copy last line one more time
			memcpy(out_buff, out_buff - (width << 1), width << 1);
			out_buff += width << 1;
		}
	}

	{
		int y, h, w;
		h = height - 1;
		w = width - 1;
		for (y = 0; y < height; y ++)
		{
			int x;
			for (x = 0; x < width; x ++)
			{
				byte a, b, c, d, p;

				p = src[(width * (y    )) + (x    )];
				a = (y > 0) ? src[(width * (y - 1)) + (x    )] : p;
				b = (x < w) ? src[(width * (y    )) + (x + 1)] : p;
				c = (x > 0) ? src[(width * (y    )) + (x - 1)] : p;
				d = (y < h) ? src[(width * (y + 1)) + (x    )] : p;

				if ((c == a) && (c != d) && (a != b))
				{
					dst[(2 * width * ((y * 2)    )) + ((x * 2)    )] = a;
				}

				if ((a == b) && (a != c) && (b != d))
				{
					dst[(2 * width * ((y * 2)    )) + ((x * 2) + 1)] = b;
				}

				if ((d == c) && (d != b) && (c != a))
				{
					dst[(2 * width * ((y * 2) + 1)) + ((x * 2)    )] = c;
				}

				if ((b == d) && (b != a) && (d != c))
				{
					dst[(2 * width * ((y * 2) + 1)) + ((x * 2) + 1)] = d;
				}
			}
		}
	}
}

void
scale3x(const byte *src, byte *dst, int width, int height)
{
	/*
		Scale3×/AdvMAME3× and ScaleFX

		A B C    1 2 3
		D E F -> 4 5 6
		G H I    7 8 9


		1=E; 2=E; 3=E; 4=E; 5=E; 6=E; 7=E; 8=E; 9=E;
		IF D==B AND D!=H AND B!=F => 1=D
		IF (D==B AND D!=H AND B!=F AND E!=C) OR (B==F AND B!=D AND F!=H AND E!=A) => 2=B
		IF B==F AND B!=D AND F!=H => 3=F
		IF (H==D AND H!=F AND D!=B AND E!=A) OR (D==B AND D!=H AND B!=F AND E!=G) => 4=D
		5=E
		IF (B==F AND B!=D AND F!=H AND E!=I) OR (F==H AND F!=B AND H!=D AND E!=C) => 6=F
		IF H==D AND H!=F AND D!=B => 7=D
		IF (F==H AND F!=B AND H!=D AND E!=G) OR (H==D AND H!=F AND D!=B AND E!=I) => 8=H
		IF F==H AND F!=B AND H!=D => 9=F
	*/
	{
		const byte *in_buff = src;
		byte *out_buff = dst;
		const byte *out_buff_full = dst + ((width * height) * 9);

		while (out_buff < out_buff_full)
		{
			int x;
			for (x = 0; x < width; x ++)
			{
				// copy one source byte to two destinatuion bytes
				*out_buff = *in_buff;
				out_buff ++;
				*out_buff = *in_buff;
				out_buff ++;
				*out_buff = *in_buff;
				out_buff ++;

				// next source pixel
				in_buff ++;
			}
			// copy last line one more time
			memcpy(out_buff, out_buff - (width * 3), width * 3);
			out_buff += width * 3;
			// copy last line one more time
			memcpy(out_buff, out_buff - (width * 3), width * 3);
			out_buff += width * 3;
		}
	}

	{
		int y, z, w;
		z = height - 1;
		w = width - 1;
		for (y = 0; y < height; y ++)
		{
			int x;
			for (x = 0; x < width; x ++)
			{
				byte a, b, c, d, e, f, g, h, i;

				e = src[(width * y) + x];

				a = ((y > 0) && (x > 0)) ? src[(width * (y - 1)) + (x - 1)] : e;
				b = ((y > 0) && (x    )) ? src[(width * (y - 1)) + (x    )] : e;
				c = ((y > 0) && (x < w)) ? src[(width * (y - 1)) + (x + 1)] : e;

				d = (           (x > 0)) ? src[(width * (y    )) + (x - 1)] : e;
				f = (           (x < w)) ? src[(width * (y    )) + (x + 1)] : e;

				g = ((y < z) && (x > 0)) ? src[(width * (y + 1)) + (x - 1)] : e;
				h = ((y < z) && (x    )) ? src[(width * (y + 1)) + (x    )] : e;
				i = ((y < z) && (x < w)) ? src[(width * (y + 1)) + (x + 1)] : e;

				if ((d == b) && (d != h) && (b != f))
				{
					dst[(3 * width * ((y * 3)    )) + ((x * 3)    )] = d;
				}

				if (((d == b) && (d != h) && (b != f) && (e != c)) ||
					((b == f) && (b != d) && (f != h) && (e != a)))
				{
					dst[(3 * width * ((y * 3)    )) + ((x * 3) + 1)] = b;
				}

				if ((b == f) && (b != d) && (f != h))
				{
					dst[(3 * width * ((y * 3)    )) + ((x * 3) + 2)] = f;
				}

				if (((h == d) && (h != f) && (d != b) && (e != a)) ||
					((d == b) && (d != h) && (b != f) && (e != g)))
				{
					dst[(3 * width * ((y * 3) + 1)) + ((x * 3)    )] = d;
				}

				if (((b == f) && (b != d) && (f != h) && (e != i)) ||
					((f == h) && (f != b) && (h != d) && (e != c)))
				{
					dst[(3 * width * ((y * 3) + 1)) + ((x * 3) + 2)] = f;
				}

				if ((h == d) && (h != f) && (d != b))
				{
					dst[(3 * width * ((y * 3) + 2)) + ((x * 3)    )] = d;
				}

				if (((f == h) && (f != b) && (h != d) && (e != g)) ||
					((h == d) && (h != f) && (d != b) && (e != i)))
				{
					dst[(3 * width * ((y * 3) + 2)) + ((x * 3) + 1)] = h;
				}

				if ((f == h) && (f != b) && (h != d))
				{
					dst[(3 * width * ((y * 3) + 2)) + ((x * 3) + 2)] = f;
				}
			}
		}
	}
}

static struct image_s *
LoadHiColorImage(const char *name, const char* namewe, const char *ext,
	imagetype_t type, loadimage_t load_image)
{
	int realwidth = 0, realheight = 0;
	int width = 0, height = 0;
	struct image_s	*image = NULL;
	byte *pic = NULL;

	/* Get size of the original texture */
	if (strcmp(ext, "pcx") == 0)
	{
		GetPCXInfo(name, &realwidth, &realheight);
	}
	else if (strcmp(ext, "wal") == 0)
	{
		GetWalInfo(name, &realwidth, &realheight);
	}
	else if (strcmp(ext, "m8") == 0)
	{
		GetM8Info(name, &realwidth, &realheight);
	}
	else if (strcmp(ext, "m32") == 0)
	{
		GetM32Info(name, &realwidth, &realheight);
	}
	else if (strcmp(ext, "swl") == 0)
	{
		GetSWLInfo(name, &realwidth, &realheight);
	}

	/* try to load a tga, png or jpg (in that order/priority) */
	if (  LoadSTB(namewe, "tga", &pic, &width, &height)
	   || LoadSTB(namewe, "png", &pic, &width, &height)
	   || LoadSTB(namewe, "jpg", &pic, &width, &height) )
	{
		if (width >= realwidth && height >= realheight)
		{
			if (realheight == 0 || realwidth == 0)
			{
				realheight = height;
				realwidth = width;
			}

			image = load_image(name, pic,
				width, realwidth,
				height, realheight,
				width * height,
				type, 32);
		}
	}

	if (pic)
	{
		free(pic);
	}

	return image;
}

static struct image_s *
LoadImage_Ext(const char *name, const char* namewe, const char *ext, imagetype_t type,
	int r_retexturing, loadimage_t load_image)
{
	struct image_s	*image = NULL;

	// with retexturing and not skin
	if (r_retexturing)
	{
		image = LoadHiColorImage(name, namewe, ext, type, load_image);
	}

	if (!image)
	{
		if (!strcmp(ext, "pcx") || !strcmp(ext, "swl"))
		{
			int width = 0, height = 0, realwidth = 0, realheight = 0;
			byte	*pic = NULL;
			byte	*palette = NULL;

			if (!strcmp(ext, "pcx"))
			{
				LoadPCX (namewe, &pic, &palette, &width, &height);
			}
			else if (!strcmp(ext, "swl"))
			{
				LoadSWL (namewe, &pic, &palette, &width, &height);
			}

			if (!pic)
			{
				return NULL;
			}

			realheight = height;
			realwidth = width;

			if (r_retexturing >= 2)
			{
				byte *image_scale = NULL;

				/* scale image paletted images */
				image_scale = malloc(width * height * 4);
				scale2x(pic, image_scale, width, height);

				/* replace pic with scale */
				free(pic);
				pic = image_scale;

				width *= 2;
				height *= 2;
			}

			if (r_retexturing && palette)
			{
				byte *image_buffer = NULL;
				int i, size;

				size = width * height;

				/* convert to full color */
				image_buffer = malloc (size * 4);
				for(i = 0; i < size; i++)
				{
					unsigned char value = pic[i];
					image_buffer[i * 4 + 0] = palette[value * 3 + 0];
					image_buffer[i * 4 + 1] = palette[value * 3 + 1];
					image_buffer[i * 4 + 2] = palette[value * 3 + 2];
					image_buffer[i * 4 + 3] = value == 255 ? 0 : 255;
				}

				image = load_image(name, image_buffer,
					width, realwidth,
					height, realheight,
					size, type, 32);
				free (image_buffer);
			}
			else
			{
				image = load_image(name, pic,
					width, width,
					height, height,
					width * height, type, 8);
			}

			if (palette)
			{
				free(palette);
			}
			free(pic);
		}
		else if (!strcmp(ext, "wal"))
		{
			image = LoadWal(name, namewe, type, load_image);
		}
		else if (!strcmp(ext, "m8"))
		{
			image = LoadM8(name, namewe, type, load_image);
		}
		else if (!strcmp(ext, "tga") ||
		         !strcmp(ext, "m32") ||
		         !strcmp(ext, "png") ||
		         !strcmp(ext, "jpg"))
		{
			byte *pic = NULL;
			int width = 0, height = 0;

			if (LoadSTB (namewe, ext, &pic, &width, &height) && pic)
			{
				image = load_image(name, pic,
					width, width,
					height, height,
					width * height,
					type, 32);

				free(pic);
			}
		}
	}

	return image;
}

struct image_s *
R_LoadImage(const char *name, const char* namewe, const char *ext, imagetype_t type,
	int r_retexturing, loadimage_t load_image)
{
	struct image_s	*image = NULL;

	/* original name */
	image = LoadImage_Ext(name, namewe, ext, type, r_retexturing, load_image);

	/* pcx check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "pcx", type, r_retexturing, load_image);
	}

	/* wal check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "wal", type, r_retexturing, load_image);
	}

	/* tga check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "tga", type, r_retexturing, load_image);
	}

	/* m32 check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "m32", type, r_retexturing, load_image);
	}

	/* m8 check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "m8", type, r_retexturing, load_image);
	}

	/* swl check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "swl", type, r_retexturing, load_image);
	}

	/* png check */
	if (!image)
	{
		image = LoadImage_Ext(name, namewe, "png", type, r_retexturing, load_image);
	}

	return image;
}

struct image_s*
GetSkyImage(const char *skyname, const char* surfname, qboolean palettedtexture,
	findimage_t find_image)
{
	struct image_s	*image = NULL;
	char	pathname[MAX_QPATH];

	/* Quake 2 */
	if (palettedtexture)
	{
		Com_sprintf(pathname, sizeof(pathname), "env/%s%s.pcx",
				skyname, surfname);
		image = find_image(pathname, it_sky);
	}

	if (!image)
	{
		Com_sprintf(pathname, sizeof(pathname), "env/%s%s.tga",
				skyname, surfname);
		image = find_image(pathname, it_sky);
	}

	/* Heretic 2 */
	if (!image)
	{
		Com_sprintf(pathname, sizeof(pathname), "pics/Skies/%s%s.m32",
			skyname, surfname);
		image = find_image(pathname, it_sky);
	}

	/* Anachronox */
	if (!image)
	{
		Com_sprintf(pathname, sizeof(pathname), "graphics/sky/%s%s.tga",
			skyname, surfname);
		image = find_image(pathname, it_sky);
	}

	/* Daikatana */
	if (!image)
	{
		Com_sprintf(pathname, sizeof(pathname), "env/32bit/%s%s.tga",
			skyname, surfname);
		image = find_image(pathname, it_sky);
	}

	return image;
}

struct image_s *
GetTexImage(const char *name, findimage_t find_image)
{
	char	pathname[MAX_QPATH];

	/* Quake 2 */
	Com_sprintf(pathname, sizeof(pathname), "textures/%s.wal", name);
	return find_image(pathname, it_wall);
}

struct image_s *
R_FindPic(const char *name, findimage_t find_image)
{
	struct image_s	*image = NULL;

	if ((name[0] != '/') && (name[0] != '\\'))
	{
		char	pathname[MAX_QPATH];
		char	namewe[MAX_QPATH];
		const char* ext;

		ext = COM_FileExtension(name);
		if(!ext[0])
		{
			/* file has no extension */
			strncpy(namewe, name, sizeof(namewe) - 1);
			namewe[sizeof(namewe) - 1] = 0;
		}
		else
		{
			int len;

			len = strlen(name);

			/* Remove the extension */
			memset(namewe, 0, MAX_QPATH);
			memcpy(namewe, name, len - (strlen(ext) + 1));
			namewe[len - (strlen(ext))] = 0;
		}

		/* Quake 2 */
		Com_sprintf(pathname, sizeof(pathname), "pics/%s.pcx", namewe);
		image = find_image(pathname, it_pic);

		/* Anachronox */
		if (!image)
		{
			Com_sprintf(pathname, sizeof(pathname), "graphics/%s.pcx", namewe);
			image = find_image(pathname, it_pic);
		}

		/* Heretic 2 */
		if (!image)
		{
			Com_sprintf(pathname, sizeof(pathname), "pics/misc/%s.m32", name);
			image = find_image(pathname, it_pic);
		}
	}
	else
	{
		image = find_image(name + 1, it_pic);
	}

	return image;
}
