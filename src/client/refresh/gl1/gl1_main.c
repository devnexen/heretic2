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
 * =======================================================================
 *
 * Refresher setup and main part of the frame generation
 *
 * =======================================================================
 */

#include "header/local.h"

#define NUM_BEAM_SEGS 6

viddef_t vid;
model_t *r_worldmodel;

float gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t gl_state;

image_t *r_notexture; /* use for bad textures */
image_t *r_particletexture; /* little dot for particles */

cplane_t frustum[4];

int r_visframecount; /* bumped when going to a new PVS */
int r_framecount; /* used for dlight push checking */

int c_brush_polys, c_alias_polys;

float v_blend[4]; /* final blending color */

void R_Strings(void);

/* view origin */
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

/* screen size info */
refdef_t r_newrefdef;

int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;
unsigned r_rawpalette[256];

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_fullbright;
cvar_t *r_novis;
cvar_t *r_lerpmodels;
cvar_t *gl_lefthand;
cvar_t *r_gunfov;
cvar_t *r_farsee;
cvar_t *r_validation;

cvar_t *r_lightlevel;
cvar_t *gl1_overbrightbits;

cvar_t *gl1_particle_min_size;
cvar_t *gl1_particle_max_size;
cvar_t *gl1_particle_size;
cvar_t *gl1_particle_att_a;
cvar_t *gl1_particle_att_b;
cvar_t *gl1_particle_att_c;
cvar_t *gl1_particle_square;

cvar_t *gl1_palettedtexture;
cvar_t *gl1_pointparameters;

cvar_t *gl_drawbuffer;
cvar_t *gl_lightmap;
cvar_t *gl_shadows;
cvar_t *gl1_stencilshadow;
cvar_t *r_mode;
cvar_t *r_fixsurfsky;

cvar_t *r_customwidth;
cvar_t *r_customheight;

cvar_t *r_retexturing;
cvar_t *r_scale8bittextures;

cvar_t *gl_nolerp_list;
cvar_t *r_lerp_list;
cvar_t *r_2D_unfiltered;
cvar_t *r_videos_unfiltered;

cvar_t *gl1_dynamic;
cvar_t *r_modulate;
cvar_t *gl_nobind;
cvar_t *gl1_round_down;
cvar_t *gl1_picmip;
cvar_t *gl_showtris;
cvar_t *gl_showbbox;
cvar_t *gl1_ztrick;
cvar_t *gl_zfix;
cvar_t *gl_finish;
cvar_t *r_clear;
cvar_t *r_cull;
cvar_t *gl_polyblend;
cvar_t *gl1_flashblend;
cvar_t *gl1_saturatelighting;
cvar_t *r_vsync;
cvar_t *gl_texturemode;
cvar_t *gl1_texturealphamode;
cvar_t *gl1_texturesolidmode;
cvar_t *gl_anisotropic;
cvar_t *r_lockpvs;
cvar_t *gl_msaa_samples;

cvar_t *vid_fullscreen;
cvar_t *vid_gamma;

cvar_t *gl1_stereo;
cvar_t *gl1_stereo_separation;
cvar_t *gl1_stereo_anaglyph_colors;
cvar_t *gl1_stereo_convergence;


refimport_t ri;

cvar_t *vid_ref;
cvar_t *gl_drawbuffer;
cvar_t *gl_lightmap;
cvar_t *r_shadows;
cvar_t* r_fog_underwater_density;
cvar_t* r_fog_underwater_color_r;
cvar_t* r_fog_underwater_color_g;
cvar_t* r_fog_underwater_color_b;
cvar_t* r_fog_underwater_color_a;

#include <ctype.h>
#include "../../../../h2common/part_uvs.h"

#define	PFL_FLAG_MASK	0x0000007f	// Mask out any flags

typedef struct {
	vec3_t start;
	vec3_t end;
} debugLine_t;

debugLine_t debug_lines[256];
int numDebugLines = 0;

void R_Clear (void);
entity_t	*currententity;
model_t		*currentmodel;

void
R_RotateForEntity(entity_t *e)
{
	glTranslatef(e->origin[0], e->origin[1], e->origin[2]);

// jmarshall - Heretic 2 rotation fix from their ref_gl.dll
	glRotatef(e->angles[1] * 57.295776, 0, 0, 1);
	glRotatef(-e->angles[0] * 57.295776, 0, 1, 0);
	glRotatef(-e->angles[2] * 57.295776, 1, 0, 0);
// jmarshall end
}

void
R_DrawSpriteModel(entity_t *currententity, const model_t *currentmodel)
{
	float alpha = 1.0F;
	vec3_t point;
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;
	image_t *skin;

	/* don't even bother culling, because it's just
	   a single polygon without a surface cache */
	psprite = (dsprite_t *)currentmodel->extradata;

	currententity->frame %= psprite->numframes;
	frame = &psprite->frames[currententity->frame];

	/* normal sprite */
	up = vup;
	right = vright;



	glColor4f(1, 1, 1, alpha);

	skin = currentmodel->skins[currententity->frame];
	if (!skin)
	{
		skin = r_notexture; /* fallback... */
	}

	R_Bind(skin->texnum);

	glShadeModel(7425);
	R_TexEnv(GL_MODULATE);

	if (alpha == 1.0)
	{
		glEnable(GL_ALPHA_TEST);
	}
	else
	{
		glDisable(GL_ALPHA_TEST);
	}

	if(currententity->flags & RF_NODEPTHTEST)
		glDisable(GL_DEPTH_TEST);

	if (currententity->flags & RF_TRANS_ADD)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}

	glColor3f(currententity->color.r / 255.0f, currententity->color.g / 255.0f, currententity->color.b / 255.0f);

	switch (currententity->spriteType)
	{
		case 0xFFFFFFFF:
		case 0:
			glBegin(GL_QUADS);

			glTexCoord2f(0, 1);
			VectorMA(currententity->origin, -frame->origin_y, up, point);
			VectorMA(point, -frame->origin_x, right, point);
			glVertex3fv(point);

			glTexCoord2f(0, 0);
			VectorMA(currententity->origin, frame->height - frame->origin_y, up, point);
			VectorMA(point, -frame->origin_x, right, point);
			glVertex3fv(point);

			glTexCoord2f(1, 0);
			VectorMA(currententity->origin, frame->height - frame->origin_y, up, point);
			VectorMA(point, frame->width - frame->origin_x, right, point);
			glVertex3fv(point);

			glTexCoord2f(1, 1);
			VectorMA(currententity->origin, -frame->origin_y, up, point);
			VectorMA(point, frame->width - frame->origin_x, right, point);
			glVertex3fv(point);

			glEnd();
			break;
	}
	if (currententity->flags & RF_NODEPTHTEST)
		glEnable(GL_DEPTH_TEST);

	glDisable(GL_ALPHA_TEST);
	R_TexEnv(GL_REPLACE);

	glDisable(GL_BLEND);
	glShadeModel(7424);

	glColor4f(1, 1, 1, 1);
}

void
R_DrawNullModel(void)
{
	vec3_t shadelight;
	int		i;

	if (currententity->flags & RF_FULLBRIGHT)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	}
	else
	{
		R_LightPoint(currententity, currententity->origin, shadelight);
	}

	glPushMatrix();
	R_RotateForEntity(currententity);

	glDisable(GL_TEXTURE_2D);
	glColor3fv(shadelight);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glColor4f(1, 1, 1, 1);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

void
R_DrawEntitiesOnList(void)
{
	int i;

	if (!r_drawentities->value)
	{
		return;
	}

	/* draw non-transparent first */
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = r_newrefdef.entities[i];

		if (currententity->flags & RF_TRANSLUCENT)
		{
			continue; /* solid */
		}

		{
			currentmodel = currententity->model[0];
			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_flex:
					R_DrawFlexModel(currententity, currentmodel);
					break;
				case mod_alias:
					R_DrawAliasModel(currententity, currentmodel);
					break;
				case mod_brush:
					R_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					R_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					ri.Sys_Error(ERR_DROP, "Bad modeltype");
					break;
			}
		}
	}

	/* draw transparent entities
	   we could sort these if it ever
	   becomes a problem... */
	glDepthMask(0);

	for (i = 0; i < r_newrefdef.num_alpha_entities; i++)
	{
		currententity = r_newrefdef.alpha_entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid
		{
			currentmodel = currententity->model[0];

			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_flex:
					R_DrawFlexModel(currententity, currentmodel);
					break;
				case mod_alias:
					R_DrawAliasModel(currententity, currentmodel);
					break;
				case mod_brush:
					R_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					R_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					ri.Sys_Error(ERR_DROP, "Bad modeltype");
					break;
			}
		}
	}

	glDepthMask(1); /* back to writing */
}


/*
==============
RB_RenderQuad
==============
*/
void RB_RenderQuad(const vec3_t origin, vec3_t left, vec3_t up, byte* color, float s1, float t1, float s2, float t2) {
	vec3_t vertexes[4];
	vec3_t st[4];
	int indexes[6] = { 0, 1, 3, 3, 1, 2 };

	VectorSet(vertexes[0], origin[0] + left[0] + up[0], origin[1] + left[1] + up[1], origin[2] + left[2] + up[2]);
	VectorSet(vertexes[1], origin[0] - left[0] + up[0], origin[1] - left[1] + up[1], origin[2] - left[2] + up[2]);
	VectorSet(vertexes[2], origin[0] - left[0] - up[0], origin[1] - left[1] - up[1], origin[2] - left[2] - up[2]);
	VectorSet(vertexes[3], origin[0] + left[0] - up[0], origin[1] + left[1] - up[1], origin[2] + left[2] - up[2]);

	st[0][0] = s1;
	st[0][1] = t1;

	st[1][0] = s2;
	st[1][1] = t1;

	st[2][0] = s2;
	st[2][1] = t2;

	st[3][0] = s1;
	st[3][1] = t2;

	glColor4ubv(color);

	for (int i = 0; i < 6; i++)
	{
		glTexCoord2f(st[indexes[i]][0], st[indexes[i]][1]);
		glVertex3fv(vertexes[indexes[i]]);
	}
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles(int num_particles, particle_t* particles, int type)
{
	const particle_t* p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

	glEnable(GL_TEXTURE_2D);

	if (type)
	{
		R_Bind(atlas_aparticle->texnum);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else
	{
		R_Bind(atlas_particle->texnum);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDepthMask(GL_FALSE);		// no z buffering
	glEnable(GL_BLEND);
	R_TexEnv(GL_MODULATE);
	glBegin(GL_TRIANGLES);

	for (p = particles, i = 0; i < num_particles; i++, p++)
	{
		VectorScale(vup, p->scale, up);
		VectorScale(vright, -p->scale, right);

		color[0] = p->color.r;
		color[1] = p->color.g;
		color[2] = p->color.b;
		color[3] = p->color.a;

		tex_coords_t* texCoord = &part_TexCoords[p->type & PFL_FLAG_MASK];

		RB_RenderQuad(p->origin, right, up, color, texCoord->lx, texCoord->ty, texCoord->rx, texCoord->by);
	}

	glEnd();
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	glDepthMask(1);		// back to normal Z buffering
	R_TexEnv(GL_REPLACE);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);

    glLoadIdentity ();

	// FIXME: get rid of these
    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);

	glColor4f(1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

void
R_SetupFrame(void)
{
	int i;
	mleaf_t *leaf;

	r_framecount++;

	/* build the transformation matrix for the given view angles */
	VectorCopy(r_newrefdef.vieworg, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	/* current viewcluster */
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		if (!r_worldmodel)
		{
			ri.Sys_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel->nodes);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
	}

	for (i = 0; i < 4; i++)
	{
		v_blend[i] = r_newrefdef.blend[i];
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

	/* clear out the portion of the screen that the NOWORLDMODEL defines */
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.3, 0.3, 0.3, 1);
		glScissor(r_newrefdef.x,
				vid.height - r_newrefdef.height - r_newrefdef.y,
				r_newrefdef.width, r_newrefdef.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(1, 0, 0.5, 0.5);
		glDisable(GL_SCISSOR_TEST);
	}
}

void
R_MYgluPerspective(GLdouble fovy, GLdouble aspect,
		GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;
	xmax += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void
R_SetupGL(void)
{
	float screenaspect;
	int x, x2, y2, y, w, h;

	/* set up viewport */
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height -
			  (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	qboolean drawing_left_eye = gl_state.camera_separation < 0;
	qboolean stereo_split_tb = ((gl_state.stereo_mode == STEREO_SPLIT_VERTICAL) && gl_state.camera_separation);
	qboolean stereo_split_lr = ((gl_state.stereo_mode == STEREO_SPLIT_HORIZONTAL) && gl_state.camera_separation);

	if(stereo_split_lr) {
		w = w / 2;
		x = drawing_left_eye ? (x / 2) : (x + vid.width) / 2;
	}

	if(stereo_split_tb) {
		h = h / 2;
		y2 = drawing_left_eye ? (y2 + vid.height) / 2 : (y2 / 2);
	}

	glViewport(x, y2, w, h);

	/* set up projection matrix */
	screenaspect = (float)r_newrefdef.width / r_newrefdef.height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (r_farsee->value == 0)
	{
		R_MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, 4096);
	}
	else
	{
		R_MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, 8192);
	}

	glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(-90, 1, 0, 0); /* put Z going up */
	glRotatef(90, 0, 0, 1); /* put Z going up */
	glRotatef(-r_newrefdef.viewangles[2], 1, 0, 0);
	glRotatef(-r_newrefdef.viewangles[0], 0, 1, 0);
	glRotatef(-r_newrefdef.viewangles[1], 0, 0, 1);
	glTranslatef(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1],
			-r_newrefdef.vieworg[2]);

	glGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);

	/* set drawing parms */
	if (r_cull->value)
	{
		glEnable(GL_CULL_FACE);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

void
GL_WaterFog()
{
	float color[4];

	color[1] = r_fog_underwater_color_g->value;
	color[2] = r_fog_underwater_color_b->value;
	color[3] = r_fog_underwater_color_a->value;
	color[0] = r_fog_underwater_color_r->value;
	glFogi(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, r_fog_underwater_density->value);

	glFogfv(GL_FOG_COLOR, &color[0]);
	glEnable(GL_FOG);
	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(16640);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void
R_Clear(void)
{
	if (r_newrefdef.rdflags & RDF_UNDERWATER)
	{
		GL_WaterFog();
	}
	else
	{
		glDisable(GL_FOG);
	}

	// Check whether the stencil buffer needs clearing, and do so if need be.
	GLbitfield stencilFlags = 0;
	if (gl_state.stereo_mode >= STEREO_MODE_ROW_INTERLEAVED && gl_state.stereo_mode <= STEREO_MODE_PIXEL_INTERLEAVED) {
		glClearStencil(0);
		stencilFlags |= GL_STENCIL_BUFFER_BIT;
	}

	if (gl1_ztrick->value)
	{
		static int trickframe;

		if (r_clear->value)
		{
			glClear(GL_COLOR_BUFFER_BIT | stencilFlags);
		}

		trickframe++;

		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc(GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc(GL_GEQUAL);
		}
	}
	else
	{
		if (r_clear->value)
		{
			glClear(GL_COLOR_BUFFER_BIT | stencilFlags | GL_DEPTH_BUFFER_BIT);
		}
		else
		{
			glClear(GL_DEPTH_BUFFER_BIT | stencilFlags);
		}

		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc(GL_LEQUAL);
	}

	glDepthRange(gldepthmin, gldepthmax);

	if (gl_zfix->value)
	{
		if (gldepthmax > gldepthmin)
		{
			glPolygonOffset(0.05, 1);
		}
		else
		{
			glPolygonOffset(-0.05, -1);
		}
	}

	/* stencilbuffer shadows */
	if (gl_shadows->value && gl_state.stencil && gl1_stencilshadow->value)
	{
		glClearStencil(1);
		glClear(GL_STENCIL_BUFFER_BIT);
	}
}

void
R_Flash(void)
{
	R_PolyBlend();
}

/*
 * r_newrefdef must be set before the first call
 */
static void
R_RenderView(refdef_t *fd)
{
	if (r_norefresh->value)
	{
		return;
	}

	r_newrefdef = *fd;

	if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		ri.Sys_Error(ERR_DROP, "%s: NULL worldmodel", __func__);
	}

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	if (gl_finish->value)
	{
		glFinish();
	}

	R_SetupFrame();

	R_SetFrustum(vup, vpn, vright, r_origin, r_newrefdef.fov_x, r_newrefdef.fov_y,
		frustum);

	R_SetupGL();

	R_MarkLeaves(); /* done here so we know if we're in water */

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles(r_newrefdef.num_particles, r_newrefdef.particles, 0);
	R_DrawParticles(r_newrefdef.anum_particles, r_newrefdef.aparticles, 1);

	R_DrawAlphaSurfaces();

	R_Flash();

	if (r_speeds->value)
	{
		R_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
				c_brush_polys, c_alias_polys, c_visible_textures,
				c_visible_lightmaps);
	}

	switch (gl_state.stereo_mode) {
		case STEREO_MODE_NONE:
			break;
		case STEREO_MODE_ANAGLYPH:
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			break;
		case STEREO_MODE_ROW_INTERLEAVED:
		case STEREO_MODE_COLUMN_INTERLEAVED:
		case STEREO_MODE_PIXEL_INTERLEAVED:
			glDisable(GL_STENCIL_TEST);
			break;
		default:
			break;
	}
}


void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	glViewport (0,0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);
}

static void GL_DrawColoredStereoLinePair( float r, float g, float b, float y )
{
	glColor3f( r, g, b );
	glVertex2f( 0, y );
	glVertex2f( vid.width, y );
	glColor3f( 0, 0, 0 );
	glVertex2f( 0, y + 1 );
	glVertex2f( vid.width, y + 1 );
}

static void GL_DrawStereoPattern( void )
{
	int i;

	if ( !gl_state.stereo_mode == STEREO_MODE_NONE )
		return;

	R_SetGL2D();

	glDrawBuffer( GL_BACK_LEFT );

	for ( i = 0; i < 20; i++ )
	{
		glBegin( GL_LINES );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
			GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
			GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
			GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
			GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
		glEnd();

		GLimp_EndFrame();
	}
}

static void
R_SetLightLevel(void)
{
	vec3_t shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint(currententity, r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[0];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
	else
	{
		if (shadelight[1] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[1];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
}

static void
RI_RenderFrame(refdef_t *fd)
{
	R_RenderView(fd);
	R_SetLightLevel ();
	glLineWidth(10.0);

	glDisable(GL_DEPTH_TEST);
	for (int i = 0; i < numDebugLines; i++)
	{
		glBegin(GL_LINES);
		glVertex3f(debug_lines[i].start[0], debug_lines[i].start[1], debug_lines[i].start[2]);
		glVertex3f(debug_lines[i].end[0], debug_lines[i].end[1], debug_lines[i].end[2]);
		glEnd();

	}
	glEnable(GL_DEPTH_TEST);

	numDebugLines = 0;

	R_SetGL2D();
}

void
R_Register(void)
{
	r_fog_underwater_density = ri.Cvar_Get(
		"r_fog_underwater_density",
		"0.0015",
		0);
	r_fog_underwater_color_r = ri.Cvar_Get(
		"r_fog_underwater_color_r",
		"0.1",
		0);
	r_fog_underwater_color_g = ri.Cvar_Get(
		"r_fog_underwater_color_g",
		"0.37",
		0);
	r_fog_underwater_color_b = ri.Cvar_Get(
		"r_fog_underwater_color_b",
		"0.6",
		0);
	r_fog_underwater_color_a = ri.Cvar_Get(
		"r_fog_underwater_color_a",
		"0.0",
		0);


	gl_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_gunfov = ri.Cvar_Get("r_gunfov", "80", CVAR_ARCHIVE);
	r_farsee = ri.Cvar_Get("r_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	gl1_overbrightbits = ri.Cvar_Get("gl1_overbrightbits", "0", CVAR_ARCHIVE);

	gl1_particle_min_size = ri.Cvar_Get("gl1_particle_min_size", "2", CVAR_ARCHIVE);
	gl1_particle_max_size = ri.Cvar_Get("gl1_particle_max_size", "40", CVAR_ARCHIVE);
	gl1_particle_size = ri.Cvar_Get("gl1_particle_size", "40", CVAR_ARCHIVE);
	gl1_particle_att_a = ri.Cvar_Get("gl1_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl1_particle_att_b = ri.Cvar_Get("gl1_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl1_particle_att_c = ri.Cvar_Get("gl1_particle_att_c", "0.01", CVAR_ARCHIVE);
	gl1_particle_square = ri.Cvar_Get("gl1_particle_square", "0", CVAR_ARCHIVE);

	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	r_mode = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE);
	gl_lightmap = ri.Cvar_Get("r_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	gl1_stencilshadow = ri.Cvar_Get("gl1_stencilshadow", "0", CVAR_ARCHIVE);
	gl1_dynamic = ri.Cvar_Get("gl1_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
	gl1_round_down = ri.Cvar_Get("gl1_round_down", "1", 0);
	gl1_picmip = ri.Cvar_Get("gl1_picmip", "0", 0);
	gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
	gl_showbbox = ri.Cvar_Get("gl_showbbox", "0", 0);
	gl1_ztrick = ri.Cvar_Get("gl1_ztrick", "0", 0);
	gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", 0);
	gl1_flashblend = ri.Cvar_Get("gl1_flashblend", "0", 0);
	r_fixsurfsky = ri.Cvar_Get("r_fixsurfsky", "0", CVAR_ARCHIVE);

	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl1_texturealphamode = ri.Cvar_Get("gl1_texturealphamode", "default", CVAR_ARCHIVE);
	gl1_texturesolidmode = ri.Cvar_Get("gl1_texturesolidmode", "default", CVAR_ARCHIVE);
	gl_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);

	gl1_palettedtexture = ri.Cvar_Get("r_palettedtextures", "0", CVAR_ARCHIVE);
	gl1_pointparameters = ri.Cvar_Get("gl1_pointparameters", "1", CVAR_ARCHIVE);

	gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	r_vsync = ri.Cvar_Get("r_vsync", "1", CVAR_ARCHIVE);

	gl1_saturatelighting = ri.Cvar_Get("gl1_saturatelighting", "0", 0);

	vid_ref = ri.Cvar_Get("vid_ref", "soft", CVAR_ARCHIVE );

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.2", CVAR_ARCHIVE);

	r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	gl_msaa_samples = ri.Cvar_Get ( "r_msaa_samples", "0", CVAR_ARCHIVE );

	r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);
	r_validation = ri.Cvar_Get("r_validation", "0", CVAR_ARCHIVE);
	r_scale8bittextures = ri.Cvar_Get("r_scale8bittextures", "0", CVAR_ARCHIVE);

	/* don't bilerp characters and crosshairs */
	gl_nolerp_list = ri.Cvar_Get("r_nolerp_list", "pics/conchars.pcx pics/ch1.pcx pics/ch2.pcx pics/ch3.pcx", CVAR_ARCHIVE);
	/* textures that should always be filtered, even if r_2D_unfiltered or an unfiltered gl mode is used */
	r_lerp_list = ri.Cvar_Get("r_lerp_list", "", CVAR_ARCHIVE);
	/* don't bilerp any 2D elements */
	r_2D_unfiltered = ri.Cvar_Get("r_2D_unfiltered", "0", CVAR_ARCHIVE);
	/* don't bilerp videos */
	r_videos_unfiltered = ri.Cvar_Get("r_videos_unfiltered", "0", CVAR_ARCHIVE);

	gl1_stereo = ri.Cvar_Get( "gl1_stereo", "0", CVAR_ARCHIVE );
	gl1_stereo_separation = ri.Cvar_Get( "gl1_stereo_separation", "-0.4", CVAR_ARCHIVE );
	gl1_stereo_anaglyph_colors = ri.Cvar_Get( "gl1_stereo_anaglyph_colors", "rc", CVAR_ARCHIVE );
	gl1_stereo_convergence = ri.Cvar_Get( "gl1_stereo_convergence", "1", CVAR_ARCHIVE );

	ri.Cmd_AddCommand("imagelist", R_ImageList_f);
	ri.Cmd_AddCommand("screenshot", R_ScreenShot);
	ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", R_Strings);
}

/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	int err;
	qboolean fullscreen;

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	r_mode->modified = false;

	if ( ( err = GLimp_SetMode( (int *)&vid.width, (int*)&vid.height, r_mode->value, fullscreen ) ) == rserr_ok )
	{
		gl_state.prev_mode = r_mode->value;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			ri.Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode((int*)&vid.width, (int*)&vid.height, r_mode->value, false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			ri.Cvar_SetValue( "r_mode", gl_state.prev_mode );
			r_mode->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode((int*)&vid.width, (int*)&vid.height, gl_state.prev_mode, false ) ) != rserr_ok )
		{
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}

	return true;
}

/*
===============
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf (PRINT_ALL, "ref_gl version: " REF_VERSION "\n");

	R_Register();

	// initialize our gl dynamic bindings
	if (!QGL_Init())
	{
		QGL_Shutdown();
		ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init() - could not load \"%s\"\n", "opengl32" );
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) )
	{
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () )
	{
		QGL_Shutdown();
	ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	ri.Vid_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char *)glGetString (GL_VENDOR);
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (const char*)glGetString (GL_RENDERER);
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (const char*)glGetString (GL_VERSION);
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = (const char*)glGetString (GL_EXTENSIONS);
	//ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	strcpy( renderer_buffer, gl_config.renderer_string );
	Q_strlwr( renderer_buffer );

	strcpy( vendor_buffer, gl_config.vendor_string );
	Q_strlwr( vendor_buffer );

	ri.Cvar_Set( "scr_drawall", "0" );

	R_SetDefaultState();

	R_InitImages ();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	err = glGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	Mod_FreeAll();

	R_ShutdownImages();

	/* shutdown OS specific OpenGL stuff like contexts, etc.  */
	GLimp_Shutdown();

	/* shutdown our QGL subsystem */
	QGL_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{
	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( r_mode->modified || vid_fullscreen->modified )
	{
		// FIXME: only restart if CDS is required
		cvar_t *ref;

		ref = ri.Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	/*
	** update 3Dfx gamma -- it is expected that a user will do a vid_restart
	** after tweaking this value
	*/
	if ( vid_gamma->modified )
	{
		vid_gamma->modified = false;
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	glViewport (0,0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glEnable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);

	if (gl1_particle_square->modified)
	{
		if (gl_config.pointparameters)
		{
			/* GL_POINT_SMOOTH is not implemented by some OpenGL
			   drivers, especially the crappy Mesa3D backends like
			   i915.so. That the points are squares and not circles
			   is not a problem by Quake II! */
			if (gl1_particle_square->value)
			{
				glDisable(GL_POINT_SMOOTH);
			}
			else
			{
				glEnable(GL_POINT_SMOOTH);
			}
		}
		else
		{
			// particles aren't drawn as GL_POINTS, but as textured triangles
			// => update particle texture to look square - or circle-ish
			R_InitParticleTexture();
		}

		gl1_particle_square->modified = false;
	}

	/* draw buffer stuff */
	if (gl_drawbuffer->modified)
	{
		gl_drawbuffer->modified = false;

		if ((gl_state.camera_separation == 0) || gl_state.stereo_mode != STEREO_MODE_OPENGL)
		{
			if (Q_stricmp(gl_drawbuffer->string, "GL_FRONT") == 0)
			{
				glDrawBuffer(GL_FRONT);
			}
			else
			{
				glDrawBuffer(GL_BACK);
			}
		}
	}

	/* texturemode stuff */
	if (gl_texturemode->modified || (gl_config.anisotropic && gl_anisotropic->modified)
	    || gl_nolerp_list->modified || r_lerp_list->modified
		|| r_2D_unfiltered->modified || r_videos_unfiltered->modified)
	{
		R_TextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
		gl_anisotropic->modified = false;
		gl_nolerp_list->modified = false;
		r_lerp_list->modified = false;
		r_2D_unfiltered->modified = false;
		r_videos_unfiltered->modified = false;
	}

	if (gl1_texturealphamode->modified)
	{
		R_TextureAlphaMode(gl1_texturealphamode->string);
		gl1_texturealphamode->modified = false;
	}

	if (gl1_texturesolidmode->modified)
	{
		R_TextureSolidMode(gl1_texturesolidmode->string);
		gl1_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	R_SetTexturePalette( r_rawpalette );

	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT);
	glClearColor (1,0, 0.5 , 0.5);
}

/*
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	glDisable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	glColor4f( r, g, b, e->color.a );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[i] );
		glVertex3fv( end_points[i] );
		glVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		glVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
}

//===================================================================


void	RI_BeginRegistration (char *map);
struct model_s	*RI_RegisterModel (char *name);
void RI_SetSky (char *name, float rotate, vec3_t axis);
void	RI_EndRegistration (void);

void	RI_RenderFrame (refdef_t *fd);

struct image_s	*RDraw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	RDraw_TileClear (int x, int y, int w, int h, char *name);
void	RDraw_Fill (int x, int y, int w, int h, byte r, byte g, byte b);
void	RDraw_FadeScreen (void);

void Draw_Name (vec3_t origin, char *Name, paletteRGBA_t color)
{

}

void DrawLine(vec3_t start, vec3_t end) {
	debug_lines[numDebugLines].start[0] = start[0];
	debug_lines[numDebugLines].start[1] = start[1];
	debug_lines[numDebugLines].start[2] = start[2];

	debug_lines[numDebugLines].end[0] = end[0];
	debug_lines[numDebugLines].end[1] = end[1];
	debug_lines[numDebugLines].end[2] = end[2];

	numDebugLines++;
}

void R_EndFrame(void)
{
	GLimp_EndFrame();
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.BeginRegistration = RI_BeginRegistration;
	re.RegisterModel = RI_RegisterModel;
	re.RegisterSkin = RI_RegisterSkin;
	re.RegisterPic = RDraw_FindPic;
	re.SetSky = RI_SetSky;
	re.EndRegistration = RI_EndRegistration;
	re.Draw_Name = Draw_Name;
	re.DrawLine = DrawLine;
	re.RenderFrame = RI_RenderFrame;
	re.DrawGetPicSize = RDraw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = RDraw_TileClear;
	re.DrawFill = RDraw_Fill;
	re.DrawFadeScreen = RDraw_FadeScreen;
	re.DrawBigFont = R_DrawBigFont;
	re.BF_Strlen = BF_Strlen;
	re.BookDrawPic = R_BookDrawPic;
	re.DrawInitCinematic = R_DrawInitCinematic;
	re.DrawCloseCinematic = R_DrawCloseCinematic;
	re.DrawCinematic = R_DrawCinematic;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = R_EndFrame;

	re.AppActivate = GLimp_AppActivate;

	Swap_Init ();

	return re;
}

void R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(level, msg, argptr);
	va_end(argptr);
}

// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ri.Sys_Error (ERR_FATAL, "%s", text);
}

void
Com_Printf(char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	vsprintf (text, msg, argptr);
	va_end(argptr);

	ri.Con_Printf (PRINT_ALL, "%s", text);
}
