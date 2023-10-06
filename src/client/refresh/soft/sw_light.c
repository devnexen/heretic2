/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 */

#include "header/local.h"

vec3_t lightspot;
light_t	*blocklights = NULL, *blocklight_max = NULL;

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

static void
R_MarkSurfaceLights(dlight_t *light, int bit, mnode_t *node, int r_dlightframecount)
{
	msurface_t	*surf;
	int			i;

	// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}
}

/*
=============
R_PushDlights
=============
*/
void
R_PushDlights (const model_t *model)
{
	int		i;
	dlight_t	*l;

	for (i=0, l = r_newrefdef.dlights ; i<r_newrefdef.num_dlights ; i++, l++)
	{
		R_MarkLights ( l, 1<<i,
			model->nodes + model->firstnode,
			r_framecount, R_MarkSurfaceLights);
	}
}

static void
R_AddDynamicLights(drawsurf_t* drawsurf)
{
	msurface_t 	*surf;
	int		lnum;
	int		smax, tmax;
	mtexinfo_t	*tex;

	surf = drawsurf->surf;
	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	tex = surf->texinfo;

	if (blocklight_max <= blocklights + smax*tmax*3)
	{
		r_outoflights = true;
		return;
	}

	for (lnum=0; lnum < r_newrefdef.num_dlights; lnum++)
	{
		vec3_t impact, local, color;
		float	dist, rad, minlight;
		int		t;
		int		i;
		dlight_t	*dl;
		int		negativeLight;
		light_t *plightdest = blocklights;

		if (!(surf->dlightbits & (1<<lnum)))
			continue;	// not lit by this light

		dl = &r_newrefdef.dlights[lnum];
		rad = dl->intensity;

		if(r_colorlight->value == 0)
		{
			for(i=0; i<3; i++)
				color[i] = 256;
		}
		else
		{
			for(i=0; i<3; i++)
				color[i] = 256 * dl->color[i];
		}

		//=====
		negativeLight = 0;
		if(rad < 0)
		{
			negativeLight = 1;
			rad = -rad;
		}
		//=====

		dist = DotProduct (dl->origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = DLIGHT_CUTOFF;	// dl->minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = dl->origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0 ; t < tmax ; t++)
		{
			int s, td;
			td = local[1] - t * (1 << surf->lmshift);
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				int sd;

				sd = local[0] - s * (1 << surf->lmshift);
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);

				for (i=0; i<3; i++)
				{
					//====
					if(!negativeLight)
					{
						if (dist < minlight)
							*plightdest += (rad - dist) * color[i];

					}
					else
					{
						if (dist < minlight)
							*plightdest -= (rad - dist) * color[i];
						if(*plightdest < minlight)
							*plightdest = minlight;
					}
					//====
					plightdest ++;
				}
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void
R_BuildLightMap (drawsurf_t* drawsurf)
{
	int			smax, tmax;
	int			size;
	byte		*lightmap;
	msurface_t	*surf;

	surf = drawsurf->surf;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax*tmax*3;

	if (blocklight_max <= blocklights + size)
	{
		r_outoflights = true;
		return;
	}

	// clear to no light
	memset(blocklights, 0, size * sizeof(light_t));

	if (r_fullbright->value || !r_worldmodel->lightdata)
	{
		return;
	}

	// add all the lightmaps
	lightmap = surf->samples;
	if (lightmap)
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			unsigned scale;
			light_t  *curr_light, *max_light;

			curr_light = blocklights;
			max_light = blocklights + size;

			scale = drawsurf->lightadj[maps];	// 8.8 fraction

			if(r_colorlight->value == 0)
			{
				do
				{
					light_t light;

					light = lightmap[0];
					if (light < lightmap[1])
						light = lightmap[1];
					if (light < lightmap[2])
						light = lightmap[2];

					light *= scale;

					*curr_light += light;
					curr_light++;
					*curr_light += light;
					curr_light++;
					*curr_light += light;
					curr_light++;

					lightmap += 3; /* skip to next lightmap */
				}
				while(curr_light < max_light);
			}
			else
			{
				do
				{
					*curr_light += *lightmap * scale;
					curr_light++;
					lightmap ++; /* skip to next lightmap */
				}
				while(curr_light < max_light);
			}
		}
	}
	else
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			unsigned scale;
			light_t  *curr_light, *max_light;

			curr_light = blocklights;
			max_light = blocklights + size;

			scale = drawsurf->lightadj[maps];	// 8.8 fraction

			do
			{
				*curr_light += 255 * scale;
				curr_light++;
			}
			while(curr_light < max_light);
		}
	}

	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (drawsurf);

	// bound, invert, and shift
	{
		light_t  *curr_light, *max_light;

		curr_light = blocklights;
		max_light = blocklights + size;

		do
		{
			int t;

			t = (int)*curr_light;

			if (t < 0)
				t = 0;
			t = (255*256 - t) >> (8 - VID_CBITS);

			if (t < (1 << 6))
				t = (1 << 6);

			*curr_light = t;
			curr_light++;
		}
		while(curr_light < max_light);
	}
}
