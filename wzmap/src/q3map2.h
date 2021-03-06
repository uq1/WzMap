/* -------------------------------------------------------------------------------

Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----------------------------------------------------------------------------------

This code has been altered significantly from its original form, to support
several games based on the Quake III Arena engine, in the form of "Q3Map2."

------------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------------

WzMap feature defines...

------------------------------------------------------------------------------- */

#define __SKIPIFY__					// Use system/skip instead of system/caulk for removing surfaces.
#define __BASEJKA_LIGHTGRID__
#define __SUPPORT_VALVE220__

// Collision model generation... TODO: Possibly add .climate ini option per model to select best method manually per model???
#define __MODEL_SIMPLIFY__			// Primary method. Simplification/Decimation.... Fairly reliable... Can sometimes leave some small holes though.
#define __MODEL_CONVEX_HULL__		// Secondary method. Convex hulls. Not reliable enough. Fails on some models (stuff that isn't a basic shape or has empty walkable areas inside).
									// Generates smaller collision models. Falls back to __MODEL_SIMPLIFY__ method if it is defined as well...

/* marker */
#ifndef Q3MAP2_H
#define Q3MAP2_H

//#define MREDEBUG // UQ1: For vis debugging info...
//#define __HIGH_MEMORY__ // UQ1: Only use if you have more than 16 GB (probably want even more than this for lighting) of ram!!!

/* -------------------------------------------------------------------------------

dependencies

------------------------------------------------------------------------------- */

/* platform-specific */
#if defined( __linux__ ) || defined( __APPLE__ )
	#define Q_UNIX
#endif

#ifdef Q_UNIX
	#include <unistd.h>
	#include <pwd.h>
	#include <limits.h>
#endif

#if defined(WIN32) || defined(WIN64)
	#include <windows.h>
#endif


/* general */
#include "version.h"			/* ttimo: might want to guard that if built outside of the GtkRadiant tree */

#include "cmdlib.h"
#include "mathlib.h"
#include "ddslib.h"

#include "picomodel.h"

#include "scriplib.h"
#include "polylib.h"
#include "imagelib.h"
#include "threads.h"
#include "inout.h"
#include "vfs.h"
#include "png.h"
#include "jpeglib.h"
#include "md4.h"

#include "matrix.h"

#include <stdlib.h>

#ifdef MAIN_C
	#define Q_EXTERN
	#define Q_ASSIGN( a )	= a
	#define Q_ASSIGN_VEC3( a, b, c ) = { a, b, c }
#else
	#define Q_EXTERN extern
	#define Q_ASSIGN( a )
	#define Q_ASSIGN_VEC3( a, b, c )
#endif

#if (_MSC_VER < 1300)
   typedef signed char       int8_t;
   typedef signed short      int16_t;
   typedef signed int        int32_t;
   typedef unsigned char     uint8_t;
   typedef unsigned short    uint16_t;
   typedef unsigned int      uint32_t;
#else
   typedef signed __int8     int8_t;
   typedef signed __int16    int16_t;
   typedef signed __int32    int32_t;
   typedef unsigned __int8   uint8_t;
   typedef unsigned __int16  uint16_t;
   typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;

/* -------------------------------------------------------------------------------

port-related hacks

------------------------------------------------------------------------------- */

#define MAC_STATIC_HACK			0
#if defined( __APPLE__ ) && MAC_STATIC_HACK
	#define MAC_STATIC			static
#else
	#define MAC_STATIC
#endif

#if defined(WIN32) || defined(WIN64)
	#define Q_stricmp			_stricmp
	#define Q_strncasecmp		_strnicmp
#else
	#define Q_stricmp			strcasecmp
	#define Q_strncasecmp		strncasecmp
#endif

/* macro version */
#define VectorMA( a, s, b, c )	((c)[ 0 ] = (a)[ 0 ] + (s) * (b)[ 0 ], (c)[ 1 ] = (a)[ 1 ] + (s) * (b)[ 1 ], (c)[ 2 ] = (a)[ 2 ] + (s) * (b)[ 2 ])


#define PATH_MAX 260
extern char     g_strDirs[VFS_MAXDIRS][PATH_MAX];


/* -------------------------------------------------------------------------------

constants

------------------------------------------------------------------------------- */

/* general */
#define MAX_QPATH				64

#define MAX_IMAGES				512
#define DEFAULT_IMAGE			"*default"

#define MAX_MODELS				512

#define	DEF_BACKSPLASH_FRACTION	0.05f	/* 5% backsplash by default */
#define	DEF_BACKSPLASH_DISTANCE	23

#define DEF_RADIOSITY_BOUNCE	1.0f	/* ydnar: default to 100% re-emitted light */

#define	MAX_SHADER_INFO			8192
#define MAX_CUST_SURFACEPARMS	1024	/* vortex: increased from 64 */

#define	SHADER_MAX_VERTEXES		1000
#define	SHADER_MAX_INDEXES		(6 * SHADER_MAX_VERTEXES)

#define MAX_JITTERS				256

/* epair parsing (note case-sensitivity directive) */
#define CASE_INSENSITIVE_EPAIRS	1

#if CASE_INSENSITIVE_EPAIRS
	#define EPAIR_STRCMP		Q_stricmp
#else
	#define EPAIR_STRCMP		strcmp
#endif


#define MATERIAL_BITS			5
#define MATERIAL_MASK			0x1f	// mask to get the material type

#define MATERIAL_NONE							0			// for when the artist hasn't set anything up =)
#define MATERIAL_SOLIDWOOD						1			// freshly cut timber
#define MATERIAL_HOLLOWWOOD						2			// termite infested creaky wood
#define MATERIAL_SOLIDMETAL						3			// solid girders
#define MATERIAL_HOLLOWMETAL					4			// hollow metal machines
#define MATERIAL_SHORTGRASS						5			// manicured lawn
#define MATERIAL_LONGGRASS						6			// long jungle grass
#define MATERIAL_DIRT							7			// hard mud
#define MATERIAL_SAND							8			// sandy beach
#define MATERIAL_GRAVEL							9			// lots of small stones
#define MATERIAL_GLASS							10			//
#define MATERIAL_CONCRETE						11			// hardened concrete pavement
#define MATERIAL_MARBLE							12			// marble floors
#define MATERIAL_WATER							13			// light covering of water on a surface
#define MATERIAL_SNOW							14			// freshly laid snow
#define MATERIAL_ICE							15			// packed snow/solid ice
#define MATERIAL_FLESH							16			// hung meat, corpses in the world
#define MATERIAL_MUD							17			// wet soil
#define MATERIAL_BPGLASS						18			// bulletproof glass
#define MATERIAL_DRYLEAVES						19			// dried up leaves on the floor
#define MATERIAL_GREENLEAVES					20			// fresh leaves still on a tree
#define MATERIAL_FABRIC							21			// Cotton sheets
#define MATERIAL_CANVAS							22			// tent material
#define MATERIAL_ROCK							23			//
#define MATERIAL_RUBBER							24			// hard tire like rubber
#define MATERIAL_PLASTIC						25			//
#define MATERIAL_TILES							26			// tiled floor
#define MATERIAL_CARPET							27			// lush carpet
#define MATERIAL_PLASTER						28			// drywall style plaster
#define MATERIAL_SHATTERGLASS					29			// glass with the Crisis Zone style shattering
#define MATERIAL_ARMOR							30			// body armor
#define MATERIAL_COMPUTER						31			// computers/electronic equipment
#define MATERIAL_PUDDLE							32			// shallow puddle on floor surface, or water that should not use GLSL.
#define MATERIAL_POLISHEDWOOD					33			// shiny polished wood
#define MATERIAL_LAVA							34			// lava
#define MATERIAL_TREEBARK						35			// tree bark
#define MATERIAL_STONE							36			// stone (rock but no vines)
#define MATERIAL_EFX							37			// any efx surfaces
#define MATERIAL_BLASTERBOLT					38			// blaster bolts, sabers, etc
#define MATERIAL_FIRE							39			// fire
#define MATERIAL_SMOKE							40			// smoke
#define MATERIAL_FIREFLIES						41			// fire flies fx
#define MATERIAL_MAGIC_PARTICLES_TREE			42			// magic tree particle fx
#define MATERIAL_MAGIC_PARTICLES				43			// magic particles (small particle fx)
#define MATERIAL_PORTAL							44			// area transition portal
#define MATERIAL_MENU_BACKGROUND				45			// main menu background glsl
#define MATERIAL_SKYSCRAPER						46			// warzone sky scrapers
#define MATERIAL_DISTORTEDGLASS					47			// warzone distorted glass
#define MATERIAL_DISTORTEDPUSH					48			// warzone distorted push
#define MATERIAL_DISTORTEDPULL					49			// warzone distorted pull
#define MATERIAL_CLOAK							50			// warzone cloak
#define MATERIAL_PROCEDURALFOLIAGE				51			// warzone GLSL foliages
#define MATERIAL_LAST							52			// number of materials

// Defined as a macro here so one change will affect all the relevant files

#define MATERIALS						\
	"none",								\
	"solidwood",						\
	"hollowwood",						\
	"solidmetal",						\
	"hollowmetal",						\
	"shortgrass",						\
	"longgrass",						\
	"dirt",	   							\
	"sand",	   							\
	"gravel",							\
	"glass",							\
	"concrete",							\
	"marble",							\
	"water",							\
	"snow",	   							\
	"ice",								\
	"flesh",							\
	"mud",								\
	"bpglass",							\
	"dryleaves",						\
	"greenleaves",						\
	"fabric",							\
	"canvas",							\
	"rock",								\
	"rubber",							\
	"plastic",							\
	"tiles",							\
	"carpet",							\
	"plaster",							\
	"shatterglass",						\
	"armor",							\
	"computer",							\
	"puddle",							\
	"polishedwood",						\
	"lava",								\
	"treebark",							\
	"stone",							\
	"efx",								\
	"blasterbolt",						\
	"fire",								\
	"smoke",							\
	"fireflies",						\
	"magicparticlestree",				\
	"magicparticles",					\
	"portal",							\
	"menubackground",					\
	"skyscraper",						\
	"distortedglass",					\
	"distortedpush",					\
	"distortedpull",					\
	"cloak",							\
	"proceduralfoliage"


/* ydnar: compiler flags, because games have widely varying content/surface flags */
#define C_SOLID					0x00000001
#define C_TRANSLUCENT			0x00000002
#define C_STRUCTURAL			0x00000004
#define C_HINT					0x00000008
#define C_NODRAW				0x00000010
#define C_LIGHTGRID				0x00000020
#define C_ALPHASHADOW			0x00000040
#define C_LIGHTFILTER			0x00000080
#define C_VERTEXLIT				0x00000100
#define C_LIQUID				0x00000200
#define C_FOG					0x00000400
#define C_SKY					0x00000800
#define C_ORIGIN				0x00001000
#define C_AREAPORTAL			0x00002000
#define C_ANTIPORTAL			0x00004000	/* like hint, but doesn't generate portals */
#define C_SKIP					0x00008000	/* like hint, but skips this face (doesn't split bsp) */
#define C_NOMARKS				0x00010000	/* no decals */
#define C_REGION				0x00020000  /* define map region to be compiled (everything outside region get excluded) */
#define C_DETAIL				0x08000000	/* THIS MUST BE THE SAME AS IN RADIANT! */


/* shadow flags */
#define WORLDSPAWN_CAST_SHADOWS	1
#define WORLDSPAWN_RECV_SHADOWS	1
#define ENTITY_CAST_SHADOWS		0
#define ENTITY_RECV_SHADOWS		1


/* bsp */
#define	MAX_PATCH_SIZE			32
#define	MAX_BRUSH_SIDES			1024
#define MAX_BUILD_SIDES			300

#define	MAX_EXPANDED_AXIS		128

#define	CLIP_EPSILON			0.1f
#define	PLANESIDE_EPSILON		0.001f
#define	PLANENUM_LEAF			-1

#define	HINT_PRIORITY			1000		/* ydnar: force hint splits first and antiportal/areaportal splits last */
#define ANTIPORTAL_PRIORITY		-1000
#define AREAPORTAL_PRIORITY		-1000

#define	PSIDE_FRONT				1
#define	PSIDE_BACK				2
#define	PSIDE_BOTH				(PSIDE_FRONT | PSIDE_BACK)
#define	PSIDE_FACING			4

#define BPRIMIT_UNDEFINED		0
#define BPRIMIT_QUAKE			BPRIMIT_UNDEFINED
#define BPRIMIT_OLDBRUSHES		1
#define BPRIMIT_NEWBRUSHES		2
#define BPRIMIT_VALVE220		3


/* vis */
#define	VIS_HEADER_SIZE			8

#define SEPERATORCACHE			/* seperator caching helps a bit */

#define	PORTALFILE				"PRT1"

#define	MAX_PORTALS				65536//131072//262144//32768
#define MAX_SEPERATORS			MAX_POINTS_ON_WINDING
#define	MAX_POINTS_ON_FIXED_WINDING	24	/* ydnar: increased this from 12 at the expense of more memory */
#define	MAX_PORTALS_ON_LEAF		128//256//32768//4096//128


/* light */
#define EMIT_POINT				0
#define EMIT_AREA				1
#define EMIT_SPOT				2
#define EMIT_SUN				3

#define LIGHT_ATTEN_LINEAR		1
#define LIGHT_ATTEN_ANGLE		2
#define LIGHT_ATTEN_DISTANCE	4
#define LIGHT_TWOSIDED			8
#define LIGHT_GRID				16
#define LIGHT_SURFACES			32
#define LIGHT_DARK				64		/* probably never use this */
#define LIGHT_FAST				256
#define LIGHT_FAST_TEMP			512
#define LIGHT_FAST_ACTUAL		(LIGHT_FAST | LIGHT_FAST_TEMP)
#define LIGHT_NEGATIVE			1024
#define LIGHT_UNNORMALIZED		2048	/* vortex: do not normalize _color */

#define LIGHT_SUN_DEFAULT		(LIGHT_ATTEN_ANGLE | LIGHT_GRID | LIGHT_SURFACES)
#define LIGHT_AREA_DEFAULT		(LIGHT_ATTEN_ANGLE | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES)	/* q3a and wolf are the same */
#define LIGHT_Q3A_DEFAULT		(LIGHT_ATTEN_ANGLE | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES | LIGHT_FAST)
#define LIGHT_WOLF_DEFAULT		(LIGHT_ATTEN_LINEAR | LIGHT_ATTEN_DISTANCE | LIGHT_GRID | LIGHT_SURFACES | LIGHT_FAST)

#define MAX_TRACE_TEST_NODES	4096

#define LUXEL_EPSILON			0.0f
#define VERTEX_EPSILON			0.0f

#define MIN_LIGHTMAP_SAMPLE_SIZE	    0.1f
#define DEFAULT_LIGHTMAP_SAMPLE_SIZE	16.0f
#define DEFAULT_SUBDIVIDE_THRESHOLD		1.0f

#define	EXTRA_SCALE				2	/* -extrawide = -super 2 */
#define	EXTRAWIDE_SCALE			2	/* -extrawide = -super 2 -filter */

#define CLUSTER_UNMAPPED		-1
#define CLUSTER_OCCLUDED		-2
#define CLUSTER_FLOODED			-3

#define VERTEX_LUXEL_SIZE		3
#define BSP_LUXEL_SIZE			3
#define RAD_LUXEL_SIZE			3
#define SUPER_LUXEL_SIZE		5
#define SUPER_ORIGIN_SIZE		3
#define SUPER_NORMAL_SIZE		4
#define SUPER_DELUXEL_SIZE		4
#define BSP_DELUXEL_SIZE		3
#define BSP_NORMAL_SIZE         3
#define SUPER_FLOODLIGHT_SIZE	4
#define SUPER_TRIORIGIN_SIZE	3
#define SUPER_TRINORMAL_SIZE	3

#define VERTEX_LUXEL( s, v )    (vertexLuxels[ s ] + ((v) * VERTEX_LUXEL_SIZE))
#define RAD_VERTEX_LUXEL( s, v )(radVertexLuxels[ s ] + ((v) * VERTEX_LUXEL_SIZE))
#define BSP_LUXEL( s, x, y )    (lm->bspLuxels[ s ] + ((((y) * lm->w) + (x)) * BSP_LUXEL_SIZE))
#define RAD_LUXEL( s, x, y )    (lm->radLuxels[ s ] + ((((y) * lm->w) + (x)) * RAD_LUXEL_SIZE))
#define BSP_DELUXEL( x, y )     (lm->bspDeluxels + ((((y) * lm->w) + (x)) * BSP_DELUXEL_SIZE))
#define BSP_NORMAL( x, y )      (lm->bspNormals + ((((y) * lm->w) + (x)) * BSP_NORMAL_SIZE))
#define SUPER_LUXEL( s, x, y )  (lm->superLuxels[ s ] + ((((y) * lm->sw) + (x)) * SUPER_LUXEL_SIZE))
#define SUPER_DELUXEL( x, y )   (lm->superDeluxels + ((((y) * lm->sw) + (x)) * SUPER_DELUXEL_SIZE))
#define SUPER_CLUSTER( x, y )   (lm->superClusters + (((y) * lm->sw) + (x)))
#define SUPER_ORIGIN( x, y )    (lm->superOrigins + ((((y) * lm->sw) + (x)) * SUPER_ORIGIN_SIZE))
#define SUPER_NORMAL( x, y )    (lm->superNormals + ((((y) * lm->sw) + (x)) * SUPER_NORMAL_SIZE))
#define SUPER_DIRT( x, y )      (lm->superNormals + ((((y) * lm->sw) + (x)) * SUPER_NORMAL_SIZE) + 3)	/* stash dirtyness in normal[ 3 ] */
#define SUPER_FLOODLIGHT( x, y )(lm->superFloodLight + ((((y) * lm->sw) + (x)) * SUPER_FLOODLIGHT_SIZE))
#define SUPER_TRIORIGIN( x, y ) (lm->superTriorigins + ((((y) * lm->sw) + (x)) * SUPER_TRIORIGIN_SIZE))
#define SUPER_TRINORMAL( x, y ) (lm->superTrinormals + ((((y) * lm->sw) + (x)) * SUPER_TRINORMAL_SIZE))

/* vortex: linear luxel access */
#define LUXEL_XY( x, y )        (((y) * lm->sw) + (x))
#define SUPER_LUXEL_XY( s, i )  (lm->superLuxels[ s ] + (i * SUPER_LUXEL_SIZE))
#define SUPER_DELUXEL_XY( i )   (lm->superDeluxels + (i * SUPER_DELUXEL_SIZE))
#define SUPER_CLUSTER_XY( i )   (lm->superClusters + i)
#define SUPER_ORIGIN_XY( i )    (lm->superOrigins + (i * SUPER_ORIGIN_SIZE))
#define SUPER_NORMAL_XY( i )    (lm->superNormals + (i * SUPER_NORMAL_SIZE))
#define SUPER_TRIORIGIN_XY( i ) (lm->superTriorigins + (i * SUPER_TRIORIGIN_SIZE))
#define SUPER_TRINORMAL_XY( i ) (lm->superTrinormals + (i * SUPER_TRINORMAL_SIZE))

/* -------------------------------------------------------------------------------

abstracted bsp file

------------------------------------------------------------------------------- */

#define EXTERNAL_LIGHTMAP		"lm_%04d"

#define MAX_LIGHTMAPS			4//1			/* RBSP */ /* VorteX: disable lightstyles */
#define MAX_LIGHT_STYLES		64
#define	MAX_SWITCHED_LIGHTS		32
#define LS_NORMAL				0x00
#define LS_UNUSED				0xFE
#define	LS_NONE					0xFF

#define MAX_LIGHTMAP_SHADERS	256//8192 /* vortex: was 256 */

#if 0
/* ok to increase these at the expense of more memory */
#define	MAX_MAP_MODELS			0x1000		// vortex: was 0x400
#define	MAX_MAP_BRUSHES			0x400000
#define	MAX_MAP_ENTITIES		0x80000		//%	0x800	/* ydnar */ // vortex: was 0x8000
#define	MAX_MAP_ENTSTRING		0x80000	    //%	0x40000	/* ydnar */ // vortex: was 0x80000
#define	MAX_MAP_SHADERS			0x800		//%	0x400	/* ydnar */

#define	MAX_MAP_AREAS			0x100		/* MAX_MAP_AREA_BYTES in q_shared must match! */
#define	MAX_MAP_FOGS			30			//& 0x100	/* RBSP (32 - world fog - goggles) */
#define	MAX_MAP_PLANES			0x7000000//0x200000	//%	0x20000	/* ydnar for md */ // vortex: doubled this
#define	MAX_MAP_NODES			0xA00000//0x40000
#define	MAX_MAP_BRUSHSIDES		0xF00000//0x100000	//%	0x20000	/* ydnar */
#define	MAX_MAP_LEAFS			0xA00000//0x20000
#define	MAX_MAP_LEAFFACES		0xA00000//0x100000	//%	0x20000	/* ydnar */
#define	MAX_MAP_LEAFBRUSHES		0xA00000//0x40000
#define	MAX_MAP_PORTALS			0xA00000//0x20000
#define	MAX_MAP_LIGHTGRID		0xE000000	//%	0x800000 /* ydnar: set to points, not bytes */
#define	MAX_MAP_VISIBILITY		0xFFFFFFF//0x200000

#define	MAX_MAP_DRAW_SURFS		0x400000//0x40000 // vortex: was 0x20000
#define	MAX_MAP_DRAW_VERTS		0x8000000//0x800000 // vortex: was 0x80000
#define	MAX_MAP_DRAW_INDEXES	0x8000000//0x800000  // vortex: was 0x80000
#elif defined(__HIGH_MEMORY__)
/* ok to increase these at the expense of more memory */
#define	MAX_MAP_MODELS			0x1000
#define	MAX_MAP_BRUSHES			0x400000
#define	MAX_MAP_ENTITIES		0x80000
#define	MAX_MAP_ENTSTRING		0x80000
#define	MAX_MAP_SHADERS			0x800

#define	MAX_MAP_AREAS			0x100		/* MAX_MAP_AREA_BYTES in q_shared must match! */
#define	MAX_MAP_FOGS			30
#define	MAX_MAP_PLANES			0x7000000
#define	MAX_MAP_NODES			0xA00000
#define	MAX_MAP_BRUSHSIDES		0xF00000
#define	MAX_MAP_LEAFS			0xA00000
#define	MAX_MAP_LEAFFACES		0xA00000
#define	MAX_MAP_LEAFBRUSHES		0xA00000
#define	MAX_MAP_PORTALS			0xA00000
#define	MAX_MAP_LIGHTGRID		0xF00000
#define	MAX_MAP_VISIBILITY		0xFFFFFFF

#define	MAX_MAP_DRAW_SURFS		0x400000
#define	MAX_MAP_DRAW_VERTS		0x8000000
#define	MAX_MAP_DRAW_INDEXES	0x8000000
#else
/* ok to increase these at the expense of more memory */
#define	MAX_MAP_MODELS			0x1000
#define	MAX_MAP_BRUSHES			0x200000//0x100000//0x400000
#define	MAX_MAP_ENTITIES		0x80000
#define	MAX_MAP_ENTSTRING		0x4000000//0x800000//0x80000
#define	MAX_MAP_SHADERS			0x800

#define	MAX_MAP_AREAS			0x100		/* MAX_MAP_AREA_BYTES in q_shared must match! */
#define	MAX_MAP_FOGS			30
#define	MAX_MAP_PLANES			0x4000000//0x7000000
#define	MAX_MAP_NODES			0x800000//0xA00000
#define	MAX_MAP_BRUSHSIDES		0x2000000//0x1000000//0xF00000
#define	MAX_MAP_LEAFS			0x800000//0xA00000
#define	MAX_MAP_LEAFFACES		0x800000//0xA00000
#define	MAX_MAP_LEAFBRUSHES		0x800000//0xA00000
#define	MAX_MAP_PORTALS			0x800000//0xA00000
#ifndef __BASEJKA_LIGHTGRID__
#define	MAX_MAP_LIGHTGRID		0x800000//0xF00000
#else //__BASEJKA_LIGHTGRID__
#define	MAX_MAP_LIGHTGRID		65535 // matching JKA
#endif //__BASEJKA_LIGHTGRID__
#define	MAX_MAP_VISIBILITY		0x3FFFFFFF

#define	MAX_MAP_DRAW_SURFS		0x200000
#define	MAX_MAP_DRAW_VERTS		0x4000000
#define	MAX_MAP_DRAW_INDEXES	0x4000000
#endif

/* key / value pair sizes in the entities lump */
#define	MAX_KEY					32
#define	MAX_VALUE				1024

/* the editor uses these predefined yaw angles to orient entities up or down */
#define	ANGLE_UP				-1
#define	ANGLE_DOWN				-2

#define	LIGHTMAP_WIDTH			128
#define	LIGHTMAP_HEIGHT			128

#ifdef __BASEJKA_LIGHTGRID__
#define MIN_WORLD_COORD			(-65536)
#define	MAX_WORLD_COORD			(65536)
#define MIN_WORLD_LIGHT_COORD			(-65536)
#define	MAX_WORLD_LIGHT_COORD			(65536)
#define WORLD_SIZE				(MAX_WORLD_COORD - MIN_WORLD_COORD)
#else //!__BASEJKA_LIGHTGRID__
#define MIN_WORLD_COORD			(-524288)//(-65536)
#define	MAX_WORLD_COORD			(524288)//(65536)
#define MIN_WORLD_LIGHT_COORD			(-65536)
#define	MAX_WORLD_LIGHT_COORD			(65536)
#define WORLD_SIZE				(MAX_WORLD_COORD - MIN_WORLD_COORD)
#endif //__BASEJKA_LIGHTGRID__


typedef void					(*bspFunc)( const char * );


typedef struct
{
	uint32_t			offset, length;
}
bspLump_t;


typedef struct
{
	char		ident[ 4 ];
	int			version;

	bspLump_t	lumps[ 100 ];	/* theoretical maximum # of bsp lumps */
}
bspHeader_t;


typedef struct
{
	float		mins[ 3 ], maxs[ 3 ];
	int			firstBSPSurface, numBSPSurfaces;
	int			firstBSPBrush, numBSPBrushes;
}
bspModel_t;


typedef struct
{
	char		shader[ MAX_QPATH ];
	int			surfaceFlags;
	int			contentFlags;
}
bspShader_t;


/* planes x^1 is allways the opposite of plane x */

typedef struct
{
	float		normal[ 3 ];
	float		dist;
}
bspPlane_t;


typedef struct
{
	int			planeNum;
	int			children[ 2 ];		/* negative numbers are -(leafs+1), not nodes */
	int			mins[ 3 ];			/* for frustom culling */
	int			maxs[ 3 ];
}
bspNode_t;


typedef struct
{
	int			cluster;			/* -1 = opaque cluster (do I still store these?) */
	int			area;

	int			mins[ 3 ];			/* for frustum culling */
	int			maxs[ 3 ];

	int			firstBSPLeafSurface;
	int			numBSPLeafSurfaces;

	int			firstBSPLeafBrush;
	int			numBSPLeafBrushes;
}
bspLeaf_t;


typedef struct
{
	int			planeNum;			/* positive plane side faces out of the leaf */
	int			shaderNum;
	int			surfaceNum;			/* RBSP */
}
bspBrushSide_t;


typedef struct
{
	int			firstSide;
	int			numSides;
	int			shaderNum;			/* the shader that determines the content flags */
}
bspBrush_t;


typedef struct
{
	char		shader[ MAX_QPATH ];
	int			brushNum;
	int			visibleSide;		/* the brush side that ray tests need to clip against (-1 == none) */
}
bspFog_t;


typedef struct
{
	vec3_t		xyz;
	float		st[ 2 ];
	float		lightmap[ MAX_LIGHTMAPS ][ 2 ];	/* RBSP */
	vec3_t		normal;
	byte		color[ MAX_LIGHTMAPS ][ 4 ];	/* RBSP */
}
bspDrawVert_t;


typedef enum
{
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE
}
bspSurfaceType_t;


typedef struct bspGridPoint_s
{
	byte		ambient[ MAX_LIGHTMAPS ][ 3 ];
	byte		directed[ MAX_LIGHTMAPS ][ 3 ];
	byte		styles[ MAX_LIGHTMAPS ];
	byte		latLong[ 2 ];
}
bspGridPoint_t;


typedef struct
{
	int			shaderNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts;

	int			firstIndex;
	int			numIndexes;

	byte		lightmapStyles[ MAX_LIGHTMAPS ];						/* RBSP */
	byte		vertexStyles[ MAX_LIGHTMAPS ];							/* RBSP */
	int			lightmapNum[ MAX_LIGHTMAPS ];							/* RBSP */
	int			lightmapX[ MAX_LIGHTMAPS ], lightmapY[ MAX_LIGHTMAPS ];	/* RBSP */
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[ 3 ];	/* on patches, [ 0 ] and [ 1 ] are lodbounds */

	int			patchWidth;
	int			patchHeight;
}
bspDrawSurface_t;



/* -------------------------------------------------------------------------------

general types

------------------------------------------------------------------------------- */

/* ydnar: for q3map_tcMod */
typedef float	tcMod_t[ 3 ][ 3 ];


/* ydnar: for multiple game support */
typedef struct surfaceParm_s
{
	char		*name;
	int			contentFlags, contentFlagsClear;
	int			surfaceFlags, surfaceFlagsClear;
	int			compileFlags, compileFlagsClear;
}
surfaceParm_t;

typedef enum
{
	MINIMAP_MODE_GRAY,
	MINIMAP_MODE_BLACK,
	MINIMAP_MODE_WHITE
}
miniMapMode_t;

typedef struct game_s
{
	char				*arg;							/* -game matches this */
	char				*gamePath;						/* main game data dir */
	char				*homeBasePath;					/* home sub-dir on unix */
	char				*magic;							/* magic word for figuring out base path */
	char				*shaderPath;					/* shader directory */
	int					maxLMSurfaceVerts;				/* default maximum meta surface verts */
	int					maxSurfaceVerts;				/* default maximum surface verts */
	int					maxSurfaceIndexes;				/* default maximum surface indexes (tris * 3) */
	qboolean			emitFlares;						/* when true, emit flare surfaces */
	char				*flareShader;					/* default flare shader (MUST BE SET) */
	qboolean			wolfLight;						/* when true, lights work like wolf q3map  */
	int					lightmapSize;					/* bsp lightmap width/height */
	float				lightmapGamma;					/* default lightmap gamma */
	qboolean            lightmapsRGB;                   /* default lightmap sRGB mode */
	qboolean            texturesRGB;                    /* default texture sRGB mode */
	qboolean            colorsRGB;                      /* default color sRGB mode */
	float				lightmapExposure;				/* default lightmap exposure */
	float				lightmapCompensate;				/* default lightmap compensate value */
	qboolean            lightAngleHL;                   /* jal: use half-lambert curve for light angle attenuation */
	qboolean			noStyles;						/* use lightstyles hack or not */
	qboolean			keepLights;						/* keep light entities on bsp */
	qboolean			colorNormalize;					/* vortex: do light color normalization */
	float				patchSubdivisions;				/* default patch subdivisions tolerance */
	qboolean			patchShadows;					/* patch casting enabled */
	qboolean			deluxeMap;						/* compile deluxemaps */
	int					deluxeMode;						/* deluxemap mode (0 - modelspace, 1 - tangentspace with renormalization, 2 - tangentspace without renormalization) */
	int                 miniMapSize;                    /* minimap size */
	float               miniMapSharpen;                 /* minimap sharpening coefficient */
	float               miniMapBorder;                  /* minimap border amount */
	qboolean            miniMapKeepAspect;              /* minimap keep aspect ratio by letterboxing */
	miniMapMode_t       miniMapMode;                    /* minimap mode */
	char                *miniMapNameFormat;             /* minimap name format */
	char				*bspIdent;						/* 4-letter bsp file prefix */
	int					bspVersion;						/* bsp version to use */
	qboolean			lumpSwap;						/* cod-style len/ofs order */
	bspFunc				load, write;					/* load/write function pointers */
	surfaceParm_t		surfaceParms[ 128 ];			/* surfaceparm array */
}
game_t;

typedef struct image_s
{
	char				*name, *filename;
	int					refCount;
	int					width, height;
	byte				*pixels;
}
image_t;

typedef struct sun_s
{
	struct sun_s		*next;
	vec3_t				direction, color;
	float				photons, deviance, filterRadius;
	int					numSamples, style;
}
sun_t;


typedef struct surfaceModel_s
{
	struct surfaceModel_s	*next;
	char				model[ MAX_QPATH ];
	float				density, odds;
	float				minScale, maxScale;
	float				minAngle, maxAngle;
	qboolean			oriented;
}
surfaceModel_t;


/* ydnar/sd: foliage stuff for wolf et (engine-supported optimization of the above) */
typedef struct foliage_s
{
	struct foliage_s	*next;
	char				model[ MAX_QPATH ];
	float				scale, density, odds;
	qboolean			inverseAlpha;
}
foliage_t;

typedef struct foliageInstance_s
{
	vec3_t				xyz, normal;
}
foliageInstance_t;


typedef struct remap_s
{
	struct remap_s		*next;
	char				from[ MAX_OS_PATH ];
	char				to[ MAX_QPATH ];
}
remap_t;


/* wingdi.h hack, it's the same: 0 */
#undef CM_NONE

typedef enum
{
	CM_NONE,
	CM_VOLUME,
	CM_COLOR_SET,
	CM_ALPHA_SET,
	CM_COLOR_SCALE,
	CM_ALPHA_SCALE,
	CM_COLOR_DOT_PRODUCT,
	CM_ALPHA_DOT_PRODUCT,
	CM_COLOR_DOT_PRODUCT_2,
	CM_ALPHA_DOT_PRODUCT_2,
	CM_COLOR_INVERT,
	CM_ALPHA_INVERT,
	CM_ALPHA_RANDOM,
	CM_ALPHA_RANDOM_JITTER,
	CM_ALPHA_RANDOM_SCALE,
	CM_WATERALPHA,
	CM_WATERMOVE,
	CM_WATERSPRING,
	CM_WATERWARP,
	CM_ALPHA_VEGETATION
}
colorModType_t;


typedef struct colorMod_s
{
	struct colorMod_s	*next;
	colorModType_t		type;
	vec_t				data[ 16 ];
}
colorMod_t;


typedef enum
{
	IM_NONE,
	IM_OPAQUE,
	IM_MASKED,
	IM_BLEND
}
implicitMap_t;


typedef struct shaderInfo_s
{
	char				shader[ MAX_QPATH ];
	int					surfaceFlags;
	int					contentFlags;
	int					compileFlags;
	qboolean			skipSolidCull;
	float				value;							/* light value */

	char				*flareShader;					/* for light flares */
	char				*damageShader;					/* ydnar: sof2 damage shader name */
	char				*backShader;					/* for surfaces that generate different front and back passes */
	char				*cloneShader;					/* ydnar: for cloning of a surface */
	char				*remapShader;					/* ydnar: remap a shader in final stage */
	char				*deprecateShader;				/* vortex: shader is deprecated and replaced by this on use */
	char				*engineShader;					/* vortex: shader used by engine (have no q3map_ directives, used at optimize stage) */

	surfaceModel_t		*surfaceModel;					/* ydnar: for distribution of models */
	foliage_t			*foliage;						/* ydnar/splash damage: wolf et foliage */

	float				subdivisions;					/* from a "tesssize xxx" */
	float				backsplashFraction;				/* floating point value, usually 0.05 */
	float				backsplashDistance;				/* default 16 */
	float				lightSubdivide;					/* default 999 */
	float				lightFilterRadius;				/* ydnar: lightmap filtering/blurring radius for lights created by this shader (default: 0) */

	float				lightmapSampleSize;				/* lightmap sample size */
	float				lightmapSampleOffset;			/* ydnar: lightmap sample offset (default: 1.0) */

	float				bounceScale;					/* ydnar: radiosity re-emission [0,1.0+] */
	float				offset;							/* ydnar: offset in units */
	float				shadeAngleDegrees;				/* ydnar: breaking angle for smooth shading (degrees) */

	vec3_t				mins, maxs;						/* ydnar: for particle studio vertexDeform move support */

	qboolean			legacyTerrain;					/* ydnar: enable legacy terrain crutches */
	qboolean			indexed;						/* ydnar: attempt to use indexmap (terrain alphamap style) */
	qboolean			invert;							/* ydnar: reverse facing */
	qboolean			nonplanar;						/* ydnar: for nonplanar meta surface merging */
	qboolean			tcGen;							/* ydnar: has explicit texcoord generation */
	qboolean			forceMeta;						/* ydnar: force metasurface path */
	qboolean			forceSunlight;					/* force sun light at this surface even tho we might not calculate shadows in vertex lighting */
	qboolean			noMeta;							/* vortex: disable metasurface path */
	qboolean			noClip;							/* ydnar: don't clip into bsp, preserve original face winding */
	qboolean            noBSP;                          /* vortex: prevent adding brush faces to BSP tree */
	qboolean			noFast;							/* ydnar: supress fast lighting for surfaces with this shader */
	qboolean			noTJunc;						/* don't use this surface for tjunction fixing */
	qboolean			noFog;							/* ydnar: supress fogging */
	qboolean			noMerge;						/* vortex: do not merge this drawsurface in OPTIMIZE stage */
	qboolean			noVertexLight;					/* ydnar: leave vertex color alone */
	qboolean            noSmooth;                       /* vortex: doesnt allow vertex normal smoothing */
	qboolean            smoothOnly;						/* uq1: smooth the normals, but don't regenerate them */
	qboolean			fixVertexAlpha;					/* vortex: automatic vertexalpha fixer*/
	qboolean			splotchFix;						/* ydnar: filter splotches on lightmaps */
	qboolean			hasPasses;						/* false if the shader doesn't define any rendering passes */
	qboolean			globalTexture;					/* don't normalize texture repeats */
	qboolean			twoSided;						/* cull none */
	qboolean			autosprite;						/* autosprite shaders will become point lights instead of area lights */
	qboolean			polygonOffset;					/* ydnar: don't face cull this or against this */
	qboolean			patchShadows;					/* have patches casting shadows when using -light for this surface */
	qboolean			vertexShadows;					/* shadows will be casted at this surface even when vertex lit */
	qboolean            vertexPointSample;              /* vertex lightning used gridlight's point sample instead of full featured */
	qboolean			fogParms;						/* ydnar: has fogparms */
	qboolean			clipModel;						/* ydnar: solid model hack */
	qboolean            lightmapNoStitch;               /* vortex: doesnt allow lightmap stitching */
	qboolean            lightmapForceStitch;            /* vortex: enable lightmap stitching (if no -stitch set) */
	qboolean			lightmapMergable;				/* ydnar */
	byte				styleMarker;					/* ydnar: light styles hack */
	byte				lightStyle;
	qboolean			custom;
	qboolean			finished;
	qboolean			warnNoShaderImage;              /* vortex: used in CheckMapForErrors */
	qboolean			glow;							/* uq1: JKA glow. Need this to skip merging */

	vec3_t				vecs[ 2 ];						/* ydnar: explicit texture vectors for [0,1] texture space */
	tcMod_t				mod;							/* ydnar: q3map_tcMod matrix for djbob :) */
	vec3_t				lightmapAxis;					/* ydnar: explicit lightmap axis projection */
	colorMod_t			*colorMod;						/* ydnar: q3map_rgb/color/alpha/Set/Mod support */

	int					fixVertexAlphaLayers[2];        /* vortex: indexes for fixVertexAlpha layers */

	int					furNumLayers;					/* ydnar: number of fur layers */
	float				furOffset;						/* ydnar: offset of each layer */
	float				furFade;						/* ydnar: alpha fade amount per layer */

	float				vertexScale;					/* vertex light scale */
	float               vertexOcclusionBias;            /* vortex: bias the occlusion point (if trace hit position within this radius from sample, we are not shadowed) */

	float               aoScale;                        /* vortex: ambient occlusion scale */
	float               aoGainScale;                    /* vortex: ambient occlusion gain scale */

	char				skyParmsImageBase[ MAX_QPATH ];	/* ydnar: for skies */

	char				editorImagePath[ MAX_QPATH ];	/* use this image to generate texture coordinates */
	char				lightImagePath[ MAX_QPATH ];	/* use this image to generate color / averageColor */
	char				normalImagePath[ MAX_QPATH ];	/* ydnar: normalmap image for bumpmapping */

	implicitMap_t		implicitMap;					/* ydnar: enemy territory implicit shaders */
	char				implicitImagePath[ MAX_QPATH ];

	image_t				*shaderImage;
	image_t				*lightImage;
	image_t				*normalImage;

	float				skyLightValue;					/* ydnar */
	int					skyLightIterations;				/* ydnar */
	sun_t				*sun;							/* ydnar */

	vec3_t				color;							/* normalized color */
	vec4_t				averageColor;

	float				floodlightDirectionScale;       /* vortex: per-surface floodlight */
	vec3_t				floodlightRGB;                  /* vortex: per-surface floodlight */
	float				floodlightIntensity;            /* vortex: per-surface floodlight */
	float				floodlightDistance;             /* vortex: per-surface floodlight */

	vec3_t              ambient;                        /* vortex: ambient (lightmapping/vertexlight) */
	vec3_t              minlight;                       /* vortex: minlight (lightmapping/vertexlight) */
	vec3_t              minvertexlight;                 /* vortex: minvertexlight (vertexlight) */
	vec3_t              colormod;                       /* vortex: colormod (lightmapping/vertexlight) */

	int					lmCustomWidth, lmCustomHeight;	/* ydnar */
	float				lmBrightness;					/* ydnar */
	float				lmFilterRadius;					/* ydnar: lightmap filtering/blurring radius for this shader (default: 0) */
	int					shaderWidth, shaderHeight;		/* ydnar */
	float				stFlat[ 2 ];
	vec3_t				fogDir;							/* ydnar */
	char				*shaderText;					/* ydnar */

	int					materialType;					/* uq1 - warzone */

	qboolean			isTreeSolid;
	qboolean			isMapObjectSolid;
}
shaderInfo_t;



/* -------------------------------------------------------------------------------

bsp structures

------------------------------------------------------------------------------- */

typedef struct face_s
{
	struct face_s		*next;
	int					planenum;
	int					priority;
	qboolean			checked;
	int					compileFlags;
	winding_t			*w;
}
face_t;


typedef struct plane_s
{
	vec3_t				normal;
	vec_t				dist;
	int					type;
	struct plane_s		*hash_chain;
}
plane_t;


typedef struct side_s
{
	int					planenum;

	int					outputNum;			/* set when the side is written to the file list */

	float				texMat[ 2 ][ 3 ];	/* brush primitive texture matrix */
	float				vecs[ 2 ][ 4 ];		/* old-style texture coordinate mapping */

	winding_t			*winding;
	winding_t			*visibleHull;		/* convex hull of all visible fragments */

	shaderInfo_t		*shaderInfo;

	int					brushType;

	int					contentFlags;		/* from shaderInfo */
	int					surfaceFlags;		/* from shaderInfo */
	int					compileFlags;		/* from shaderInfo */
	int					value;				/* from shaderInfo */

	qboolean			visible;			/* choose visble planes first */
	qboolean			bevel;				/* don't ever use for bsp splitting, and don't bother making windings for it */
	qboolean			culled;				/* ydnar: face culling */
}
side_t;


typedef struct sideRef_s
{
	struct sideRef_s	*next;
	side_t				*side;
}
sideRef_t;


/* ydnar: generic index mapping for entities (natural extension of terrain texturing) */
typedef struct indexMap_s
{
	int					w, h, numLayers;
	char				name[ MAX_QPATH ], shader[ MAX_QPATH ];
	float				offsets[ 256 ];
	byte				*pixels;
}
indexMap_t;


typedef struct brush_s
{
	struct brush_s		*next;
	struct brush_s		*nextColorModBrush;	/* ydnar: colorMod volume brushes go here */
	struct brush_s		*original;			/* chopped up brushes will reference the originals */

	int					entityNum, mapEntityNum, brushNum;/* editor numbering */
	int					outputNum;			/* set when the brush is written to the file list */

	/* ydnar: for shadowcasting entities */
	char				castShadows;
	char				recvShadows;

	shaderInfo_t		*contentShader;
	shaderInfo_t		*celShader;			/* :) */

	/* ydnar: gs mods */
	float				lightmapScale;
	vec3_t              lightmapAxis;
	int					smoothNormals;
	int					vertTexProj;
	vec3_t				eMins, eMaxs;
	indexMap_t			*im;
	vec3_t              ambient;        /* vortex: ambient (lightmapping/vertexlight) */
	vec3_t              minlight;       /* vortex: minlight (lightmapping/vertexlight) */
	vec3_t              minvertexlight; /* vortex: minvertexlight (vertexlight) */
	vec3_t              colormod;       /* vortex: colormod (lightmapping/vertexlight) */

	int					contentFlags;
	int					compileFlags; /* ydnar */
	qboolean			detail;
	qboolean			nonsolid;     /* vortex: render-only brush */
	qboolean            noclip;       /* vortex: don't clip faces by BSP tree */
	qboolean            noTJunc;      /* vortex: disable T-junction fixing */
	qboolean			opaque;
	qboolean			isMapFileBrush = qfalse;
	int					portalareas[ 2 ];

	vec3_t				mins, maxs;
	int					numsides;

	qboolean			isOriginalMapSurface;

	side_t				sides[ 6 ];			/* variably sized */
}
brush_t;


typedef struct fog_s
{
	shaderInfo_t		*si;
	brush_t				*brush;
	int					visibleSide;		/* the brush side that ray tests need to clip against (-1 == none) */
}
fog_t;


typedef struct
{
	int					width, height;
	bspDrawVert_t		*verts;
}
mesh_t;


typedef struct parseMesh_s
{
	struct parseMesh_s	*next;

	int					entityNum, mapEntityNum, brushNum;	/* ydnar: editor numbering */

	/* ydnar: for shadowcasting entities */
	int					castShadows;
	int					recvShadows;

	mesh_t				mesh;
	shaderInfo_t		*shaderInfo;
	shaderInfo_t		*celShader;				/* :) */

	/* ydnar: gs mods */
	float				lightmapScale;
	vec3_t              lightmapAxis;
	int					smoothNormals; /* VorteX: per-entity normal smoothing (with func_group support) */
	int					vertTexProj;
	vec3_t				eMins, eMaxs;
	indexMap_t			*im;
	vec3_t              ambient;        /* vortex: ambient (lightmapping/vertexlight) */
	vec3_t              minlight;       /* vortex: minlight (lightmapping/vertexlight) */
	vec3_t              minvertexlight; /* vortex: minvertexlight (vertexlight) */
	vec3_t              colormod;       /* vortex: colormod (lightmapping/vertexlight) */
	qboolean            patchMeta;         /* vortex: patch meta on func_group */
	float               patchQuality;      /* vortex: patch meta on func_group */
	float               patchSubdivisions; /* vortex: patch meta on func_group */
	qboolean            nonsolid;

	/* grouping */
	qboolean			grouped;
	float				longestCurve;
	int					maxIterations;
}
parseMesh_t;


/*
	ydnar: the drawsurf struct was extended to allow for:
	- non-convex planar surfaces
	- non-planar brushface surfaces
	- lightmapped terrain
	- planar patches
*/

typedef enum
{
	/* ydnar: these match up exactly with bspSurfaceType_t */
	SURFACE_BAD,
	SURFACE_FACE,
	SURFACE_PATCH,
	SURFACE_TRIANGLES,
	SURFACE_FLARE,
	SURFACE_FOLIAGE,	/* wolf et */

	/* ydnar: compiler-relevant surface types */
	SURFACE_FORCED_META,
	SURFACE_META,
	SURFACE_FOGHULL,
	SURFACE_DECAL,
	SURFACE_SHADER,

	NUM_SURFACE_TYPES
}
surfaceType_t;

Q_EXTERN char	*surfaceTypes[]
#ifndef MAIN_C
				;
#else
				=
				{
					"SURFACE_BAD",
					"SURFACE_FACE",
					"SURFACE_PATCH",
					"SURFACE_TRIANGLES",
					"SURFACE_FLARE",
					"SURFACE_FOLIAGE",
					"SURFACE_FORCED_META",
					"SURFACE_META",
					"SURFACE_FOGHULL",
					"SURFACE_DECAL",
					"SURFACE_SHADER"
				};
#endif


/* ydnar: this struct needs an overhaul (again, heh) */
typedef struct mapDrawSurface_s
{
	surfaceType_t		type;
	qboolean			planar;
	int					outputNum;			/* ydnar: to match this sort of thing up */

	qboolean			fur;				/* ydnar: this is kind of a hack, but hey... */
	qboolean			skybox;				/* ydnar: yet another fun hack */
	qboolean			backSide;			/* ydnar: q3map_backShader support */

	struct mapDrawSurface_s	*parent;		/* ydnar: for cloned (skybox) surfaces to share lighting data */
	struct mapDrawSurface_s	*clone;			/* ydnar: for cloned surfaces */
	struct mapDrawSurface_s	*cel;			/* ydnar: for cloned cel surfaces */

	shaderInfo_t		*shaderInfo;
	shaderInfo_t		*celShader;
	brush_t				*mapBrush;
	parseMesh_t			*mapMesh;
	sideRef_t			*sideRef;

	int					fogNum;

	int					numVerts;			/* vertexes and triangles */
	bspDrawVert_t		*verts;
	int					numIndexes;
	int					*indexes;

	int					currentNumVerts;
	int					currentNumIndexes;

	int					planeNum;
	vec3_t				lightmapOrigin;		/* also used for flares */
	vec3_t				lightmapVecs[ 3 ];	/* also used for flares */
	int					lightStyle;			/* used for flares */

	/* ydnar: gs mods */
	float				lightmapScale;
	int					smoothNormals;
	int					vertTexProj;
	qboolean			noAlphaFix;
	qboolean            noClip;
	qboolean            noTJunc;
	vec3_t              minlight;
	vec3_t              minvertexlight;
	vec3_t              ambient;
	vec3_t              colormod;

	/* ydnar: surface classification */
	vec3_t				mins, maxs;
	vec3_t				lightmapAxis;
	float				sampleSize;

	/* ydnar: shadow group support */
	char				castShadows, recvShadows;

	/* ydnar: texture coordinate range monitoring for hardware with limited texcoord precision (in texel space) */
	float				bias[ 2 ];
	int					texMins[ 2 ], texMaxs[ 2 ], texRange[ 2 ];

	/* ydnar: for patches */
	float				longestCurve;
	int					maxIterations;
	int					patchWidth, patchHeight;
	vec3_t				bounds[ 2 ];
	qboolean            patchMeta;
	float               patchQuality;
	float               patchSubdivisions;

	/* ydnar/sd: for foliage */
	int					numFoliageInstances;

	/* ydnar: editor/useful numbering */
	int					entityNum;
	int                 mapEntityNum;
	int					surfaceNum;
}
mapDrawSurface_t;


typedef struct drawSurfRef_s
{
	struct drawSurfRef_s	*nextRef;
	int					outputNum;
}
drawSurfRef_t;


/* ydnar: metasurfaces are constructed from lists of metatriangles so they can be merged in the best way */
typedef struct metaTriangle_s
{
	mapDrawSurface_t    *ds;
	shaderInfo_t		*si;
	side_t				*side;
	int					entityNum, mapEntityNum, planeNum, fogNum;
	float               sampleSize;
	char				castShadows, recvShadows;
	vec3_t              minlight, minvertexlight, ambient, colormod;
	int					smoothNormals;
	vec3_t				mins;
	vec3_t				maxs;
	vec4_t				plane;
	vec3_t				lightmapAxis;
	int					indexes[ 3 ];
	float               area;
}
metaTriangle_t;

typedef struct epair_s
{
	struct epair_s		*next;
	char				*key, *value;
}
epair_t;

typedef struct
{
	vec3_t				origin;
	brush_t				*brushes, *lastBrush, *colorModBrushes;
	parseMesh_t			*patches;
	int					mapEntityNum, firstDrawSurf;
	int					firstBrush, numBrushes;		/* only valid during BSP compile */
	qboolean			forceSubmodel; /* vortex: entity goes thru SubModel code */
	epair_t				*epairs;
	vec3_t              originbrush_origin;
	float				lowestPointNear;
	qboolean			alreadyAdded = qfalse;
}
entity_t;


typedef struct node_s
{
	/* both leafs and nodes */
	int					planenum;		/* -1 = leaf node */
	struct node_s		*parent;
	vec3_t				mins, maxs;		/* valid after portalization */
	brush_t				*volume;		/* one for each leaf/node */

	/* nodes only */
	side_t				*side;			/* the side that created the node */
	struct node_s		*children[ 2 ];
	int					compileFlags;	/* ydnar: hint, antiportal */
	int					tinyportals;
	vec3_t				referencepoint;

	/* leafs only */
	qboolean			opaque;			/* view can never be inside */
	qboolean			areaportal;
	qboolean			skybox;			/* ydnar: a skybox leaf */
	qboolean			sky;			/* ydnar: a sky leaf */
	int					cluster;		/* for portalfile writing */
	int					area;			/* for areaportals */
	brush_t				*brushlist;		/* fragments of all brushes in this leaf */
	drawSurfRef_t		*drawSurfReferences;

	int					occupied;		/* 1 or greater can reach entity */
	entity_t			*occupant;		/* for leak file testing */

	struct portal_s		*portals;		/* also on nodes during construction */
}
node_t;


typedef struct portal_s
{
	plane_t				plane;
	node_t				*onnode;		/* NULL = outside box */
	node_t				*nodes[ 2 ];	/* [ 0 ] = front side of plane */
	struct portal_s		*next[ 2 ];
	winding_t			*winding;

	qboolean			sidefound;		/* false if ->side hasn't been checked */
	int					compileFlags;	/* from original face that caused the split */
	side_t				*side;			/* NULL = non-visible */
}
portal_t;


typedef struct
{
	node_t				*headnode;
	node_t				outside_node;
	vec3_t				mins, maxs;
}
tree_t;



/* -------------------------------------------------------------------------------

vis structures

------------------------------------------------------------------------------- */

typedef struct
{
	vec3_t				normal;
	float				dist;
}
visPlane_t;


typedef struct
{
	uint32_t/*int*/					numpoints;
	vec3_t				points[	MAX_POINTS_ON_FIXED_WINDING	];		/* variable sized */
}
fixedWinding_t;


typedef struct passage_s
{
	struct passage_s	*next;
	byte				cansee[ 1 ];	/* all portals that can be seen through this passage */
} passage_t;


typedef enum
{
	stat_none,
	stat_working,
	stat_done
}
vstatus_t;


typedef struct
{
	int					num;
	int/*qboolean*/			hint;			/* true if this portal was created from a hint splitter */
	qboolean			removed;
	visPlane_t			plane;			/* normal pointing into neighbor */
	int					leaf;			/* neighbor */

	vec3_t				origin;			/* for fast clip testing */
	float				radius;

	fixedWinding_t		*winding;
	vstatus_t			status;
	byte				*portalfront;	/* [portals], preliminary */
	byte				*portalflood;	/* [portals], intermediate */
	byte				*portalvis;		/* [portals], final */

	int					nummightsee;	/* bit count on portalflood for sort */
	passage_t			*passages;		/* there are just as many passages as there */
										/* are portals in the leaf this portal leads */
}
vportal_t;


typedef struct leaf_s
{
	int					numportals;
	int					merged;
	vportal_t			*portals[MAX_PORTALS_ON_LEAF];
}
leaf_t;


typedef struct pstack_s
{
	byte				mightsee[ MAX_PORTALS / 8 ];
	struct pstack_s		*next;
	leaf_t				*leaf;
	vportal_t			*portal;		/* portal exiting */
	fixedWinding_t		*source;
	fixedWinding_t		*pass;

	fixedWinding_t		windings[ 3 ];	/* source, pass, temp in any order */
	int					freewindings[ 3 ];

	visPlane_t			portalplane;
	int depth;
#ifdef SEPERATORCACHE
	visPlane_t			seperators[ 2 ][ MAX_SEPERATORS ];
	int					numseperators[ 2 ];
#endif
}
pstack_t;


typedef struct
{
	vportal_t			*base;
	int					c_chains;
	pstack_t			pstack_head;
}
threaddata_t;



/* -------------------------------------------------------------------------------

light structures

------------------------------------------------------------------------------- */

/* ydnar: new light struct with flags */
typedef struct light_s
{
	struct light_s		*next;

	int					type;
	int					flags;			/* ydnar: condensed all the booleans into one flags int */
	shaderInfo_t		*si;

	vec3_t				origin;
	vec3_t				normal;			/* for surfaces, spotlights, and suns */
	float				dist;			/* plane location along normal */

	float				photons;
	int					mindist;		/* vortex: minimum distance to prevent super hot spots (default is 16) */
	int					style;
	vec3_t				color;
	float				radiusByDist;	/* for spotlights */
	float				fade;			/* ydnar: from wolf, for linear lights */
	float				angleScale;		/* ydnar: stolen from vlight for K */

	float				add;			/* ydnar: used for area lights */
	float				envelope;		/* ydnar: units until falloff < tolerance */
	float				envelope2;		/* ydnar: envelope squared (tiny optimization) */
	vec3_t				mins, maxs;		/* ydnar: pvs envelope */
	int					cluster;		/* ydnar: cluster light falls into */

	winding_t			*w;
	vec3_t				emitColor;		/* full out-of-gamut value */

	int                 devianceRadius;  /* radius of light-casting zone  */
	int                 devianceSamples; /* number of deviance samples    */
	vec3_t			   *devianceOrigins; /* deviance origins for penumbra */

	float				falloffTolerance;	/* ydnar: minimum attenuation threshold */
	float				filterRadius;	/* ydnar: lightmap filter radius in world units, 0 == default */
}
light_t;


typedef struct
{
	/* constant input */
	qboolean			testOcclusion, forceSunlight, testAll, forceSelfShadow;
	int					recvShadows;
	float               occlusionBias;

	int                 entityNum;
	int					numSurfaces;
	int					*surfaces;

	int					numLights;
	light_t				**lights;

	qboolean			twoSided;

	/* per-sample input */
	int					cluster;
	vec3_t				origin, normal;
	vec_t				inhibitRadius;	/* sphere in which occluding geometry is ignored */

	/* per-light input */
	light_t				*light;
	vec3_t				end;

	/* calculated input */
	vec3_t				displacement, direction;
	vec_t				distance;

	/* input and output */
	vec3_t				color;			/* starts out at full color, may be reduced if transparent surfaces are crossed */
	vec3_t				colorNoShadow;	/* vortex: no shadow tracing for deluxemaps */

	/* output */
	vec3_t				hit;
	int					compileFlags;	/* for determining surface compile flags traced through */
	qboolean			passSolid;
	qboolean			opaque;
	shaderInfo_t        *skyLightShader;

	/* working data */
	int					numTestNodes;
	int					testNodes[ MAX_TRACE_TEST_NODES ];
}
trace_t;



/* must be identical to bspDrawVert_t except for float color! */
typedef struct
{
	vec3_t				xyz;
	float				st[ 2 ];
	float				lightmap[ MAX_LIGHTMAPS ][ 2 ];
	vec3_t				normal;
	float				color[ MAX_LIGHTMAPS ][ 4 ];
}
radVert_t;


typedef struct
{
	int					numVerts;
	radVert_t			verts[ MAX_POINTS_ON_WINDING ];
}
radWinding_t;


/* crutch for poor local allocations in win32 smp */
typedef struct
{
	vec_t				dists[ MAX_POINTS_ON_WINDING + 4 ];
	int					sides[ MAX_POINTS_ON_WINDING + 4 ];
}
clipWork_t;


/* ydnar: new lightmap handling code */
typedef struct outLightmap_s
{
	int					lightmapNum, extLightmapNum;
	int					customWidth, customHeight;
	int					numLightmaps;
	int					freeLuxels;
	int					numShaders;
	shaderInfo_t		*shaders[ MAX_LIGHTMAP_SHADERS ];
	byte				*lightBits;
	byte				*bspLightBytes;
	byte				*bspDirBytes;
}
outLightmap_t;


typedef struct rawLightmap_s
{
	qboolean				finished, splotchFix, wrap[ 2 ], stitch, translucent, unused2, unused3;
	int						customWidth, customHeight;
	float					brightness;
	float                   shadeAngle;
	float					filterRadius;

	int						firstLightSurface, numLightSurfaces;	/* index into lightSurfaces */
	int						numLightClusters, *lightClusters;

	float                   sampleSize;
	float                   actualSampleSize;
	float                   sampleOffset;
	int						axisNum;

	int						entityNum;
	char					recvShadows;
	vec3_t					mins, maxs, axis, origin, *vecs;
	float					*plane;
	int						w, h, sw, sh, used;

	qboolean				solid[ MAX_LIGHTMAPS ];
	vec3_t					solidColor[ MAX_LIGHTMAPS ];

	int						numStyledTwins;
	struct rawLightmap_s	*twins[ MAX_LIGHTMAPS ];

	int						outLightmapNums[ MAX_LIGHTMAPS ];
	int						twinNums[ MAX_LIGHTMAPS ];
	int						lightmapX[ MAX_LIGHTMAPS ], lightmapY[ MAX_LIGHTMAPS ];
	byte					styles[ MAX_LIGHTMAPS ];
	float					*bspLuxels[ MAX_LIGHTMAPS ];
	float					*radLuxels[ MAX_LIGHTMAPS ];
	float					*superLuxels[ MAX_LIGHTMAPS ];
	float					*superOrigins;
	float					*superNormals;
	int						*superClusters;
	float					*superDeluxels;	  /* average light direction */
	float					*bspDeluxels;
	float					*bspNormals;
	float					*superFloodLight; /* floodlight color */
	float					*superTriorigins; /* sample real origins (on a triangle) */
	float					*superTrinormals; /* triangle normals */

	/* vortex: per-surface lighting control */
	float					floodlightDirectionScale;
	vec3_t					floodlightRGB;
	float					floodlightIntensity;
	float					floodlightDistance;
	float                   aoScale;
	float                   aoGainScale;
	vec3_t                  ambient;
	vec3_t                  minlight;
	vec3_t                  colormod;
}
rawLightmap_t;


typedef struct rawGridPoint_s
{
	vec3_t				ambient[ MAX_LIGHTMAPS ];
	vec3_t				directed[ MAX_LIGHTMAPS ];
	vec3_t				dir;
	byte				styles[ MAX_LIGHTMAPS ];
	qboolean            mapped;        // vortex: inside lightgrid brush (if any)
	qboolean            flooded;       // vortex: inside wall, flood from the neightbors
	qboolean            floodOnlyColor;// vortex: do not flood direction
	qboolean            unused1;       //
	qboolean            unused2;
	int                 contributions; // vortex: how many lights contributed to this point
}
rawGridPoint_t;

typedef struct surfaceInfo_s
{
	bspModel_t			*model;
	shaderInfo_t		*si;
	rawLightmap_t		*lm;
	int					parentSurfaceNum, childSurfaceNum;
	int					entityNum, patchIterations;
	float               sampleSize;
	char				castShadows, recvShadows, unused2, unused3;
	float				longestCurve;
	float				*plane;
	vec3_t				axis, mins, maxs;
	qboolean			hasLightmap, approximated, lightmapStitch, unused1;
	int					firstSurfaceCluster, numSurfaceClusters;
	float               shadeAngle;
	vec3_t              ambient;
	vec3_t              minlight;
	vec3_t              minvertexlight;
	vec3_t              colormod;
}
surfaceInfo_t;

/* -------------------------------------------------------------------------------

prototypes

------------------------------------------------------------------------------- */

/* main.c */
vec_t						Random( void );
int							BSPInfo( int count, char **fileNames );
int							ScaleBSPMain( int argc, char **argv );
int							ConvertMain( int argc, char **argv );


/* path_init.c */
game_t						*GetGame( char *arg );
void						InitPaths( int *argc, char **argv );
qboolean                    GetGamePath(char *fileFullPath, char *outGamePath);
void                        SanitizePath(char *path, char *out);

/* bsp.c */
int							BSPMain( int argc, char **argv );

/* patchbsp.c */
entity_t                    *AllocateEntity(entity_t *base);

/* convert_map.c */
int							ConvertBSPToMap( char *bspName, int collapseByTexture );


/* convert_ase.c */
int							ConvertBSPToASE( char *bspName, int collapseByTexture );

/* brush.c */
sideRef_t					*AllocSideRef( side_t *side, sideRef_t *next );
int							CountBrushList( brush_t *brushes );
brush_t						*AllocBrush( int numsides );
void						FreeBrush( brush_t *brushes );
void						FreeBrushList( brush_t *brushes );
brush_t                     *CopyBrush( brush_t *brush );
qboolean					BoundBrush( brush_t *brush );
qboolean					CreateBrushWindings( brush_t *brush );
brush_t						*BrushFromBounds( float minx, float miny, float minz, float maxx, float maxy, float maxz, shaderInfo_t *si );
vec_t						BrushVolume( brush_t *brush );
void						WriteBSPBrushMap( char *name, brush_t *list );

void						FilterDetailBrushesIntoTree( entity_t *e, tree_t *tree );
void						FilterStructuralBrushesIntoTree( entity_t *e, tree_t *tree, qboolean quiet );
void                        FilterStructuralBrushesIntoTreeStats( void );

int							BoxOnPlaneSide( vec3_t mins, vec3_t maxs, plane_t *plane );
qboolean					WindingIsTiny( winding_t *w );

void						SplitBrush( brush_t *brush, int planenum, brush_t **front, brush_t **back);

tree_t						*AllocTree( void );
node_t						*AllocNode( void );


/* mesh.c */
void						LerpDrawVert( bspDrawVert_t *a, bspDrawVert_t *b, bspDrawVert_t *out );
void						LerpDrawVertAmount( bspDrawVert_t *a, bspDrawVert_t *b, float amount, bspDrawVert_t *out );
void						FreeMesh( mesh_t *m );
mesh_t						*CopyMesh( mesh_t *mesh );
void						PrintMesh( mesh_t *m );
mesh_t						*TransposeMesh( mesh_t *in );
void						InvertMesh( mesh_t *m );
mesh_t						*SubdivideMesh( mesh_t in, float maxError, float minLength );
int							IterationsForCurve( float len, float subdivisions );
mesh_t						*SubdivideMesh2( mesh_t in, int iterations );
mesh_t						*SubdivideMeshQuads( mesh_t *in, float minLength, int maxsize, int *widthtable, int *heighttable );
mesh_t						*RemoveLinearMeshColumnsRows( mesh_t *in );
void						MakeMeshNormals( mesh_t in );
void						PutMeshOnCurve( mesh_t in );

/* map.c */
int                         CheckMapForErrors( void );
void 						LoadMapFile( char *filename, qboolean onlyLights, qboolean onlyLightgridBrushes, qboolean onlyFoliage, qboolean externalFile );
int							FindFloatPlane( vec3_t normal, vec_t dist, int numPoints, vec3_t *points );
int							PlaneTypeForNormal( vec3_t normal );
void						AddBrushBevels( void );
brush_t						*FinishBrush( void );

/* portals.c */
void						MakeHeadnodePortals( tree_t *tree );
void						MakeNodePortal( node_t *node );
void						SplitNodePortals( node_t *node );

qboolean					PortalPassable( portal_t *p );

qboolean					FloodEntities( tree_t *tree, qboolean quiet );
void                        FloodEntitiesStats( void );
void						FillOutside( node_t *headnode );
void                        FillOutsideStats (void);
void						FloodAreas( tree_t *tree);
face_t						*VisibleFaces( entity_t *e, tree_t *tree );
void						FreePortal( portal_t *p );

void						MakeTreePortals( tree_t *tree, qboolean quiet );
void                        MakeTreePortalsStats( void );


/* leakfile.c */
xmlNodePtr					LeakFile( tree_t *tree );


/* prtfile.c */
void						NumberClusters( tree_t *tree );
void						WritePortalFile( tree_t *tree );


/* writebsp.c */
void						SetModelNumbers( void );
void						SetLightStyles( void );

int							EmitShader( const char *shader, int *contentFlags, int *surfaceFlags );

void						BeginBSPFile( void );
void						EndBSPFile( void );
void						EmitBrushes( brush_t *brushes, int *firstBrush, int *numBrushes );
void						EmitFogs( void );

void						BeginModel( void );
void						EndModel( entity_t *e, node_t *headnode );


/* tree.c */
void						FreeTree( tree_t *tree );
void						FreeTree_r( node_t *node );
void						PrintTree_r( node_t *node, int depth );
void						FreeTreePortals_r( node_t *node );


/* patch.c */
void						ParsePatch( qboolean onlyLights );
mesh_t						*SubdivideMesh( mesh_t in, float maxError, float minLength );
void						PatchMapDrawSurfs( entity_t *e );
void						TriangulatePatchSurface( entity_t *e , mapDrawSurface_t *ds );


/* tjunction.c */
void						FixTJunctions( entity_t *e );

/* fog.c */
winding_t					*WindingFromDrawSurf( mapDrawSurface_t *ds );
void						FogDrawSurfaces( entity_t *e );
int							FogForPoint( vec3_t point, float epsilon );
int							FogForBounds( vec3_t mins, vec3_t maxs, float epsilon );
void						CreateMapFogs( void );


/* facebsp.c */
face_t						*MakeStructuralBSPFaceList( brush_t *list );
face_t						*MakeVisibleBSPFaceList( brush_t *list );
tree_t						*FaceBSP( face_t *list, qboolean quiet );
void                         FaceBSPStats( void );


/* model.c */
void                        LoadTriangleModels( void );
void						PicoPrintFunc( int level, const char *str );
void						PicoLoadFileFunc( char *name, byte **buffer, int *bufSize );
picoModel_t					*FindModel( const char *name, int frame );
picoModel_t					*LoadModel( const char *name, int frame );
void						InsertModel(char *name, int frame, int skin, m4x4_t transform, float uvScale, remap_t *remap, shaderInfo_t *celShader, shaderInfo_t *overrideShader, qboolean forcedSolid, qboolean forcedFullSolid, qboolean forcedNoSolid, int entityNum, int mapEntityNum, char castShadows, char recvShadows, int spawnFlags, float lightmapScale, vec3_t lightmapAxis, vec3_t minlight, vec3_t minvertexlight, vec3_t ambient, vec3_t colormod, float lightmapSampleSize, int shadeAngle, int vertTexProj, qboolean noAlphaFix, float pushVertexes, qboolean skybox, int *added_surfaces, int *added_verts, int *added_triangles, int *added_brushes, qboolean cullSmallSolids, float LOWEST_NEAR_POINT, qboolean isLodModel, int centerChunkPass);
void						AddTriangleModels( int entityNum, qboolean quiet, qboolean cullSmallSolids, qboolean chunksPass, qboolean centerChunkPass);


/* surface.c */
mapDrawSurface_t			*AllocDrawSurface( surfaceType_t type );
void						FinishSurface( mapDrawSurface_t *ds );
void						StripFaceSurface( mapDrawSurface_t *ds, qboolean onlyCreateIndexes );
qboolean					CalcSurfaceTextureRange( mapDrawSurface_t *ds );
qboolean					CalcLightmapAxis( vec3_t normal, vec3_t axis );
void                        ClassifySurfaces( int numSurfs, mapDrawSurface_t *ds );
void						ClassifyEntitySurfaces( entity_t *e );
void						TidyEntitySurfaces( entity_t *e );
mapDrawSurface_t			*CloneSurface( mapDrawSurface_t *src, shaderInfo_t *si );
mapDrawSurface_t			*MakeCelSurface( mapDrawSurface_t *src, shaderInfo_t *si );
qboolean					IsTriangleDegenerate( bspDrawVert_t *points, int a, int b, int c );
void						ClearSurface( mapDrawSurface_t *ds );
void						AddEntitySurfaceModels( entity_t *e );
mapDrawSurface_t			*DrawSurfaceForSide( entity_t *e, brush_t *b, side_t *s, winding_t *w, vec3_t *vertexCache, int *numVertsInCache, int maxCache );
mapDrawSurface_t			*DrawSurfaceForMesh( entity_t *e, parseMesh_t *p, mesh_t *mesh );
mapDrawSurface_t			*DrawSurfaceForFlare( int entNum, vec3_t origin, vec3_t normal, vec3_t color, char *flareShader, int lightStyle );
mapDrawSurface_t			*DrawSurfaceForShader( char *shader );
void						ClipSidesIntoTree( entity_t *e, tree_t *tree, qboolean quiet );
void                        ClipSidesIntoTreeStats( void );
void						MakeDebugPortalSurfs( tree_t *tree );
void						MakeFogHullSurfs( entity_t *e, tree_t *tree, char *shader );
void						SubdivideFaceSurfaces( entity_t *e, tree_t *tree );
void						AddEntitySurfaceModels( entity_t *e );
int							AddSurfaceModels( mapDrawSurface_t *ds );
void                        DeformVertexes( mapDrawSurface_t *ds, float pushVertexes );
void						FilterDrawsurfsIntoTree( entity_t *e, tree_t *tree, qboolean showpacifier );
void                        EmitDrawsurfsStats( void );
void                        EmitDrawsurfsSimpleStats( void );
void						EmitPatchSurface( entity_t *e, mapDrawSurface_t *ds );
static void					EmitTriangleSurface( mapDrawSurface_t *ds );


/* surface_fur.c */
void						Fur( mapDrawSurface_t *src );


/* surface_foliage.c */
void						Foliage( mapDrawSurface_t *src );


/* ydnar: surface_meta.c */
void						ClearMetaTriangles( void );
int							FindMetaTriangle( metaTriangle_t *src, bspDrawVert_t *a, bspDrawVert_t *b, bspDrawVert_t *c, int planeNum );
void						MakeEntityMetaTriangles( entity_t *e );
void						FixMetaTJunctions( void );
void						SmoothMetaTriangles( void );
void						MergeMetaTriangles( void );
void						EmitMetaStats(); // vortex: print meta statistics even in no-verbose mode


/* surface_extra.c */
void						SetDefaultSampleSize( float sampleSize );

void						SetSurfaceExtra( mapDrawSurface_t *ds, int num );

shaderInfo_t				*GetSurfaceExtraShaderInfo( int num );
int							GetSurfaceExtraParentSurfaceNum( int num );
int							GetSurfaceExtraEntityNum( int num );
int                         GetSurfaceExtraMapEntityNum( int num );
char						GetSurfaceExtraCastShadows( int num );
char						GetSurfaceExtraRecvShadows( int num );
float						GetSurfaceExtraSampleSize( int num );
float						GetSurfaceExtraLongestCurve( int num );
void						GetSurfaceExtraLightmapAxis( int num, vec3_t lightmapAxis );
float						GetSurfaceExtraShadeAngle( int num );
qboolean					GetSurfaceExtraLightmapStitch( int num );
void                        GetSurfaceExtraAmbient( int num, vec3_t ambient );
void                        GetSurfaceExtraMinLight( int num, vec3_t minlight );
void                        GetSurfaceExtraMinVertexLight( int num, vec3_t minvertexlight );
void                        GetSurfaceExtraColormod( int num, vec3_t colormod );

void						WriteSurfaceExtraFile( const char *path );
void						LoadSurfaceExtraFile( const char *path );


/* decals.c */
void						ProcessDecals( void );
void						MakeEntityDecals( entity_t *e );


/* decorations.c */
void						LoadDecorations( char *source );
void						ProcessDecorations( void );

/* brush_primit.c */
void						ComputeAxisBase( vec3_t normal, vec3_t texX, vec3_t texY);

/* vis.c */
fixedWinding_t				*NewFixedWinding( int points );
int							VisMain( int argc, char **argv );

/* visflow.c */
//int							CountBits( byte *bits, uint32_t numbits );
uint32_t					CountBits(byte *bits, uint32_t numbits);
void						PassageFlow( int portalnum );
void						CreatePassages( int portalnum );
void						PassageMemory( void );
void						BasePortalVis( int portalnum );
void						BetterPortalVis( int portalnum );
void						PortalFlow( int portalnum );
void						PassagePortalFlow( int portalnum );



/* light.c  */
float						PointToPolygonFormFactor( const vec3_t point, const vec3_t normal, const winding_t *w );
int							LightContribution ( trace_t *trace, int lightflags, qboolean point3d );
void						LightContributionAllStyles( trace_t *trace, byte styles[ MAX_LIGHTMAPS ], vec3_t colors[ MAX_LIGHTMAPS ], int lightflags, qboolean point3d );
int                         LightContributionSuper(trace_t *trace, int lightflags, qboolean point3d, int samples, const vec3_t multiVec1, const vec3_t multiVec2, const vec3_t multiVec3, float sampleSize );
int							LightMain( int argc, char **argv );


/* light_trace.c */
void						SetupTraceNodes( void );
void                        SetupTraceNodes_GPU(void);
void						TraceLine( trace_t *trace );
float						SetupTrace( trace_t *trace );


/* light_bounce.c */
qboolean					RadSampleImage( byte *pixels, int width, int height, float st[ 2 ], float color[ 4 ] );
void						RadLightForTriangles( int num, int lightmapNum, rawLightmap_t *lm, shaderInfo_t *si, float scale, float subdivide, clipWork_t *cw );
void						RadLightForPatch( int num, int lightmapNum, rawLightmap_t *lm, shaderInfo_t *si, float scale, float subdivide, clipWork_t *cw );
void						RadCreateDiffuseLights( void );
void						RadFreeLights();


/* light_ydnar.c */
void						SmoothNormals( void );

void						MapRawLightmap(int num);

void						SetupDirt();
float						DirtForSample( trace_t *trace  );
void						DirtyRawLightmap(int num);

void                        SetupGrid();
void                        AllocateGridArea(vec3_t mins, vec3_t maxs);
void                        SampleGrid(vec3_t origin, vec3_t outambient, vec3_t outdirected, vec3_t outdirection );

void						SetupFloodLight();
void						FloodlightRawLightmaps();
float						FloodLightForSample( trace_t *trace , float floodLightDistance, qboolean floodLightLowQuality);
void						FloodLightRawLightmap(int num);

void						IlluminateRawLightmap(int num);
void						LightGridRawLightmap(int num);
void						FilterRawLightmap(int num);
void						StitchRawLightmap(int num);
void						IlluminateVertexes(int num);
void						WriteRawLightmapsFile( const char *outFile );

void						SetupBrushes( void );
void						SetupClusters( void );
qboolean					ClusterVisible( int a, int b );
qboolean					ClusterVisibleToPoint( vec3_t point, int cluster );
int							ClusterForPoint( vec3_t point );
int							ClusterForPointExt( vec3_t point, float epsilon );
int							ClusterForPointExtFilter( vec3_t point, float epsilon, int numClusters, int *clusters );
int							ShaderForPointInLeaf( vec3_t point, int leafNum, float epsilon, int wantContentFlags, int wantSurfaceFlags, int *contentFlags, int *surfaceFlags );
void						SetupEnvelopes( qboolean forGrid, qboolean fastFlag );
void						FreeTraceLights( trace_t *trace );
void						CreateTraceLightsForBounds( qboolean forGrid, vec3_t mins, vec3_t maxs, vec3_t normal, int numClusters, int *clusters, int flags, trace_t *trace );
void						CreateTraceLightsForSurface( int num, trace_t *trace );


/* lightmaps_ydnar.c */
void						ExportLightmaps( void );

int							ExportLightmapsMain( int argc, char **argv );
int							ImportLightmapsMain( int argc, char **argv );

void						SetupSurfaceLightmaps( void );
void						AllocateSurfaceLightmaps( void );
void						StitchRawLightmaps( void );
void						StoreSurfaceLightmaps( void );

void                        GetExternalLightmapPath(char *source, char *prefixPath, int lightmapNum, char *ext, char *outLightmapName);

/* optimize.c */
int                         OptimizeBSPMain( int argc, char **argv );

/* smoothnormals.c */
int							SmoothBSPMain(int argc, char **argv);

/* lightmapGPU.c */
int							LightmapGPUMain(int argc, char **argv);

/* patchbsp.c */
int                         PatchBSPMain( int argc, char **argv );

/* image.c */
void						ImageFree( image_t *image );
image_t						*ImageFind( const char *filename );
image_t						*ImageLoad( const char *filename );


/* shaders.c */
void						ColorMod( entity_t *e, mapDrawSurface_t *ds, colorMod_t *am, brush_t *b, int startVert, int numVerts );

void						TCMod( tcMod_t mod, float st[ 2 ] );
void						TCModIdentity( tcMod_t mod );
void						TCModMultiply( tcMod_t a, tcMod_t b, tcMod_t out );
void						TCModTranslate( tcMod_t mod, float s, float t );
void						TCModScale( tcMod_t mod, float s, float t );
void						TCModRotate( tcMod_t mod, float euler );

qboolean					ApplySurfaceParm( char *name, int *contentFlags, int *surfaceFlags, int *compileFlags );

void						BeginMapShaderFile( const char *mapFile );
void						WriteMapShaderFile( void );
shaderInfo_t				*CustomShader( shaderInfo_t *si, char *find, char *replace );
void						EmitVertexRemapShader( char *from, char *to );

void						LoadShaderInfo( void );
shaderInfo_t				*ShaderInfoForShader( const char *shader );


/* bspfile_abstract.c */
void						SetGridPoints( int n );
void						SetDrawVerts( int n );
void						IncDrawVerts();
void						SetDrawSurfaces(int n);
void						SetDrawSurfacesBuffer();

void						SwapBlock( int *block, int size );

int							GetLumpElements( bspHeader_t *header, int lump, int size );
void						*GetLump( bspHeader_t *header, int lump );
int							CopyLump( bspHeader_t *header, int lump, void *dest, int size );
void						AddLump( FILE *file, bspHeader_t *header, int lumpNum, const void *data, int length );

void						LoadBSPFile( const char *filename );
void						WriteBSPFile( const char *filename );
void						PrintBSPFileSizes( void );

epair_t						*ParseEPair( void );
void						ParseEntities( void );
void						UnparseEntities( qboolean removeQ3map2keys );
void						PrintEntity( const entity_t *ent );
void						SetKeyValue( entity_t *ent, const char *key, const char *value );
qboolean					KeyExists( const entity_t *ent, const char *key ); /* VorteX: check if key exists */
const char					*ValueForKey( const entity_t *ent, const char *key );
int							IntForKey( const entity_t *ent, const char *key );
vec_t						FloatForKey( const entity_t *ent, const char *key );
void						GetVectorForKey( const entity_t *ent, const char *key, vec3_t vec );
entity_t					*FindTargetEntity( const char *target );
void						GetEntityShadowFlags( const entity_t *ent, const entity_t *ent2, char *castShadows, char *recvShadows, qboolean isWorldspawn );
void						GetEntityLightmapScale( const entity_t *ent, float *lightmapScale, float baseScale );
void                        GetEntityLightmapAxis( const entity_t *ent, vec3_t lightmapAxis, vec3_t baseAxis );
void						GetEntityNormalSmoothing( const entity_t *ent, int *smoothNormals, int baseSmoothing );
void                        GetEntityMinlightAmbientColor( const entity_t *ent, vec3_t color, vec3_t minlight, vec3_t minvertexlight, vec3_t ambient, vec3_t colormod, qboolean setDefaults );
void                        GetEntityPatchMeta( const entity_t *ent, qboolean *patchMeta, float *patchQuality, float *patchSubdivisions, float baseQuality, float baseSubdivisions );

/* bspfile_ibsp.c */
void						LoadIBSPFile( const char *filename );
void						WriteIBSPFile( const char *filename );


/* bspfile_rbsp.c */
void						LoadRBSPFile( const char *filename );
void						WriteRBSPFile( const char *filename );



/* -------------------------------------------------------------------------------

bsp/general global variables

------------------------------------------------------------------------------- */

/* path support */
#define MAX_BASE_PATHS	10
#define MAX_GAME_PATHS	10
Q_EXTERN char					*homePath;
Q_EXTERN char					installPath[ MAX_OS_PATH ];
Q_EXTERN int					numBasePaths;
Q_EXTERN char					*basePaths[ MAX_BASE_PATHS ];
Q_EXTERN int					numGamePaths;
Q_EXTERN char					*gamePaths[ MAX_GAME_PATHS ];

/* game support */
Q_EXTERN game_t				games[]
#ifndef MAIN_C
							;
#else
							=
							{
								#include "quake3.h"
	,
								#include "quakelive.h" /* most be after game_quake3.h as they share defines! */
	,
								#include "nexuiz.h" /* most be after game_quake3.h as they share defines! */
	,
								#include "xonotic.h" /* most be after game_quake3.h as they share defines! */
	,
								#include "tremulous.h" /*LinuxManMikeC: must be after game_quake3.h, depends on #define's set in it */
	,
								#include "tenebrae.h"
	,
								#include "wolf.h"
	,
								#include "wolfet.h" /* most be after game_wolf.h as they share defines! */
	,
								#include "etut.h"
	,
								#include "ef.h"
	,
								#include "sof2.h"
	,
								#include "jk2.h"   /* most be after game_sof2.h as they share defines! */
	,
								#include "ja.h"    /* most be after game_jk2.h as they share defines! */
	,
								#include "qfusion.h"   /* qfusion game */
	,
								#include "reaction.h" /* must be after game_quake3.h */
	,
								#include "darkplaces.h"    /* vortex: darkplaces q1 engine */
	,
								#include "dq.h"    /* vortex: deluxe quake game ( darkplaces q1 engine) */
	,
								#include "prophecy.h"  /* vortex: prophecy game ( darkplaces q1 engine) */
	,
								#include "_null.h" /* null game (must be last item) */
	};
#endif
Q_EXTERN game_t				*game Q_ASSIGN( &games[ 0 ] );


/* general */
Q_EXTERN int				numImages Q_ASSIGN( 0 );
Q_EXTERN image_t			images[ MAX_IMAGES ];

Q_EXTERN int				numPicoModels Q_ASSIGN( 0 );
Q_EXTERN picoModel_t		*picoModels[ MAX_MODELS ];

Q_EXTERN shaderInfo_t		*shaderInfo Q_ASSIGN( NULL );
Q_EXTERN int				numShaderInfo Q_ASSIGN( 0 );
Q_EXTERN int				numVertexRemaps Q_ASSIGN( 0 );

Q_EXTERN surfaceParm_t		custSurfaceParms[ MAX_CUST_SURFACEPARMS ];
Q_EXTERN int				numCustSurfaceParms Q_ASSIGN( 0 );

Q_EXTERN char				mapName[ MAX_QPATH ];	/* ydnar: per-map custom shaders for larger lightmaps */
Q_EXTERN char				mapShaderFile[ MAX_OS_PATH ];
Q_EXTERN qboolean			warnImage Q_ASSIGN( qtrue );

/* ydnar: sinusoid samples */
Q_EXTERN float				jitters[ MAX_JITTERS ];


/* commandline arguments */
Q_EXTERN qboolean			verbose;
Q_EXTERN qboolean			gpu;
Q_EXTERN qboolean			verboseEntities Q_ASSIGN( qfalse );
Q_EXTERN qboolean			useBuildCfg Q_ASSIGN( qfalse );
Q_EXTERN qboolean			force Q_ASSIGN( qfalse );
Q_EXTERN qboolean			infoMode Q_ASSIGN( qfalse );
Q_EXTERN qboolean			useCustomInfoParms Q_ASSIGN( qfalse );
Q_EXTERN qboolean			useEntitySaveId Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noprune Q_ASSIGN( qfalse );
Q_EXTERN qboolean			leaktest Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nodetail Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nosubdivide Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noclipmodel Q_ASSIGN( qfalse ); /* vortex: disable misc_model "autoclip" spawnflag */
Q_EXTERN qboolean			noclip Q_ASSIGN( qfalse ); /* globally disable BSP clipping of brush faces */
Q_EXTERN qboolean			noTJunc Q_ASSIGN( qfalse );
Q_EXTERN qboolean			fulldetail Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nodetailcollision Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nofoliage Q_ASSIGN( qfalse ); /* vortex: don't load foliage file */
Q_EXTERN qboolean			nodecals Q_ASSIGN( qfalse ); /* vortex: don't process decals */
Q_EXTERN qboolean			nowater Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noCurveBrushes Q_ASSIGN( qfalse );
Q_EXTERN qboolean			fakemap Q_ASSIGN( qfalse );
Q_EXTERN qboolean			coplanar Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nofog Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noHint Q_ASSIGN( qfalse );				/* ydnar */
Q_EXTERN qboolean			renameModelShaders Q_ASSIGN( qfalse );	/* ydnar */
Q_EXTERN qboolean			skyFixHack Q_ASSIGN( qfalse );			/* ydnar */
Q_EXTERN qboolean			drawBSP Q_ASSIGN(qfalse);				/* UQ1 */
Q_EXTERN qboolean			randomizeBSP Q_ASSIGN(qfalse);			/* UQ1 */

Q_EXTERN float				patchSubdivisions Q_ASSIGN( 8.0f );		/* ydnar: -patchmeta subdivisions */

Q_EXTERN int				maxLMSurfaceVerts Q_ASSIGN( 64 );		/* ydnar */
Q_EXTERN int				maxSurfaceVerts Q_ASSIGN( 999 );		/* ydnar */
Q_EXTERN int				maxSurfaceIndexes Q_ASSIGN( 6000 );		/* ydnar */
Q_EXTERN float				npDegrees Q_ASSIGN( 0.0f );				/* ydnar: nonplanar degrees */
Q_EXTERN int				bevelSnap Q_ASSIGN( 0 );				/* ydnar: bevel plane snap */
Q_EXTERN int				texRange Q_ASSIGN( 0 );
Q_EXTERN qboolean			flat Q_ASSIGN( qfalse );
Q_EXTERN qboolean			meta Q_ASSIGN( qfalse );
Q_EXTERN qboolean			patchMeta Q_ASSIGN( qfalse );
Q_EXTERN qboolean			emitFlares Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugSurfaces Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugInset Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugPortals Q_ASSIGN( qfalse );

Q_EXTERN double				normalEpsilon Q_ASSIGN( 0.00001 );
Q_EXTERN double				distanceEpsilon Q_ASSIGN( 0.01 );

/* bsp */
Q_EXTERN int				numMapEntities Q_ASSIGN( 0 );

Q_EXTERN int				blockSize[ 3 ]					/* should be the same as in radiant */
#ifndef MAIN_C
							;
#else
							= { 1024, 1024, 1024 };
#endif

Q_EXTERN char				name[ MAX_OS_PATH ];
Q_EXTERN char				source[ MAX_OS_PATH ];
Q_EXTERN char				outbase[ 32 ];

Q_EXTERN float				sampleScale;					/* vortex: lightmap sample scale (ie quality)*/

Q_EXTERN int				mapEntityNum Q_ASSIGN( 0 );

Q_EXTERN int				entitySourceBrushes;

//Q_EXTERN plane_t			mapplanes[ MAX_MAP_PLANES ];	/* mapplanes[ num ^ 1 ] will always be the mirror or mapplanes[ num ] */
Q_EXTERN plane_t			*mapplanes;	/* mapplanes[ num ^ 1 ] will always be the mirror or mapplanes[ num ] */
Q_EXTERN int				nummapplanes;					/* nummapplanes will always be even */
Q_EXTERN int				allocatedmapplanes Q_ASSIGN(0);
Q_EXTERN int				numMapPatches;
Q_EXTERN vec3_t				mapMins, mapMaxs;
Q_EXTERN vec3_t				mapRegionMins, mapRegionMaxs;
Q_EXTERN vec3_t				mapPlayableMins, mapPlayableMaxs;
Q_EXTERN qboolean			mapRegion Q_ASSIGN( qfalse );
Q_EXTERN qboolean			mapRegionBrushes Q_ASSIGN( qtrue );
Q_EXTERN int				defaultFogNum Q_ASSIGN( -1 );	/* ydnar: cleaner fog handling */
Q_EXTERN int				numMapFogs Q_ASSIGN( 0 );
Q_EXTERN fog_t				mapFogs[ MAX_MAP_FOGS ];

Q_EXTERN entity_t			*mapEnt;
Q_EXTERN brush_t			*buildBrush;
Q_EXTERN int				numActiveBrushes;
Q_EXTERN int				g_bBrushPrimit;

Q_EXTERN int				numStrippedLights Q_ASSIGN( 0 );


/* surface stuff */
Q_EXTERN mapDrawSurface_t	*mapDrawSurfs Q_ASSIGN( NULL );
Q_EXTERN int				numMapDrawSurfs;

Q_EXTERN int				numSurfacesByType[ NUM_SURFACE_TYPES ];
Q_EXTERN int				numClearedSurfaces;
Q_EXTERN int				numStripSurfaces;
Q_EXTERN int				numFanSurfaces;
Q_EXTERN int				numMergedSurfaces;
Q_EXTERN int				numMergedVerts;

Q_EXTERN int				numRedundantIndexes;

Q_EXTERN int				numSurfaceModels Q_ASSIGN( 0 );

Q_EXTERN byte				debugColors[ 12 ][ 3 ]
#ifndef MAIN_C
							;
#else
							=
							{
								{ 255, 0, 0 },
								{ 192, 128, 128 },
								{ 255, 255, 0 },
								{ 192, 192, 128 },
								{ 0, 255, 255 },
								{ 128, 192, 192 },
								{ 0, 0, 255 },
								{ 128, 128, 192 },
								{ 255, 0, 255 },
								{ 192, 128, 192 },
								{ 0, 255, 0 },
								{ 128, 192, 128 }
							};
#endif

Q_EXTERN qboolean			skyboxPresent Q_ASSIGN( qfalse );
Q_EXTERN int				skyboxArea Q_ASSIGN( -1 );
Q_EXTERN m4x4_t				skyboxTransform;


/* -------------------------------------------------------------------------------

vis global variables

------------------------------------------------------------------------------- */

/* commandline arguments */
Q_EXTERN qboolean			fastvis;
Q_EXTERN qboolean			noPassageVis;
Q_EXTERN qboolean			passageVisOnly;
Q_EXTERN qboolean			mergevis;
Q_EXTERN qboolean			nosort;
Q_EXTERN qboolean			saveprt;
Q_EXTERN qboolean			hint;	/* ydnar */
Q_EXTERN char				inbase[ MAX_QPATH ];

/* other bits */
Q_EXTERN uint32_t/*int*/				totalvis;

Q_EXTERN float				farPlaneDist;	/* rr2do2, rf, mre, ydnar all contributed to this one... */

Q_EXTERN uint32_t/*int*/				numportals;
Q_EXTERN uint32_t/*int*/				portalclusters;

Q_EXTERN vportal_t			*portals;
Q_EXTERN leaf_t				*leafs;

Q_EXTERN vportal_t			*faces;
Q_EXTERN leaf_t				*faceleafs;

Q_EXTERN uint32_t/*int*/				numfaces;

Q_EXTERN uint32_t/*int*/				c_portaltest, c_portalpass, c_portalcheck;
Q_EXTERN uint32_t/*int*/				c_portalskip, c_leafskip;
Q_EXTERN uint32_t/*int*/				c_vistest, c_mighttest;
Q_EXTERN uint32_t/*int*/				c_chains;

Q_EXTERN byte				*vismap, *vismap_p, *vismap_end;

Q_EXTERN uint32_t/*int*/				testlevel;

Q_EXTERN byte				*uncompressed;

Q_EXTERN uint32_t/*int*/				leafbytes, leaflongs;
Q_EXTERN uint32_t/*int*/				portalbytes, portallongs;

Q_EXTERN vportal_t			*sorted_portals[ MAX_MAP_PORTALS * 2 ];



/* -------------------------------------------------------------------------------

light global variables

------------------------------------------------------------------------------- */

/* commandline arguments */
Q_EXTERN qboolean			wolfLight Q_ASSIGN( qfalse );
Q_EXTERN qboolean			loMem Q_ASSIGN( qfalse );
Q_EXTERN qboolean			loMemSky Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noStyles Q_ASSIGN( qfalse );
Q_EXTERN qboolean			keepLights Q_ASSIGN( qfalse );
Q_EXTERN qboolean			colorNormalize Q_ASSIGN( qfalse );

Q_EXTERN float				sampleSize Q_ASSIGN( DEFAULT_LIGHTMAP_SAMPLE_SIZE );
Q_EXTERN qboolean			noVertexLighting Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noGridLighting Q_ASSIGN( qfalse );

Q_EXTERN qboolean			noTrace Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noTraceGrid Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noSurfaces Q_ASSIGN( qfalse );
Q_EXTERN qboolean			noStitch Q_ASSIGN( qfalse );
Q_EXTERN qboolean			patchShadows Q_ASSIGN( qfalse );
Q_EXTERN qboolean			cpmaHack Q_ASSIGN( qfalse );
Q_EXTERN qboolean			stitch Q_ASSIGN( qfalse );

Q_EXTERN qboolean			deluxemap Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugDeluxemap Q_ASSIGN( qfalse );
Q_EXTERN int				deluxemode Q_ASSIGN( 0 );	/* deluxemap format (0 - modelspace, 1 - tangentspace with renormalization, 2 - tangentspace without renormalization) */

Q_EXTERN qboolean			fast Q_ASSIGN( qfalse );
Q_EXTERN qboolean			faster Q_ASSIGN( qfalse );
Q_EXTERN qboolean			fastgrid Q_ASSIGN( qfalse );
Q_EXTERN qboolean			fastbounce Q_ASSIGN( qfalse );
Q_EXTERN int				bounce Q_ASSIGN( 0 );
Q_EXTERN qboolean			bounceOnly Q_ASSIGN( qfalse );
Q_EXTERN qboolean			bouncing Q_ASSIGN( qfalse );
Q_EXTERN qboolean			bouncegrid Q_ASSIGN( qfalse );
Q_EXTERN qboolean			normalmap Q_ASSIGN( qfalse );
Q_EXTERN qboolean			trisoup Q_ASSIGN( qfalse );
Q_EXTERN qboolean			shade Q_ASSIGN( qfalse );
Q_EXTERN float				shadeAngleDegrees Q_ASSIGN( 0.0f );
Q_EXTERN int				superSample Q_ASSIGN( 0 );
Q_EXTERN int				gridSuperSample Q_ASSIGN( 0 );
Q_EXTERN qboolean           gridOnly Q_ASSIGN( qfalse );
Q_EXTERN qboolean           gridFromLightmap Q_ASSIGN( qfalse );
Q_EXTERN int				lightSamples Q_ASSIGN( 1 );
Q_EXTERN qboolean			filter Q_ASSIGN( qfalse );
Q_EXTERN qboolean			dark Q_ASSIGN( qfalse );
Q_EXTERN qboolean			sunOnly Q_ASSIGN( qfalse );
Q_EXTERN qboolean			skyLightSurfaces Q_ASSIGN( qfalse );
Q_EXTERN int				approximateTolerance Q_ASSIGN( 0 );
Q_EXTERN qboolean			noCollapse Q_ASSIGN( qfalse );
Q_EXTERN qboolean			exportLightmaps Q_ASSIGN( qfalse );
Q_EXTERN qboolean			externalLightmaps Q_ASSIGN( qfalse );
Q_EXTERN char				externalLightmapsPath[ MAX_OS_PATH ];

Q_EXTERN int				lmCustomSize Q_ASSIGN( LIGHTMAP_WIDTH );
Q_EXTERN int				lmMaxSurfaceSize Q_ASSIGN( 0 );
Q_EXTERN qboolean			lightmapsRGB Q_ASSIGN( qfalse );
Q_EXTERN qboolean           colorsRGB Q_ASSIGN( qfalse );
Q_EXTERN qboolean           texturesRGB Q_ASSIGN( qfalse );

/* vortex: dirtmapping/ambient occlusion */
typedef enum
{
	DIRTMODE_ORDERED_CONE,
	DIRTMODE_RANDOM_CONE,
	DIRTMODE_AMBIENT_SPHERE_LOWQUALITY,
	DIRTMODE_AMBIENT_SPHERE_HIGHQUALITY
}dirtMode_t;
char *DirtModeName(dirtMode_t *mode);
typedef enum
{
	DIRTFILTER_NONE,
	DIRTFILTER_AVERAGE
}dirtFilter_t;
char *DirtFilterName(dirtFilter_t *filter);
Q_EXTERN qboolean			dirty Q_ASSIGN( qfalse );
Q_EXTERN qboolean			nodirt Q_ASSIGN( qfalse );
Q_EXTERN qboolean			dirtDebug Q_ASSIGN( qfalse );
Q_EXTERN dirtMode_t			dirtMode Q_ASSIGN( DIRTMODE_ORDERED_CONE );
Q_EXTERN dirtFilter_t		dirtFilter Q_ASSIGN( DIRTFILTER_AVERAGE );
Q_EXTERN float				dirtDepth Q_ASSIGN( 128.0f );
Q_EXTERN float				dirtDepthExponent Q_ASSIGN( 2.0f );
Q_EXTERN float				dirtScale Q_ASSIGN( 1.0f );
Q_EXTERN float				dirtGain Q_ASSIGN( 1.0f );
Q_EXTERN vec3_t             dirtGainMask Q_ASSIGN_VEC3( 1, 1, 1 );
Q_EXTERN vec3_t             dirtScaleMask Q_ASSIGN_VEC3( 1, 1, 1 );
typedef struct
{
	qboolean       enabled;
	/* dirt vectors */
	vec3_t        *vectors;
	int            numVectors;
	float          wholeDepth;
	/* options */
	dirtMode_t     mode;
	float          depth;
	float          depthExponent;
	float          gain;
	vec3_t         gainMask;
	float          scale;
	vec3_t         scaleMask;
	dirtFilter_t   filter;
} dirtSettings_t;
Q_EXTERN dirtSettings_t dirtSettings[MAX_MAP_ENTITIES];

/* vortex: light deviance */
Q_EXTERN qboolean			noDeviance Q_ASSIGN( qfalse );
Q_EXTERN float			    devianceJitter Q_ASSIGN( 0.0f );
Q_EXTERN int			    devianceSamples Q_ASSIGN( 0 );
Q_EXTERN float			    devianceSamplesScale Q_ASSIGN( 1.0f );
Q_EXTERN int			    devianceAtten Q_ASSIGN( 0 );
Q_EXTERN int			    devianceForm Q_ASSIGN( 0 );

/* 27: floodlighting */
Q_EXTERN qboolean			debugnormals Q_ASSIGN( qfalse );
Q_EXTERN qboolean			floodlighty Q_ASSIGN( qfalse );
Q_EXTERN qboolean			floodlight_lowquality Q_ASSIGN( qfalse );
Q_EXTERN vec3_t				floodlightRGB;
Q_EXTERN float				floodlightIntensity Q_ASSIGN( 512.0f );
Q_EXTERN float				floodlightDistance Q_ASSIGN( 1024.0f );

Q_EXTERN qboolean			dump Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debug Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugUnused Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugAxis Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugCluster Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugOrigin Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugStitch Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugLightmap Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugRawLightmap Q_ASSIGN( qfalse );
Q_EXTERN qboolean			debugGrid Q_ASSIGN( qfalse );
Q_EXTERN qboolean			lightmapBorder Q_ASSIGN( qfalse );
Q_EXTERN qboolean			lightmapDebugState Q_ASSIGN( qfalse );

/* longest distance across the map */
Q_EXTERN float				maxMapDistance Q_ASSIGN( 0 );

/* for run time tweaking of light sources */
Q_EXTERN float				pointScale Q_ASSIGN( 7500.0f );
Q_EXTERN float				areaScale Q_ASSIGN( 0.25f );
Q_EXTERN float				skyScale Q_ASSIGN( 1.0f );
Q_EXTERN float				bounceScale Q_ASSIGN( 0.25f );
Q_EXTERN float				vertexScale Q_ASSIGN( 1.0f );

/* jal: alternative angle attenuation curve */
Q_EXTERN qboolean lightAngleHL Q_ASSIGN( qfalse );

/* ydnar: lightmap gamma/compensation */
Q_EXTERN float				lightmapGamma Q_ASSIGN( 1.0f );
Q_EXTERN double				lightmapInvGamma Q_ASSIGN( 1.0f );
Q_EXTERN float				lightmapCompensate Q_ASSIGN( 1.0f );
Q_EXTERN double				lightmapInvCompensate Q_ASSIGN( 1.0f );
Q_EXTERN float				lightmapExposure Q_ASSIGN( 1.0f );
Q_EXTERN double				lightmapInvExposure Q_ASSIGN( 1.0f );
Q_EXTERN float				lightmapBrightness Q_ASSIGN( 1.0f );
Q_EXTERN void (*ColorToBytes)(const float *, byte *, float, qboolean);
		 void ColorToBytesLinear( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesLinearExposure( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesLinearCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesLinearExposureCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesGamma( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesGammaExposure( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesGammaCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesGammaExposureCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB );
		 void ColorToBytesUnified( const float *color, byte *colorBytes, float scale, qboolean sRGB );

/* ydnar: for runtime tweaking of falloff tolerance */
Q_EXTERN float				falloffTolerance Q_ASSIGN( 0.1f ); /* vortex: was 1.0 */
Q_EXTERN qboolean			exactPointToPolygon Q_ASSIGN( qtrue );
Q_EXTERN float				formFactorValueScale Q_ASSIGN( 3.0f );
Q_EXTERN float				linearScale Q_ASSIGN( 1.0f / 8000.0f );

Q_EXTERN light_t			*lights;
Q_EXTERN int				numPointLights;
Q_EXTERN int				numSpotLights;
Q_EXTERN int				numSunLights;
Q_EXTERN int				numAreaLights;
Q_EXTERN int				numLights Q_ASSIGN( -1 );
Q_EXTERN int				numCulledLights;
Q_EXTERN int				numNegativeLights;

/* ydnar: for luxel placement */
Q_EXTERN int				numSurfaceClusters, maxSurfaceClusters;
Q_EXTERN int				*surfaceClusters;

/* ydnar: for radiosity */
Q_EXTERN int				numDiffuseLights;
Q_EXTERN int				numBrushDiffuseLights;
Q_EXTERN int				numTriangleDiffuseLights;
Q_EXTERN int				numPatchDiffuseLights;

/* ydnar: general purpose extra copy of drawvert list */
Q_EXTERN bspDrawVert_t		*yDrawVerts;

/* ydnar: for tracing statistics */
Q_EXTERN int				minSurfacesTested;
Q_EXTERN int				maxSurfacesTested;
Q_EXTERN int				totalSurfacesTested;
Q_EXTERN int				totalTraces;

Q_EXTERN FILE				*dumpFile;

Q_EXTERN int				c_visible, c_occluded;
Q_EXTERN int				c_subsampled;	/* ydnar */

Q_EXTERN int				defaultLightSubdivide Q_ASSIGN( 999 );

Q_EXTERN vec3_t				ambientColor;
Q_EXTERN vec3_t				minLight Q_ASSIGN_VEC3( 0, 0, 0 );
Q_EXTERN vec3_t             minVertexLight Q_ASSIGN_VEC3( 0, 0, 0 );
Q_EXTERN vec3_t             minGridLight Q_ASSIGN_VEC3( 0, 0, 0 );
Q_EXTERN vec3_t             colorMod Q_ASSIGN_VEC3( 1, 1, 1 );

Q_EXTERN int				*entitySurface;
Q_EXTERN vec3_t				*surfaceOrigin;

Q_EXTERN vec3_t				sunDirection;
Q_EXTERN vec3_t				sunLight;

/* tracing */
Q_EXTERN int				c_totalTrace;
Q_EXTERN int				c_cullTrace, c_testTrace;
Q_EXTERN int				c_testFacets;

/* ydnar: light optimization */
Q_EXTERN float				subdivideThreshold Q_ASSIGN( DEFAULT_SUBDIVIDE_THRESHOLD );

Q_EXTERN int				numOpaqueBrushes, maxOpaqueBrush;
Q_EXTERN byte				*opaqueBrushes;

Q_EXTERN int				lightsBoundsCulled;
Q_EXTERN int				lightsEnvelopeCulled;
Q_EXTERN int				lightsPlaneCulled;
Q_EXTERN int				lightsClusterCulled;

/* ydnar: radiosity */
Q_EXTERN float				diffuseSubdivide Q_ASSIGN( 256.0f );
Q_EXTERN float				minDiffuseSubdivide Q_ASSIGN( 64.0f );
Q_EXTERN int				numDiffuseSurfaces Q_ASSIGN( 0 );

/* ydnar: list of surface information necessary for lightmap calculation */
Q_EXTERN surfaceInfo_t		*surfaceInfos Q_ASSIGN( NULL );

/* ydnar: sorted list of surfaces */
Q_EXTERN int				*sortSurfaces Q_ASSIGN( NULL );

/* clumps of surfaces that share a raw lightmap */
Q_EXTERN int				numLightSurfaces Q_ASSIGN( 0 );
Q_EXTERN int				*lightSurfaces Q_ASSIGN( NULL );

/* raw lightmaps */
Q_EXTERN int				numRawSuperLuxels Q_ASSIGN( 0 );
Q_EXTERN int				numRawLightmaps Q_ASSIGN( 0 );
Q_EXTERN rawLightmap_t		*rawLightmaps Q_ASSIGN( NULL );
Q_EXTERN int				*sortLightmaps Q_ASSIGN( NULL );

/* vertex luxels */
Q_EXTERN float				*vertexLuxels[ MAX_LIGHTMAPS ];
Q_EXTERN float				*radVertexLuxels[ MAX_LIGHTMAPS ];

/* bsp lightmaps */
Q_EXTERN int				numLightmapShaders Q_ASSIGN( 0 );
Q_EXTERN int				numSolidLightmaps Q_ASSIGN( 0 );
Q_EXTERN int				numOutLightmaps Q_ASSIGN( 0 );
Q_EXTERN int				numBSPLightmaps Q_ASSIGN( 0 );
Q_EXTERN int				numExtLightmaps Q_ASSIGN( 0 );
Q_EXTERN outLightmap_t		*outLightmaps Q_ASSIGN( NULL );

/* vortex: per surface floodlight statictics */
Q_EXTERN int				numSurfacesFloodlighten Q_ASSIGN( 0 );

/* grid points */
Q_EXTERN int				numRawGridPoints Q_ASSIGN( 0 );
Q_EXTERN rawGridPoint_t		*rawGridPoints Q_ASSIGN( NULL );

Q_EXTERN int				numSurfsVertexLit Q_ASSIGN( 0 );
Q_EXTERN int				numSurfsVertexForced Q_ASSIGN( 0 );
Q_EXTERN int				numSurfsVertexApproximated Q_ASSIGN( 0 );
Q_EXTERN int				numSurfsLightmapped Q_ASSIGN( 0 );
Q_EXTERN int				numPlanarsLightmapped Q_ASSIGN( 0 );
Q_EXTERN int				numNonPlanarsLightmapped Q_ASSIGN( 0 );
Q_EXTERN int				numPatchesLightmapped Q_ASSIGN( 0 );
Q_EXTERN int				numPlanarPatchesLightmapped Q_ASSIGN( 0 );

/* luxels */
Q_EXTERN ThreadMutex        LightmapGrowStitchMutex;
Q_EXTERN int				numLuxels Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsMapped Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsNudged Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsRemapped Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsOccluded Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsIlluminated Q_ASSIGN( 0 );
Q_EXTERN int				numLuxelsStitched Q_ASSIGN( 0 );
Q_EXTERN int				numVertsIlluminated Q_ASSIGN( 0 );

/* vortex: lightgrid areas */
#define MAX_GRID_AREAS      1024
typedef struct gridArea_s
{
	vec3_t mins;
	vec3_t maxs;
}gridArea_t;
Q_EXTERN gridArea_t         gridAreas[MAX_GRID_AREAS];
Q_EXTERN int                numGridAreas Q_ASSIGN( 0 );

/* lightgrid */
Q_EXTERN vec3_t				gridMins;
Q_EXTERN int				gridBounds[ 3 ];
Q_EXTERN vec3_t				gridDefaultSize Q_ASSIGN_VEC3( 64, 64, 128 );
Q_EXTERN vec3_t				gridSize Q_ASSIGN_VEC3( 64, 64, 128 );

Q_EXTERN int                gridBlocks[3];
Q_EXTERN int                gridBlockSize[3];
Q_EXTERN int                numGridBlocks;

/* -------------------------------------------------------------------------------

convert global variables

------------------------------------------------------------------------------- */

/* convert_ase.c, used for mesh collapsing code */
Q_EXTERN int				*markDrawSurfaces;
Q_EXTERN int				numMarkDrawSurfaces;
Q_EXTERN bspDrawVert_t		*collapseDrawVerts;
Q_EXTERN int				collapseNumVerts;

/* -------------------------------------------------------------------------------

abstracted bsp globals

------------------------------------------------------------------------------- */

Q_EXTERN int				numEntities Q_ASSIGN( 0 );
Q_EXTERN int				numBSPEntities Q_ASSIGN( 0 );
Q_EXTERN entity_t			entities[ MAX_MAP_ENTITIES ];

Q_EXTERN int				numBSPModels Q_ASSIGN( 0 );
Q_EXTERN bspModel_t			bspModels[ MAX_MAP_MODELS ];

Q_EXTERN int				numBSPShaders Q_ASSIGN( 0 );
Q_EXTERN bspShader_t		bspShaders[ MAX_MAP_MODELS ];

Q_EXTERN int				bspEntDataSize Q_ASSIGN( 0 );
Q_EXTERN char				bspEntData[ MAX_MAP_ENTSTRING ];

Q_EXTERN int				numBSPLeafs Q_ASSIGN( 0 );
//Q_EXTERN bspLeaf_t			bspLeafs[ MAX_MAP_LEAFS ];
Q_EXTERN bspLeaf_t			*bspLeafs;

Q_EXTERN int				numBSPPlanes Q_ASSIGN( 0 );
//Q_EXTERN bspPlane_t			bspPlanes[ MAX_MAP_PLANES ];
Q_EXTERN bspPlane_t			*bspPlanes;

Q_EXTERN int				numBSPNodes Q_ASSIGN( 0 );
//Q_EXTERN bspNode_t			bspNodes[ MAX_MAP_NODES ];
Q_EXTERN bspNode_t			*bspNodes;

Q_EXTERN int				numBSPLeafSurfaces Q_ASSIGN( 0 );
//Q_EXTERN int				bspLeafSurfaces[ MAX_MAP_LEAFFACES ];
Q_EXTERN int				*bspLeafSurfaces;

Q_EXTERN int				numBSPLeafBrushes Q_ASSIGN( 0 );
//Q_EXTERN int				bspLeafBrushes[ MAX_MAP_LEAFBRUSHES ];
Q_EXTERN int				*bspLeafBrushes;

Q_EXTERN int				numBSPBrushes Q_ASSIGN( 0 );
//Q_EXTERN bspBrush_t			bspBrushes[ MAX_MAP_BRUSHES ];
Q_EXTERN bspBrush_t			*bspBrushes;

Q_EXTERN int				numBSPBrushSides Q_ASSIGN( 0 );
//Q_EXTERN bspBrushSide_t		bspBrushSides[ MAX_MAP_BRUSHSIDES ];
Q_EXTERN bspBrushSide_t		*bspBrushSides;

Q_EXTERN int				numBSPLightBytes Q_ASSIGN( 0 );
Q_EXTERN byte				*bspLightBytes Q_ASSIGN( NULL );

Q_EXTERN int				numBSPGridPoints Q_ASSIGN( 0 );
Q_EXTERN bspGridPoint_t		*bspGridPoints Q_ASSIGN( NULL );

Q_EXTERN uint32_t			numBSPVisBytes Q_ASSIGN( 0 );
Q_EXTERN byte				bspVisBytes[ MAX_MAP_VISIBILITY ];
//Q_EXTERN byte				*bspVisBytes;

Q_EXTERN int				numBSPDrawVerts Q_ASSIGN( 0 );
Q_EXTERN bspDrawVert_t		*bspDrawVerts Q_ASSIGN( NULL );

Q_EXTERN int				numBSPDrawIndexes Q_ASSIGN( 0 );
Q_EXTERN int				bspDrawIndexes[ MAX_MAP_DRAW_INDEXES ];

Q_EXTERN int				numBSPDrawSurfaces Q_ASSIGN( 0 );
Q_EXTERN bspDrawSurface_t	*bspDrawSurfaces Q_ASSIGN( NULL );

Q_EXTERN int				numBSPFogs Q_ASSIGN( 0 );
Q_EXTERN bspFog_t			bspFogs[ MAX_MAP_FOGS ];

// sRGB colorspace
#define linear_to_srgb(c) (((c) < 0.0031308) ? (c) * 12.92 : 1.055 * pow((c), 1.0/2.4) - 0.055)
#define srgb_to_linear(c) (((c) <= 0.04045) ? (c) * (1.0 / 12.92) : pow(((c) + 0.055f)*(1.0/1.055), 2.4))




/*OpenCL Helper Functions to keep the clutter out of q3map2's source*/
qboolean InitOpenCL( void );
qboolean GetDevice( void );
qboolean CreateContext( void );
qboolean CreateCommandQueue( void );

void CleanOpenCL( void ); /*clean up memory and release ocl objects*/

qboolean LoadProgramSource(char *filename); /*Load a .cl file to a single string. Return value stored in kernels[]*/
void BuildAllProgramSource( void ); /*builds all loaded .cl files in to executable code*/

void PushKernel(char *kernel, unsigned int *length); /*push a kernel on to the array*/
void do_stuff();


void VertexCacheOptimizeMeshIndexes(uint32_t numVerts, uint32_t numIndexes, uint32_t *indexes, float *stats);

/* end marker */
#endif
