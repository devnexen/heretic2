#include "../qcommon/H2Common.h"
#include "../qcommon/ResourceManager.h"

#include "../qcommon/surfaceprops.h"

char *surfacePropNames[256] =
{
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood",
	"gravel",
	"metal",
	"stone",
	"wood"
}; // weak

H2COMMON_API char *GetClientGroundSurfaceMaterialName(playerinfo_t *playerinfo)
{
	csurface_t *groundSurface;
	char *result = NULL; 

	//result = *(char **)(a1 + 256);
	groundSurface = playerinfo->GroundSurface; // jmarshall - check this.
	if (groundSurface)
		result = surfacePropNames[groundSurface->value >> 24];
	return result;
}