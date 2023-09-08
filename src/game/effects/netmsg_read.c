//
// Heretic II
// Copyright 1998 Raven Software
//
#include "../../common/header/common.h"
#include "../common/angles.h"
#include "../common/vector.h"
#include "assert.h"
#include "client_effects.h"


void MSG_ReadDirMag(sizebuf_t *sb, vec3_t dir)
{
	int		b;

	// Read in index into vector table
	b = fxi.MSG_ReadByte(sb);
	if (b >= NUMVERTEXNORMALS)
	{
		assert(0);
		fxi.Com_Error (ERR_DROP, "MSF_ReadDirMag: out of range");
	}
	VectorCopy (bytedirs[b], dir);

	// Scale by magnitude
	b = fxi.MSG_ReadByte(sb);
	Vec3ScaleAssign(10.0 * b, dir);
}

void MSG_ReadShortYawPitch(sizebuf_t *sb, vec3_t dir)
{
	vec3_t	angles;

	if(sb->readcount+4 > sb->cursize)
	{
		assert(0);
	}

	angles[0] = fxi.MSG_ReadShort(sb) * (1.0/8);
	angles[1] = fxi.MSG_ReadShort(sb) * (1.0/8);
	angles[2] = 0;

	angles[YAW] = angles[YAW] * ANGLE_TO_RAD;
	angles[PITCH] = angles[PITCH] * ANGLE_TO_RAD;
	DirFromAngles(angles, dir);
}

void MSG_ReadYawPitch(sizebuf_t *sb, vec3_t dir)
{
	int		yb, pb;
	float	yaw, pitch;
	vec3_t	angles;

	yb = fxi.MSG_ReadByte(sb);
	pb = fxi.MSG_ReadByte(sb);

	// Convert to signed degrees
	yaw = (yb * (360.0 / 255.0)) - 180.0;
	pitch = (pb * (180.0 / 255.0)) - 90.0;

	// Convert to radians
	angles[YAW] = yaw * ANGLE_TO_RAD;
	angles[PITCH] = pitch * ANGLE_TO_RAD;
	DirFromAngles(angles, dir);
}

void MSG_ReadEffects(sizebuf_t *msg_read, EffectsBuffer_t *fxBuf)
{
	int len;

	fxBuf->numEffects += fxi.MSG_ReadByte(msg_read);

	assert(fxBuf->numEffects >= 0);

	if(fxBuf->numEffects < 0)
	{
		fxi.Com_Error(ERR_DROP, "MSG_ReadEffects: number of effects < 0");
		return;
	}

	if(fxBuf->numEffects == 0)
	{
		return;
	}

	if(fxBuf->numEffects & 0x80)
	{
		fxBuf->numEffects &= ~0x80;
		len = fxi.MSG_ReadShort(msg_read);
	}
	else
	{
		len = fxi.MSG_ReadByte(msg_read);
	}

	assert(len > 0);

	if(fxBuf->numEffects <= 0)
	{
		fxi.Com_Error (ERR_DROP, "MSG_ReadEffects: bufSize not > 0");
		return;
	}

	fxi.MSG_ReadData(msg_read, fxBuf->buf+fxBuf->bufSize, len);

	fxBuf->bufSize+=len;
}
