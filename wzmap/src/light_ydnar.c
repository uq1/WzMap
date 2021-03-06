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

/* marker */
#define LIGHT_YDNAR_C

/* dependencies */
#include "q3map2.h"

/*
ColorToBytesLinear()
ColorToBytesLinearExposure
ColorToBytesLinearCompensate
ColorToBytesLinearExposureCompensate
ColorToBytesGamma
ColorToBytesGammaExposure
ColorToBytesGammaCompensate
ColorToBytesGammaExposureCompensate
ColorToBytesUnified
*/

#define _ColorToBytesStart	double r, g, b, dif, m; \
							r = color[ 0 ] * scale * lightmapBrightness; \
							g = color[ 1 ] * scale * lightmapBrightness; \
							b = color[ 2 ] * scale * lightmapBrightness;

#define _ColorToBytesGamma	if( r < 0 ) r = 0; else r = pow( r / 255.0f, lightmapInvGamma ) * 255.0f; \
								if( g < 0 ) g = 0; else g = pow( g / 255.0f, lightmapInvGamma ) * 255.0f; \
								if( b < 0 ) b = 0; else b = pow( b / 255.0f, lightmapInvGamma ) * 255.0f;

#define _ColorToBytesClampWithNormalization	m = max(r, g); \
											m = max(m, b); \
											if( m > 255 ) \
											{ \
												dif = 255.0f / m; \
												r *= dif;\
												g *= dif; \
												b *= dif; \
											}

#define _ColorToBytesExposure		m = max(r, g); \
									m = max(m, b); \
										dif = (1 - exp(-m * lightmapInvExposure) ) * 255.0f; \
										if (m > 0) \
											dif = dif / m; \
										else \
											dif = 0; \
										r *= dif; \
										g *= dif; \
										b *= dif;

#define _ColorToBytesCompensate	r *= lightmapInvCompensate; \
								g *= lightmapInvCompensate; \
								b *= lightmapInvCompensate;

#define _ColorToBytesStore  if (sRGB) \
							{ \
								r *= ( 1.0f / 255.0f ); \
								g *= ( 1.0f / 255.0f ); \
								b *= ( 1.0f / 255.0f ); \
								r = floor( linear_to_srgb( r ) * 255 + 0.5 ) ; \
								g = floor( linear_to_srgb( g ) * 255 + 0.5 ) ; \
								b = floor( linear_to_srgb( b ) * 255 + 0.5 ) ; \
								colorBytes[ 0 ] = r; \
								colorBytes[ 1 ] = g; \
								colorBytes[ 2 ] = b; \
								return; \
							} \
							colorBytes[ 0 ] = r; \
							colorBytes[ 1 ] = g; \
							colorBytes[ 2 ] = b;

// default
void ColorToBytesLinear( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesClampWithNormalization
	_ColorToBytesStore
}

// gamma
void ColorToBytesGamma( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesGamma
	_ColorToBytesClampWithNormalization
	_ColorToBytesStore
}

// exposure
void ColorToBytesLinearExposure( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesExposure
	_ColorToBytesStore
}

// gamma + exposure
void ColorToBytesGammaExposure( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesGamma
	_ColorToBytesExposure
	_ColorToBytesStore
}

// compensate
void ColorToBytesLinearCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesClampWithNormalization
	_ColorToBytesCompensate
	_ColorToBytesStore
}

// gamma + compensate
void ColorToBytesGammaCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesGamma
	_ColorToBytesClampWithNormalization
	_ColorToBytesCompensate
	_ColorToBytesStore
}

// exposure + compensate
void ColorToBytesLinearExposureCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesExposure
	_ColorToBytesCompensate
	_ColorToBytesStore
}

// gamma + exposure + compensate
void ColorToBytesGammaExposureCompensate( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart
	_ColorToBytesGamma
	_ColorToBytesExposure
	_ColorToBytesCompensate
	_ColorToBytesStore
}

// unified
void ColorToBytesUnified( const float *color, byte *colorBytes, float scale, qboolean sRGB )
{
	_ColorToBytesStart

	/* gamma */
	_ColorToBytesGamma
	
	/* clamp with color normalization */
	if (lightmapExposure == 1)
	{
		_ColorToBytesClampWithNormalization
	}
	else
	{
		_ColorToBytesExposure
	}

	/* compensate for ingame overbrighting/bitshifting */
	if (lightmapInvCompensate != 1)
	{
		_ColorToBytesCompensate
	}

	/* store in RGB / sRGB */
	_ColorToBytesStore
}

/* -------------------------------------------------------------------------------

this section deals with phong shading (normal interpolation across brush faces)

------------------------------------------------------------------------------- */

/*
SmoothNormals()
smooths together coincident vertex normals across the bsp
*/

#define MAX_SAMPLES				256
#define THETA_EPSILON			0.000001
#define EQUAL_NORMAL_EPSILON	0.01

void SmoothNormals( void )
{
	int					i, j, k, f, cs, numVerts, numVotes, fOld, start;
	float				shadeAngle, defaultShadeAngle, maxShadeAngle, dot, testAngle;
	bspDrawSurface_t	*ds;
	shaderInfo_t		*si;
	float				*shadeAngles;
	byte				*smoothed;
	vec3_t				average, diff;
	int					indexes[ MAX_SAMPLES ];
	vec3_t				votes[ MAX_SAMPLES ];
	
	
	/* allocate shade angle table */
	shadeAngles = (float *)safe_malloc( numBSPDrawVerts * sizeof( float ) );
	memset( shadeAngles, 0, numBSPDrawVerts * sizeof( float ) );
	
	/* allocate smoothed table */
	cs = (numBSPDrawVerts / 8) + 1;
	smoothed = (byte *)safe_malloc( cs );
	memset( smoothed, 0, cs );
	
	/* set default shade angle */
	defaultShadeAngle = DEG2RAD( shadeAngleDegrees );
	maxShadeAngle = 0;
	
	/* run through every surface and flag verts belonging to non-lightmapped surfaces
	   and set per-vertex smoothing angle */
	for( i = 0; i < numBSPDrawSurfaces; i++ )
	{
		printLabelledProgress("NonLightmapFlags", i, numBSPDrawSurfaces);

		/* get drawsurf */
		ds = &bspDrawSurfaces[ i ];
		si = surfaceInfos[ i ].si;

		/* shader-set normal smoothing */
		shadeAngle = 0.0f;
		if (si->shadeAngleDegrees)
			shadeAngle = DEG2RAD( si->shadeAngleDegrees );

		/* default normal smoothing */
		if (shadeAngle <= 0.0f)
			shadeAngle = defaultShadeAngle;
		if (shadeAngle > maxShadeAngle)
			maxShadeAngle = shadeAngle;
		
		/* flag its verts */
		for( j = 0; j < ds->numVerts; j++ )
		{
			f = ds->firstVert + j;
			shadeAngles[ f ] = shadeAngle;
			if( ds->surfaceType == MST_TRIANGLE_SOUP )
				smoothed[ f >> 3 ] |= (1 << (f & 7));
		}
		
		/* ydnar: optional force-to-trisoup */
		if( trisoup && ds->surfaceType == MST_PLANAR )
		{
			ds->surfaceType = MST_TRIANGLE_SOUP;
			ds->lightmapNum[ 0 ] = -3;
		}
	}
	
	/* bail if no surfaces have a shade angle */
	if( maxShadeAngle <= 0 )
	{
		free( shadeAngles );
		free( smoothed );
		return;
	}
	
	/* init pacifier */
	fOld = -1;
	start = I_FloatTime();
	
	/* go through the list of vertexes */
	for( i = 0; i < numBSPDrawVerts; i++ )
	{
		printLabelledProgress("SmoothNormals", i, numBSPDrawSurfaces);

		/* already smoothed? */
		if( smoothed[ i >> 3 ] & (1 << (i & 7)) )
			continue;
		
		/* clear */
		VectorClear( average );
		numVerts = 0;
		numVotes = 0;
		
		/* build a table of coincident vertexes */
		for( j = i; j < numBSPDrawVerts && numVerts < MAX_SAMPLES; j++ )
		{
			/* already smoothed? */
			if( smoothed[ j >> 3 ] & (1 << (j & 7)) )
				continue;
			
			/* test vertexes */
			/* vortex: added normal smoothing epsilon */
			if( VectorCompareExt( yDrawVerts[ i ].xyz, yDrawVerts[ j ].xyz, 0.05 ) == qfalse )
				continue;
			
			/* use smallest shade angle */
			shadeAngle = (shadeAngles[ i ] < shadeAngles[ j ] ? shadeAngles[ i ] : shadeAngles[ j ]);
			
			/* check shade angle */
			dot = DotProduct( bspDrawVerts[ i ].normal, bspDrawVerts[ j ].normal );
			if( dot > 1.0 )
				dot = 1.0;
			else if( dot < -1.0 )
				dot = -1.0;
			testAngle = acos( dot ) + THETA_EPSILON;
			if( testAngle >= shadeAngle )
			{
				//Sys_Printf( "F(%3.3f >= %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );
				continue;
			}
			//Sys_Printf( "P(%3.3f < %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );
			
			/* add to the list */
			indexes[ numVerts++ ] = j;
			
			/* flag vertex */
			smoothed[ j >> 3 ] |= (1 << (j & 7));
			
			/* see if this normal has already been voted */
			for( k = 0; k < numVotes; k++ )
			{
				VectorSubtract( bspDrawVerts[ j ].normal, votes[ k ], diff );
				if( fabs( diff[ 0 ] ) < EQUAL_NORMAL_EPSILON &&
					fabs( diff[ 1 ] ) < EQUAL_NORMAL_EPSILON &&
					fabs( diff[ 2 ] ) < EQUAL_NORMAL_EPSILON )
					break;
			}
			
			/* add a new vote? */
			if( k == numVotes && numVotes < MAX_SAMPLES )
			{
				VectorAdd( average, bspDrawVerts[ j ].normal, average );
				VectorCopy( bspDrawVerts[ j ].normal, votes[ numVotes ] );
				numVotes++;
			}
		}
		
		/* don't average for less than 2 verts */
		if( numVerts < 2 )
			continue;
		
		/* average normal */
		if( VectorNormalize( average, average ) > 0 )
		{
			/* smooth */
			for( j = 0; j < numVerts; j++ )
				VectorCopy( average, yDrawVerts[ indexes[ j ] ].normal );
		}
	}
	
	/* free the tables */
	free( shadeAngles );
	free( smoothed );
	
	/* print time */
	//Sys_Printf( " (%i)\n", (int) (I_FloatTime() - start) );
}



/* -------------------------------------------------------------------------------

this section deals with phong shaded lightmap tracing

------------------------------------------------------------------------------- */

/* 9th rewrite (recursive subdivision of a lightmap triangle) */

/*
CalcTangentVectors()
calculates the st tangent vectors for normalmapping
*/

static qboolean CalcTangentVectors( int numVerts, bspDrawVert_t **dv, vec3_t *stv, vec3_t *ttv )
{
	int			i;
	float		bb, s, t;
	vec3_t		bary;
	
	
	/* calculate barycentric basis for the triangle */
	bb = (dv[ 1 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ]) * (dv[ 2 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ]) - (dv[ 2 ]->st[ 0 ] - dv[ 0 ]->st[ 0 ]) * (dv[ 1 ]->st[ 1 ] - dv[ 0 ]->st[ 1 ]);
	if( fabs( bb ) < 0.00000001f )
		return qfalse;
	
	/* do each vertex */
	for( i = 0; i < numVerts; i++ )
	{
		/* calculate s tangent vector */
		s = dv[ i ]->st[ 0 ] + 10.0f;
		t = dv[ i ]->st[ 1 ];
		bary[ 0 ] = ((dv[ 1 ]->st[ 0 ] - s) * (dv[ 2 ]->st[ 1 ] - t) - (dv[ 2 ]->st[ 0 ] - s) * (dv[ 1 ]->st[ 1 ] - t)) / bb;
		bary[ 1 ] = ((dv[ 2 ]->st[ 0 ] - s) * (dv[ 0 ]->st[ 1 ] - t) - (dv[ 0 ]->st[ 0 ] - s) * (dv[ 2 ]->st[ 1 ] - t)) / bb;
		bary[ 2 ] = ((dv[ 0 ]->st[ 0 ] - s) * (dv[ 1 ]->st[ 1 ] - t) - (dv[ 1 ]->st[ 0 ] - s) * (dv[ 0 ]->st[ 1 ] - t)) / bb;
		
		stv[ i ][ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		stv[ i ][ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		stv[ i ][ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];
		
		VectorSubtract( stv[ i ], dv[ i ]->xyz, stv[ i ] );
		VectorNormalize( stv[ i ], stv[ i ] );
		
		/* calculate t tangent vector */
		s = dv[ i ]->st[ 0 ];
		t = dv[ i ]->st[ 1 ] + 10.0f;
		bary[ 0 ] = ((dv[ 1 ]->st[ 0 ] - s) * (dv[ 2 ]->st[ 1 ] - t) - (dv[ 2 ]->st[ 0 ] - s) * (dv[ 1 ]->st[ 1 ] - t)) / bb;
		bary[ 1 ] = ((dv[ 2 ]->st[ 0 ] - s) * (dv[ 0 ]->st[ 1 ] - t) - (dv[ 0 ]->st[ 0 ] - s) * (dv[ 2 ]->st[ 1 ] - t)) / bb;
		bary[ 2 ] = ((dv[ 0 ]->st[ 0 ] - s) * (dv[ 1 ]->st[ 1 ] - t) - (dv[ 1 ]->st[ 0 ] - s) * (dv[ 0 ]->st[ 1 ] - t)) / bb;
		
		ttv[ i ][ 0 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 0 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 0 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 0 ];
		ttv[ i ][ 1 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 1 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 1 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 1 ];
		ttv[ i ][ 2 ] = bary[ 0 ] * dv[ 0 ]->xyz[ 2 ] + bary[ 1 ] * dv[ 1 ]->xyz[ 2 ] + bary[ 2 ] * dv[ 2 ]->xyz[ 2 ];
		
		VectorSubtract( ttv[ i ], dv[ i ]->xyz, ttv[ i ] );
		VectorNormalize( ttv[ i ], ttv[ i ] );
		
		/* debug code */
		//%	Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%		stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}
	
	/* return to caller */
	return qtrue;
}




/*
PerturbNormal()
perterbs the normal by the shader's normalmap in tangent space
*/

static void PerturbNormal( bspDrawVert_t *dv, shaderInfo_t *si, vec3_t pNormal, vec3_t stv[ 3 ], vec3_t ttv[ 3 ] )
{
	int			i;
	vec4_t		bump;
	
	
	/* passthrough */
	VectorCopy( dv->normal, pNormal );
	
	/* sample normalmap */
	if( RadSampleImage( si->normalImage->pixels, si->normalImage->width, si->normalImage->height, dv->st, bump ) == qfalse )
		return;
	
	/* remap sampled normal from [0,255] to [-1,-1] */
	for( i = 0; i < 3; i++ )
		bump[ i ] = (bump[ i ] - 127.0f) * (1.0f / 127.5f);
	
	/* scale tangent vectors and add to original normal */
	VectorMA( dv->normal, bump[ 0 ], stv[ 0 ], pNormal );
	VectorMA( pNormal, bump[ 1 ], ttv[ 0 ], pNormal );
	VectorMA( pNormal, bump[ 2 ], dv->normal, pNormal );
	
	/* renormalize and return */
	VectorNormalize( pNormal, pNormal );
}



/*
MapSingleLuxel()
maps a luxel for triangle bv at
*/

#define NUDGE

static int MapSingleLuxel( rawLightmap_t *lm, surfaceInfo_t *info, bspDrawVert_t *dv, vec4_t plane, vec4_t triplane, float pass, vec3_t stv[ 3 ], vec3_t ttv[ 3 ], vec3_t worldverts[ 3 ] )
{
	int				i, j, x, y, numClusters, *clusters, pointCluster, *cluster, next, numnudges;
	float			*luxel, *origin, *normal, *triorigin, *trinormal, d, d2, lightmapSampleOffset, e, *nudge;
	vec3_t			pNormal, temp, vecs[ 3 ], cverts[ 3 ], nudged;
	vec4_t			sideplane, hostplane;
	shaderInfo_t	*si;
	qboolean        remap;
	static float	nudges[][ 3 ] =
					{
						{ -1.0f,  0.0f,  0.0f }, /* left */
						{  1.0f,  0.0f,  0.0f }, /* right */
						{  0.0f,  1.0f,  0.0f }, /* up */
						{  0.0f, -1.0f,  0.0f }, /* down */
						{ -1.0f,  1.0f,  0.0f }, /* left/up */
						{  1.0f, -1.0f,  0.0f }, /* right/down */
						{  1.0f,  1.0f,  0.0f }, /* right/up */
						{ -1.0f, -1.0f,  0.0f }, /* left/down */
						{  0.0f,  0.0f,  1.0f }, /* top 1x */
						{  0.0f,  0.0f,  2.0f }, /* top 2x */
						{  0.0f,  0.0f,  4.0f }, /* top 4x */
						{  0.0f,  0.0f, -1.0f }, /* bottom 1x */
						{  0.0f,  0.0f, -2.0f }, /* bottom 2x */
						{ -999999.0f, -999999.0f }
					};

	/* find luxel xy coords (fixme: subtract 0.5?) */
	x = dv->lightmap[ 0 ][ 0 ];
	y = dv->lightmap[ 0 ][ 1 ];
	if( x < 0 )
		x = 0;
	else if( x >= lm->sw )
		x = lm->sw - 1;
	if( y < 0 )
		y = 0;
	else if( y >= lm->sh )
		y = lm->sh - 1;
	
	/* set shader and cluster list */
	if( info != NULL )
	{
		si = info->si;
		numClusters = info->numSurfaceClusters;
		clusters = &surfaceClusters[ info->firstSurfaceCluster ];
	}
	else
	{
		si = NULL;
		numClusters = 0;
		clusters = NULL;
	}
	
	/* get luxel, origin, cluster, and normal */
	luxel = SUPER_LUXEL( 0, x, y );
	origin = SUPER_ORIGIN( x, y );
	normal = SUPER_NORMAL( x, y );
	cluster = SUPER_CLUSTER( x, y );
	triorigin = SUPER_TRIORIGIN( x, y );
	trinormal = SUPER_TRINORMAL( x, y );
	
	/* don't attempt to remap occluded luxels for planar surfaces */
	/* vortex: why is that? disabled for now */
	//if( (*cluster) == CLUSTER_OCCLUDED && lm->plane != NULL )
	//	return (*cluster);
	
	/* get origin */
	VectorClear( temp );
	if( lm->vecs != NULL )
	{
		/* axial lightmap projection */
		/* calculate an origin for the sample from the lightmap vectors */
		VectorCopy( lm->origin, temp );
		for( i = 0; i < 3; i++ )
		{
			/* add unless it's the axis, which is taken care of later */
			if( i == lm->axisNum )
				continue;
			temp[ i ] += (x * lm->vecs[ 0 ][ i ]) + (y * lm->vecs[ 1 ][ i ]);
		}
		
		/* project the origin onto the plane */
		d = DotProduct( temp, plane ) - plane[ 3 ];
		d /= plane[ lm->axisNum ];
		temp[ lm->axisNum ] -= d;
	}
	else
	{
		/* non axial lightmap projection (explicit xyz) */
		VectorCopy( dv->xyz, temp );
	}

	/* calculate nudge/projection axis */
	if( lm->plane != NULL )
	{
		/* planar surfaces have precalculated lightmap vectors for nudging */
		VectorCopy( lm->vecs[ 0 ], vecs[ 0 ] );
		VectorCopy( lm->vecs[ 1 ], vecs[ 1 ] );
		VectorCopy( lm->plane, vecs[ 2 ] );
	}
	else
	{
		/* non-planar surfaces must calculate them */
		if( plane != NULL )
			VectorCopy( plane, vecs[ 2 ] );
		else
			VectorCopy( dv->normal, vecs[ 2 ] );
		MakeNormalVectors( vecs[ 2 ], vecs[ 0 ], vecs[ 1 ] );
	}

	/* vortex: check if this luxel has been mapped (overlapping surfaces case) */
	remap = qfalse;
	if( trinormal[ 0 ] || trinormal[ 1 ] || trinormal[ 2 ] )
	{
		/* always remap bad cluster luxels */
		if( *cluster > 0 )
		{
			d = DotProduct( vecs[ 2 ], triorigin );
			d2 = DotProduct( vecs[ 2 ], temp );

			/* choose the most 'outer' luxel */
			if( d >= d2 )
			{
				/* do bumpmap calculations */
				if( stv )
					PerturbNormal( dv, si, pNormal, stv, ttv );
				else
					VectorCopy( dv->normal, pNormal );

				/* only average the normal for premapped luxels */
				VectorAdd( normal, pNormal, normal );
				luxel[ 3 ] += 1.0f;

				/* skip (do not remap) */
				return (*cluster);
			}
		}

		/* remap */
		remap = qtrue;
		numLuxelsRemapped++;
	}

	/* vortex: store sampled origin and triangle normal */
	VectorCopy( temp, origin );
	VectorCopy( temp, triorigin );
	VectorNormalize( triplane, trinormal );

	/* 27's test to make sure samples stay within the triangle boundaries
	 1) Test the sample origin to see if it lays on the wrong side of any edge (x/y)
	 2) if it does, nudge it onto the correct side.
	*/
	if( worldverts != NULL )
	{
		for( j = 0; j < 3; j++)
			VectorCopy( worldverts[j], cverts[j] );    
		PlaneFromPoints( hostplane, cverts[0], cverts[1], cverts[2] );
		for( j = 0; j < 3; j++)
		{
			for( i = 0; i < 3; i++)
			{
				/* build plane using 2 edges and a normal */
				next = ( i + 1 ) % 3;
				VectorCopy( cverts[ next ], temp );
				VectorAdd( temp, hostplane, temp );
				PlaneFromPoints( sideplane, cverts[i], cverts[ next ], temp );

				/* planetest sample point */
				e = DotProduct( origin, sideplane );
				e = e - sideplane[3];
				if ( e >= 0 )
				{
					/* move the sample point back inside triangle bounds */
					origin[0] -= sideplane[0]*(e + 0.25);
					origin[1] -= sideplane[1]*(e + 0.25);
					origin[2] -= sideplane[2]*(e + 0.25);
				}
			}
		}
	}
	
	/* push the origin off the surface (sample offset) */
	/* vortex: set lightmap sample offset based on sampleSize (unless set explicitly) */
	lightmapSampleOffset = ( si != NULL ) ? si->lightmapSampleOffset : 0.0f;
	if( lightmapSampleOffset <= 0.0f )
	{
		lightmapSampleOffset = (lm->actualSampleSize * 0.5f) / superSample;
		lightmapSampleOffset = min(max(0.25f, lightmapSampleOffset), 4.0f);
	}
	VectorMA( origin, lightmapSampleOffset, triplane, origin );

	/* get cluster */
	pointCluster = ClusterForPointExtFilter( origin, LUXEL_EPSILON, numClusters, clusters );
	
	/* point in solid? */
	numnudges = 0;
#ifdef NUDGE
	if( pointCluster < 0 )
	{
		/* nudge the the location around */
		nudge = nudges[ 0 ];
		while( nudge[ 0 ] > -999999.0f )
		{
			/* vortex: try 4 distances (0.25, 0.5, 0.75, 1.0) */
			for( e = 0.25f; e <= 1.0f; e += 0.25f )
			{
				/* nudge the vector around a bit */
				nudged[ 0 ] = origin[ 0 ] + ((nudge[ 0 ] * vecs[ 0 ][ 0 ] * lm->sampleSize) + (nudge[ 1 ] * vecs[ 1 ][ 0 ] * lm->sampleSize) + (nudge[ 2 ] * vecs[ 2 ][ 0 ] * lm->sampleSize)) * e;
				nudged[ 1 ] = origin[ 1 ] + ((nudge[ 0 ] * vecs[ 0 ][ 1 ] * lm->sampleSize) + (nudge[ 1 ] * vecs[ 1 ][ 1 ] * lm->sampleSize) + (nudge[ 2 ] * vecs[ 2 ][ 0 ] * lm->sampleSize)) * e;
				nudged[ 2 ] = origin[ 2 ] + ((nudge[ 0 ] * vecs[ 0 ][ 2 ] * lm->sampleSize) + (nudge[ 1 ] * vecs[ 1 ][ 2 ] * lm->sampleSize) + (nudge[ 2 ] * vecs[ 2 ][ 0 ] * lm->sampleSize)) * e;
				
				/* get pvs cluster */
				numnudges++;
				pointCluster = ClusterForPointExtFilter( nudged, LUXEL_EPSILON, numClusters, clusters );
				if( pointCluster >= 0 )
				{
					VectorCopy( nudged, origin );
					numLuxelsNudged++;
					goto luxelNudged;
				}
			}

			/* try next nudge position */
			nudge += 3;
		}

		/* as a last resort, if still in solid, try drawvert origin offset by normal */
		if( si != NULL )
		{
			VectorMA( dv->xyz, lightmapSampleOffset, dv->normal, nudged );
			numnudges++;
			pointCluster = ClusterForPointExtFilter( nudged, LUXEL_EPSILON, numClusters, clusters );
			if( pointCluster >= 0 )
			{
				VectorCopy( nudged, origin );
				numLuxelsNudged++;
			}
		}
	}
luxelNudged:
#endif

	/* valid? */
	if( pointCluster < 0 )
	{
		(*cluster) = CLUSTER_OCCLUDED;
		numLuxelsOccluded++;
		return (*cluster);
	}
	
	/* do bumpmap calculations */
	if( stv )
		PerturbNormal( dv, si, pNormal, stv, ttv );
	else
		VectorCopy( dv->normal, pNormal );
	
	/* store cluster */
	(*cluster) = pointCluster;

	/* store normal */
	if( remap )
		VectorAdd( normal, pNormal, normal );
	else
		VectorCopy( pNormal, normal );
	VectorNormalize( normal, normal );

	/* store explicit mapping pass and implicit mapping pass */
	luxel[ 0 ] = pass;
	luxel[ 1 ] = numnudges;
	luxel[ 3 ] = 1.0f;

	/* add to count */
	numLuxelsMapped++;
	
	/* return ok */
	return (*cluster);
}



/*
MapTriangle_r()
recursively subdivides a triangle until its edges are shorter
than the distance between two luxels (thanks jc :)
*/

static void MapTriangle_r( rawLightmap_t *lm, surfaceInfo_t *info, bspDrawVert_t *dv[ 3 ], vec4_t plane, vec4_t triplane, vec3_t stv[ 3 ], vec3_t ttv[ 3 ], vec3_t worldverts[ 3 ] )
{
	bspDrawVert_t	mid, *dv2[ 3 ];
	int				max;
	
	
	/* map the vertexes */
	#if 0
	MapSingleLuxel( lm, info, dv[ 0 ], plane, triplane, 1, stv, ttv );
	MapSingleLuxel( lm, info, dv[ 1 ], plane, triplane, 1, stv, ttv );
	MapSingleLuxel( lm, info, dv[ 2 ], plane, triplane, 1, stv, ttv );
	#endif
	
	/* subdivide calc */
	{
		int			i;
		float		*a, *b, dx, dy, dist, maxDist;
		
		
		/* find the longest edge and split it */
		max = -1;
		maxDist = 0;
		for( i = 0; i < 3; i++ )
		{
			/* get verts */
			a = dv[ i ]->lightmap[ 0 ];
			b = dv[ (i + 1) % 3 ]->lightmap[ 0 ];
			
			/* get dists */
			dx = a[ 0 ] - b[ 0 ];
			dy = a[ 1 ] - b[ 1 ];
			dist = (dx * dx) + (dy * dy);	//% sqrt( (dx * dx) + (dy * dy) );
			
			/* longer? */
			if( dist > maxDist )
			{
				maxDist = dist;
				max = i;
			}
		}
		
		/* try to early out */
		if( max < 0 || maxDist <= subdivideThreshold )	/* ydnar: was i < 0 instead of max < 0 (?) */
			return;
	}
	
	/* split the longest edge and map it */
	LerpDrawVert( dv[ max ], dv[ (max + 1) % 3 ], &mid );
	MapSingleLuxel( lm, info, &mid, plane, triplane, 1, stv, ttv, worldverts );
	
	/* push the point up a little bit to account for fp creep (fixme: revisit this) */
	//%	VectorMA( mid.xyz, 2.0f, mid.normal, mid.xyz );
	
	/* recurse to first triangle */
	VectorCopy( dv, dv2 );
	dv2[ max ] = &mid;
	MapTriangle_r( lm, info, dv2, plane, triplane, stv, ttv, worldverts );
	
	/* recurse to second triangle */
	VectorCopy( dv, dv2 );
	dv2[ (max + 1) % 3 ] = &mid;
	MapTriangle_r( lm, info, dv2, plane, triplane, stv, ttv, worldverts );
}



/*
MapTriangle()
seed function for MapTriangle_r()
requires a cw ordered triangle
*/

static qboolean MapTriangle( rawLightmap_t *lm, surfaceInfo_t *info, bspDrawVert_t *dv[ 3 ], qboolean mapNonAxial )
{
	int				i;
	vec4_t			plane, triplane;
	vec3_t			*stv, *ttv, stvStatic[ 3 ], ttvStatic[ 3 ];
	vec3_t			worldverts[ 3 ];
	qboolean        triplanemade;
	
	/* make triangle plane */
	triplanemade = PlaneFromPoints( triplane, dv[ 0 ]->xyz, dv[ 1 ]->xyz, dv[ 2 ]->xyz );

	/* get plane if possible */
	if( lm->plane != NULL )
	{
		VectorCopy( lm->plane, plane );
		plane[ 3 ] = lm->plane[ 3 ];
	}
	
	/* otherwise make one from the points */
	else 
	{
		if( triplanemade == qfalse )
			return qfalse;
		VectorCopy( triplane, plane );
		plane[ 3 ] = triplane[ 3 ];
	}
	
	/* check to see if we need to calculate texture->world tangent vectors */
	if( info->si->normalImage != NULL && CalcTangentVectors( 3, dv, stvStatic, ttvStatic ) )
	{
		stv = stvStatic;
		ttv = ttvStatic;
	}
	else
	{
		stv = NULL;
		ttv = NULL;
	}
	
	VectorCopy( dv[ 0 ]->xyz, worldverts[ 0 ] );
	VectorCopy( dv[ 1 ]->xyz, worldverts[ 1 ] );
	VectorCopy( dv[ 2 ]->xyz, worldverts[ 2 ] );
	
	/* map the vertexes */
	MapSingleLuxel( lm, info, dv[ 0 ], plane, triplane, 1, stv, ttv, worldverts );
	MapSingleLuxel( lm, info, dv[ 1 ], plane, triplane, 1, stv, ttv, worldverts );
	MapSingleLuxel( lm, info, dv[ 2 ], plane, triplane, 1, stv, ttv, worldverts );
	
	/* 2002-11-20: prefer axial triangle edges */
	if( mapNonAxial )
	{
		/* subdivide the triangle */
		MapTriangle_r( lm, info, dv, plane, triplane, stv, ttv, worldverts );
		return qtrue;
	}
	
	for( i = 0; i < 3; i++ )
	{
		float			*a, *b;
		bspDrawVert_t	*dv2[ 3 ];
		
		/* get verts */
		a = dv[ i ]->lightmap[ 0 ];
		b = dv[ (i + 1) % 3 ]->lightmap[ 0 ];
		
		/* make degenerate triangles for mapping edges */
		if( fabs( a[ 0 ] - b[ 0 ] ) < 0.01f || fabs( a[ 1 ] - b[ 1 ] ) < 0.01f )
		{
			dv2[ 0 ] = dv[ i ];
			dv2[ 1 ] = dv[ (i + 1) % 3 ];
			dv2[ 2 ] = dv[ (i + 1) % 3 ];
			
			/* map the degenerate triangle */
			MapTriangle_r( lm, info, dv2, plane, triplane, stv, ttv, worldverts );
		}
	}
	
	return qtrue;
}



/*
MapQuad_r()
recursively subdivides a quad until its edges are shorter
than the distance between two luxels
*/

static void MapQuad_r( rawLightmap_t *lm, surfaceInfo_t *info, bspDrawVert_t *dv[ 4 ], vec4_t plane, vec3_t triplane, vec3_t stv[ 4 ], vec3_t ttv[ 4 ] )
{
	bspDrawVert_t	mid[ 2 ], *dv2[ 4 ];
	int				max;
	
	
	/* subdivide calc */
	{
		int			i;
		float		*a, *b, dx, dy, dist, maxDist;
		
		
		/* find the longest edge and split it */
		max = -1;
		maxDist = 0;
		for( i = 0; i < 4; i++ )
		{
			/* get verts */
			a = dv[ i ]->lightmap[ 0 ];
			b = dv[ (i + 1) % 4 ]->lightmap[ 0 ];
			
			/* get dists */
			dx = a[ 0 ] - b[ 0 ];
			dy = a[ 1 ] - b[ 1 ];
			dist = (dx * dx) + (dy * dy);	//% sqrt( (dx * dx) + (dy * dy) );
			
			/* longer? */
			if( dist > maxDist )
			{
				maxDist = dist;
				max = i;
			}
		}
		
		/* try to early out */
		if( max < 0 || maxDist <= subdivideThreshold )
			return;
	}
	
	/* we only care about even/odd edges */
	max &= 1;
	
	/* split the longest edges */
	LerpDrawVert( dv[ max ], dv[ (max + 1) % 4 ], &mid[ 0 ] );
	LerpDrawVert( dv[ max + 2 ], dv[ (max + 3) % 4 ], &mid[ 1 ] );
	
	/* map the vertexes */
	MapSingleLuxel( lm, info, &mid[ 0 ], plane, triplane, 1, stv, ttv, NULL );
	MapSingleLuxel( lm, info, &mid[ 1 ], plane, triplane, 1, stv, ttv, NULL );
	
	/* 0 and 2 */
	if( max == 0 )
	{
		/* recurse to first quad */
		dv2[ 0 ] = dv[ 0 ];
		dv2[ 1 ] = &mid[ 0 ];
		dv2[ 2 ] = &mid[ 1 ];
		dv2[ 3 ] = dv[ 3 ];
		MapQuad_r( lm, info, dv2, plane, triplane, stv, ttv );
		
		/* recurse to second quad */
		dv2[ 0 ] = &mid[ 0 ];
		dv2[ 1 ] = dv[ 1 ];
		dv2[ 2 ] = dv[ 2 ];
		dv2[ 3 ] = &mid[ 1 ];
		MapQuad_r( lm, info, dv2, plane, triplane, stv, ttv );
	}
	
	/* 1 and 3 */
	else
	{
		/* recurse to first quad */
		dv2[ 0 ] = dv[ 0 ];
		dv2[ 1 ] = dv[ 1 ];
		dv2[ 2 ] = &mid[ 0 ];
		dv2[ 3 ] = &mid[ 1 ];
		MapQuad_r( lm, info, dv2, plane, triplane, stv, ttv );
		
		/* recurse to second quad */
		dv2[ 0 ] = &mid[ 1 ];
		dv2[ 1 ] = &mid[ 0 ];
		dv2[ 2 ] = dv[ 2 ];
		dv2[ 3 ] = dv[ 3 ];
		MapQuad_r( lm, info, dv2, plane, triplane, stv, ttv );
	}
}



/*
MapQuad()
seed function for MapQuad_r()
requires a cw ordered triangle quad
*/

#define QUAD_PLANAR_EPSILON		0.5f

static qboolean MapQuad( rawLightmap_t *lm, surfaceInfo_t *info, bspDrawVert_t *dv[ 4 ] )
{
	float			dist;
	vec4_t			plane, triplane;
	vec3_t			*stv, *ttv, stvStatic[ 4 ], ttvStatic[ 4 ];
	qboolean        triplanemade;

	/* make plane */
	triplanemade = PlaneFromPoints( triplane, dv[ 0 ]->xyz, dv[ 1 ]->xyz, dv[ 2 ]->xyz );
	
	/* get plane if possible */
	if( lm->plane != NULL )
	{
		VectorCopy( lm->plane, plane );
		plane[ 3 ] = lm->plane[ 3 ];
	}
	
	/* otherwise make one from the points */
	else 
	{ 
		if( triplanemade == qfalse )
			return qfalse;
		VectorCopy(  triplane, plane );
		plane[ 3 ] = triplane[ 3 ];
	}
	
	/* 4th point must fall on the plane */
	dist = DotProduct( plane, dv[ 3 ]->xyz ) - plane[ 3 ];
	if( fabs( dist ) > QUAD_PLANAR_EPSILON )
		return qfalse;
	
	/* check to see if we need to calculate texture->world tangent vectors */
	if( info->si->normalImage != NULL && CalcTangentVectors( 4, dv, stvStatic, ttvStatic ) )
	{
		stv = stvStatic;
		ttv = ttvStatic;
	}
	else
	{
		stv = NULL;
		ttv = NULL;
	}
	
	/* map the vertexes */
	MapSingleLuxel( lm, info, dv[ 0 ], plane, triplane, 1, stv, ttv, NULL );
	MapSingleLuxel( lm, info, dv[ 1 ], plane, triplane, 1, stv, ttv, NULL );
	MapSingleLuxel( lm, info, dv[ 2 ], plane, triplane, 1, stv, ttv, NULL );
	MapSingleLuxel( lm, info, dv[ 3 ], plane, triplane, 1, stv, ttv, NULL );
	
	/* subdivide the quad */
	MapQuad_r( lm, info, dv, plane, triplane, stv, ttv );
	return qtrue;
}

/*
MapRawLightmap()
maps the locations, normals, and pvs clusters for a raw lightmap
*/

#define VectorDivide( in, d, out )	VectorScale( in, (1.0f / (d)), out )	//%	(out)[ 0 ] = (in)[ 0 ] / (d), (out)[ 1 ] = (in)[ 1 ] / (d), (out)[ 2 ] = (in)[ 2 ] / (d)

void MapRawLightmap(int rawLightmapNum)
{
	int					n, num, i, x, y, sx, sy, pw[ 5 ], r, *cluster, mapNonAxial;
	float				*luxel, *origin, *normal, samples, radius, pass;
	rawLightmap_t		*lm;
	bspDrawSurface_t	*ds;
	surfaceInfo_t		*info;
	mesh_t				src, *subdivided, *mesh;
	bspDrawVert_t		*verts, *dv[ 4 ], fake;
	
	
	/* bail if this number exceeds the number of raw lightmaps */
	if( rawLightmapNum >= numRawLightmaps )
		return;
	
	/* get lightmap */
	lm = &rawLightmaps[rawLightmapNum];

	/* -----------------------------------------------------------------
	   map referenced surfaces onto the raw lightmap
	   ----------------------------------------------------------------- */
	
	/* walk the list of surfaces on this raw lightmap */
	for( n = 0; n < lm->numLightSurfaces; n++ )
	{
		/* with > 1 surface per raw lightmap, clear occluded */
		if( n > 0 )
		{
			for( y = 0; y < lm->sh; y++ )
			{
				for( x = 0; x < lm->sw; x++ )
				{
					/* get cluster */
					cluster = SUPER_CLUSTER( x, y );
					if( *cluster < 0 )
						*cluster = CLUSTER_UNMAPPED;
				}
			}
		}
		
		/* get surface */
		num = lightSurfaces[ lm->firstLightSurface + n ];
		ds = &bspDrawSurfaces[ num ];
		info = &surfaceInfos[ num ];

		/* bail if no lightmap to calculate */
		if( info->lm != lm )
		{
			Sys_Printf( "!" );
			continue;
		}
		
		/* map the surface onto the lightmap origin/cluster/normal buffers */
		switch( ds->surfaceType )
		{
			case MST_PLANAR:
				/* get verts */
				verts = yDrawVerts + ds->firstVert;
				
				/* map the triangles */
				for( mapNonAxial = 0; mapNonAxial < 2; mapNonAxial++ )
				{
					for( i = 0; i < ds->numIndexes; i += 3 )
					{
						dv[ 0 ] = &verts[ bspDrawIndexes[ ds->firstIndex + i ] ];
						dv[ 1 ] = &verts[ bspDrawIndexes[ ds->firstIndex + i + 1 ] ];
						dv[ 2 ] = &verts[ bspDrawIndexes[ ds->firstIndex + i + 2 ] ];
						MapTriangle( lm, info, dv, mapNonAxial ? qtrue : qfalse );
					}
				}
				break;
			
			case MST_PATCH:
				/* make a mesh from the drawsurf */ 
				src.width = ds->patchWidth;
				src.height = ds->patchHeight;
				src.verts = &yDrawVerts[ ds->firstVert ];
				//%	subdivided = SubdivideMesh( src, 8, 512 );
				subdivided = SubdivideMesh2( src, info->patchIterations );
				
				/* fit it to the curve and remove colinear verts on rows/columns */
				PutMeshOnCurve( *subdivided );
				mesh = RemoveLinearMeshColumnsRows( subdivided );
				FreeMesh( subdivided );
				
				/* get verts */
				verts = mesh->verts;
				
				/* debug code */
				#if 0
					if( lm->plane )
					{
						Sys_Printf( "Planar patch: [%1.3f %1.3f %1.3f] [%1.3f %1.3f %1.3f] [%1.3f %1.3f %1.3f]\n",
							lm->plane[ 0 ], lm->plane[ 1 ], lm->plane[ 2 ],
							lm->vecs[ 0 ][ 0 ], lm->vecs[ 0 ][ 1 ], lm->vecs[ 0 ][ 2 ],
							lm->vecs[ 1 ][ 0 ], lm->vecs[ 1 ][ 1 ], lm->vecs[ 1 ][ 2 ] );
					}
				#endif
				
				/* map the mesh quads */
				#if 0

				for( mapNonAxial = 0; mapNonAxial < 2; mapNonAxial++ )
				{
					for( y = 0; y < (mesh->height - 1); y++ )
					{
						for( x = 0; x < (mesh->width - 1); x++ )
						{
							/* set indexes */
							pw[ 0 ] = x + (y * mesh->width);
							pw[ 1 ] = x + ((y + 1) * mesh->width);
							pw[ 2 ] = x + 1 + ((y + 1) * mesh->width);
							pw[ 3 ] = x + 1 + (y * mesh->width);
							pw[ 4 ] = x + (y * mesh->width);	/* same as pw[ 0 ] */
							
							/* set radix */
							r = (x + y) & 1;
							
							/* get drawverts and map first triangle */
							dv[ 0 ] = &verts[ pw[ r + 0 ] ];
							dv[ 1 ] = &verts[ pw[ r + 1 ] ];
							dv[ 2 ] = &verts[ pw[ r + 2 ] ];
							MapTriangle( lm, info, dv, mapNonAxial );
							
							/* get drawverts and map second triangle */
							dv[ 0 ] = &verts[ pw[ r + 0 ] ];
							dv[ 1 ] = &verts[ pw[ r + 2 ] ];
							dv[ 2 ] = &verts[ pw[ r + 3 ] ];
							MapTriangle( lm, info, dv, mapNonAxial );
						}
					}
				}
				
				#else
				
				for( y = 0; y < (mesh->height - 1); y++ )
				{
					for( x = 0; x < (mesh->width - 1); x++ )
					{
						/* set indexes */
						pw[ 0 ] = x + (y * mesh->width);
						pw[ 1 ] = x + ((y + 1) * mesh->width);
						pw[ 2 ] = x + 1 + ((y + 1) * mesh->width);
						pw[ 3 ] = x + 1 + (y * mesh->width);
						pw[ 4 ] = pw[ 0 ];
						
						/* set radix */
						r = (x + y) & 1;
						
						/* attempt to map quad first */
						dv[ 0 ] = &verts[ pw[ r + 0 ] ];
						dv[ 1 ] = &verts[ pw[ r + 1 ] ];
						dv[ 2 ] = &verts[ pw[ r + 2 ] ];
						dv[ 3 ] = &verts[ pw[ r + 3 ] ];
						if( MapQuad( lm, info, dv ) )
							continue;
						
						/* get drawverts and map first triangle */
						MapTriangle( lm, info, dv, mapNonAxial ? qtrue : qfalse );
						
						/* get drawverts and map second triangle */
						dv[ 1 ] = &verts[ pw[ r + 2 ] ];
						dv[ 2 ] = &verts[ pw[ r + 3 ] ];
						MapTriangle( lm, info, dv, mapNonAxial ? qtrue : qfalse );
					}
				}
				
				#endif
				
				/* free the mesh */
				FreeMesh( mesh );
				break;
			
			default:
				break;
		}
	}
	
	/* -----------------------------------------------------------------
	   average and clean up luxel normals
	   ----------------------------------------------------------------- */
	
	/* walk the luxels */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			luxel = SUPER_LUXEL( 0, x, y );
			normal = SUPER_NORMAL( x, y );
			cluster = SUPER_CLUSTER( x, y );

			/* clear lights count */
			luxel[ 4 ] = 0.0f;

			/* only look at mapped luxels */
			if( *cluster < 0 )
				continue;
			
			/* the normal data could be the sum of multiple samples */
			if( luxel[ 3 ] > 1.0f )
				VectorNormalize( normal, normal );
			
			/* mark this luxel as having only one normal */
			luxel[ 3 ] = 1.0f;
		}
	}
	
	/* non-planar surfaces stop here */
	if( lm->plane == NULL )
		return;
	
	/* -----------------------------------------------------------------
	   map occluded or unused luxels
	   ----------------------------------------------------------------- */

	/* walk the luxels */
	radius = floor( (float)superSample / 2.0f );
	radius = radius > 0 ? radius : 1.0f;
	radius += 1.0f;
	for( pass = 2.0f; pass <= radius; pass += 1.0f )
	{
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get luxel */
				luxel = SUPER_LUXEL( 0, x, y );
				normal = SUPER_NORMAL( x, y );
				cluster = SUPER_CLUSTER( x, y );
				
				/* only look at unmapped luxels */
				if( *cluster != CLUSTER_UNMAPPED )
					continue;
				
				/* divine a normal and origin from neighboring luxels */
				VectorClear( fake.xyz );
				VectorClear( fake.normal );
				fake.lightmap[ 0 ][ 0 ] = x;	//% 0.0001 + x;
				fake.lightmap[ 0 ][ 1 ] = y;	//% 0.0001 + y;
				samples = 0.0f;
				for( sy = (y - 1); sy <= (y + 1); sy++ )
				{
					if( sy < 0 || sy >= lm->sh )
						continue;
					
					for( sx = (x - 1); sx <= (x + 1); sx++ )
					{
						if( sx < 0 || sx >= lm->sw || (sx == x && sy == y) )
							continue;
						
						/* get neighboring luxel */
						luxel = SUPER_LUXEL( 0, sx, sy );
						origin = SUPER_ORIGIN( sx, sy );
						normal = SUPER_NORMAL( sx, sy );
						cluster = SUPER_CLUSTER( sx, sy );
						
						/* only consider luxels mapped in previous passes */
						if( *cluster < 0 || luxel[ 0 ] >= pass )
							continue;
						
						/* add its distinctiveness to our own */
						VectorAdd( fake.xyz, origin, fake.xyz );
						VectorAdd( fake.normal, normal, fake.normal );
						samples += luxel[ 3 ];
					}
				}
				
				/* any samples? */
				if( samples == 0.0f )
					continue;
				
				/* average */
				VectorDivide( fake.xyz, samples, fake.xyz );
				if( VectorNormalize( fake.normal, fake.normal ) == 0.0f )
					continue;
				
				/* map the fake vert */
				MapSingleLuxel( lm, NULL, &fake, lm->plane, lm->plane, pass, NULL, NULL, NULL );
			}
		}
	}
	
	/* -----------------------------------------------------------------
	   average and clean up luxel normals
	   ----------------------------------------------------------------- */
	
	/* walk the luxels */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			luxel = SUPER_LUXEL( 0, x, y );
			normal = SUPER_NORMAL( x, y );
			cluster = SUPER_CLUSTER( x, y );
			
			/* only look at mapped luxels */
			if( *cluster < 0 )
				continue;
			
			/* the normal data could be the sum of multiple samples */
			if( luxel[ 3 ] > 1.0f )
				VectorNormalize( normal, normal );
			
			/* mark this luxel as having only one normal */
			luxel[ 3 ] = 1.0f;
		}
	}
	
	/* debug code */
	#if 0
		Sys_Printf( "\n" );
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				vec3_t	mins, maxs;
				

				cluster = SUPER_CLUSTER( x, y );
				origin = SUPER_ORIGIN( x, y );
				normal = SUPER_NORMAL( x, y );
				luxel = SUPER_LUXEL( x, y );
				
				if( *cluster < 0 )
					continue;
				
				/* check if within the bounding boxes of all surfaces referenced */
				ClearBoundsLighting( mins, maxs );
				for( n = 0; n < lm->numLightSurfaces; n++ )
				{
					int TOL;
					info = &surfaceInfos[ lightSurfaces[ lm->firstLightSurface + n ] ];
					TOL = info->sampleSize + 2;
					AddPointToBoundsLighting( info->mins, mins, maxs );
					AddPointToBoundsLighting( info->maxs, mins, maxs );
					if( origin[ 0 ] > (info->mins[ 0 ] - TOL) && origin[ 0 ] < (info->maxs[ 0 ] + TOL) &&
						origin[ 1 ] > (info->mins[ 1 ] - TOL) && origin[ 1 ] < (info->maxs[ 1 ] + TOL) &&
						origin[ 2 ] > (info->mins[ 2 ] - TOL) && origin[ 2 ] < (info->maxs[ 2 ] + TOL) )
						break;
				}
				
				/* inside? */
				if( n < lm->numLightSurfaces )
					continue;
				
				/* report bogus origin */
				Sys_Printf( "%6d [%2d,%2d] (%4d): XYZ(%+4.1f %+4.1f %+4.1f) LO(%+4.1f %+4.1f %+4.1f) HI(%+4.1f %+4.1f %+4.1f) <%3.0f>\n",
					rawLightmapNum, x, y, *cluster,
					origin[ 0 ], origin[ 1 ], origin[ 2 ],
					mins[ 0 ], mins[ 1 ], mins[ 2 ],
					maxs[ 0 ], maxs[ 1 ], maxs[ 2 ],
					luxel[ 3 ] );
			}
		}
	#endif
}



/*
SetupDirt()
sets up dirtmap (ambient occlusion)
*/

#define DIRT_CONE_ANGLE				88	/* degrees */
#define DIRT_NUM_ANGLE_STEPS		16
#define DIRT_NUM_ELEVATION_STEPS	3
#define	DIRT_NUM_VECTORS			(DIRT_NUM_ANGLE_STEPS * DIRT_NUM_ELEVATION_STEPS)

static int numDirtEntities = 0;

vec3_t dirtVectors2[42] = {
	{8.72255, 5.42647, -0.04020}, 
	{10.25000, -0.04020, -0.04020}, 
	{8.32059, 3.17549, 5.10490}, 
	{5.42647, -0.04020, 8.72255}, 
	{8.32059, -3.17549, 5.10490}, 
	{8.72255, -5.42647, -0.04020}, 
	{5.10490, 8.32059, 3.17549}, 
	{-0.04020, 8.72255, 5.42647}, 
	{3.17549, 5.10490, 8.32059}, 
	{-3.17549, 5.10490, 8.32059}, 
	{-5.42647, -0.04020, 8.72255}, 
	{-0.04020, -0.04020, 10.25000}, 
	{3.17549, -5.10490, 8.32059}, 
	{-3.17549, -5.10490, 8.32059}, 
	{-0.04020, -8.72255, 5.42647}, 
	{5.10490, -8.32059, 3.17549}, 
	{-0.04020, -10.25000, -0.04020}, 
	{-0.04020, -8.72255, -5.42647}, 
	{5.10490, -8.32059, -3.17549}, 
	{-5.10490, -8.32059, 3.17549}, 
	{-8.72255, -5.42647, -0.04020}, 
	{-5.10490, -8.32059, -3.17549}, 
	{-8.32059, -3.17549, 5.10490}, 
	{-8.32059, 3.17549, 5.10490}, 
	{-8.72255, 5.42647, -0.04020}, 
	{-10.25000, -0.04020, -0.04020}, 
	{-8.32059, 3.17549, -5.10490}, 
	{-5.42647, -0.04020, -8.72255}, 
	{-8.32059, -3.17549, -5.10490}, 
	{-3.17549, -5.10490, -8.32059}, 
	{-0.04020, -0.04020, -10.25000}, 
	{5.42647, -0.04020, -8.72255}, 
	{3.17549, -5.10490, -8.32059}, 
	{-3.17549, 5.10490, -8.32059}, 
	{-0.04020, 8.72255, -5.42647}, 
	{3.17549, 5.10490, -8.32059}, 
	{5.10490, 8.32059, -3.17549}, 
	{8.32059, 3.17549, -5.10490}, 
	{8.32059, -3.17549, -5.10490}, 
	{-0.04020, 10.25000, -0.04020}, 
	{-5.10490, 8.32059, 3.17549}, 
	{-5.10490, 8.32059, -3.17549}
};

vec3_t dirtVectors3[162] = {
	{8.93530, 5.55882, -0.04118},
	{10.08824, 2.84118, -0.04118},
	{9.01765, 4.57059, 2.75882},
	{8.52353, 3.25294, 5.22941},
	{10.00588, 1.68824, 2.75882},
	{10.50000, -0.04118, -0.04118},
	{5.55882, -0.04118, 8.93530},
	{7.28824, 1.68824, 7.37059},
	{7.28824, -1.68824, 7.37059},
	{8.52353, -3.25294, 5.22941},
	{8.93530, -0.04118, 5.55882},
	{8.93530, -5.55882, -0.04118},
	{9.01765, -4.57059, 2.75882},
	{10.08824, -2.84118, -0.04118},
	{10.00588, -1.68824, 2.75882},
	{7.37059, 7.28824, 1.68824},
	{5.22941, 8.52353, 3.25294},
	{7.20588, 6.13529, 4.48824},
	{-0.04118, 8.93530, 5.55882},
	{2.75882, 9.01765, 4.57059},
	{1.68824, 7.37059, 7.28824},
	{3.25294, 5.22941, 8.52353},
	{4.48824, 7.20588, 6.13529},
	{4.57059, 2.75882, 9.01765},
	{6.13529, 4.48824, 7.20588},
	{-1.68824, 7.37059, 7.28824},
	{-3.25294, 5.22941, 8.52353},
	{-0.04118, 5.55882, 8.93530},
	{-5.55882, -0.04118, 8.93530},
	{-4.57059, 2.75882, 9.01765},
	{-2.84118, -0.04118, 10.08824},
	{-0.04118, -0.04118, 10.50000},
	{-1.68824, 2.75882, 10.00588},
	{2.84118, -0.04118, 10.08824},
	{1.68824, 2.75882, 10.00588},
	{4.57059, -2.75882, 9.01765},
	{1.68824, -2.75882, 10.00588},
	{3.25294, -5.22941, 8.52353},
	{-4.57059, -2.75882, 9.01765},
	{-3.25294, -5.22941, 8.52353},
	{-1.68824, -2.75882, 10.00588},
	{-0.04118, -8.93529, 5.55882},
	{-1.68824, -7.37059, 7.28824},
	{1.68824, -7.37059, 7.28824},
	{-0.04118, -5.55882, 8.93530},
	{6.13529, -4.48823, 7.20588},
	{2.75882, -9.01765, 4.57059},
	{5.22941, -8.52353, 3.25294},
	{4.48824, -7.20588, 6.13529},
	{7.37059, -7.28824, 1.68824},
	{7.20588, -6.13529, 4.48824},
	{-0.04118, -10.08823, 2.84118},
	{-0.04118, -10.50000, -0.04118},
	{2.75882, -10.00588, 1.68824},
	{-0.04118, -8.93529, -5.55882},
	{-0.04118, -10.08823, -2.84118},
	{2.75882, -9.01765, -4.57059},
	{5.22941, -8.52353, -3.25294},
	{2.75882, -10.00588, -1.68824},
	{7.37059, -7.28824, -1.68824},
	{5.55882, -8.93529, -0.04118},
	{-2.75882, -9.01765, 4.57059},
	{-5.22941, -8.52353, 3.25294},
	{-2.75882, -10.00588, 1.68824},
	{-8.93529, -5.55882, -0.04118},
	{-7.37059, -7.28824, 1.68824},
	{-7.37059, -7.28824, -1.68824},
	{-5.22941, -8.52353, -3.25294},
	{-5.55882, -8.93529, -0.04118},
	{-2.75882, -9.01765, -4.57059},
	{-2.75882, -10.00588, -1.68824},
	{-7.28824, -1.68824, 7.37059},
	{-8.52353, -3.25294, 5.22941},
	{-6.13529, -4.48823, 7.20588},
	{-9.01765, -4.57059, 2.75882},
	{-7.20588, -6.13529, 4.48824},
	{-4.48823, -7.20588, 6.13529},
	{-7.28824, 1.68824, 7.37059},
	{-8.52353, 3.25294, 5.22941},
	{-8.93529, -0.04118, 5.55882},
	{-8.93529, 5.55882, -0.04118},
	{-9.01765, 4.57059, 2.75882},
	{-10.08823, 2.84118, -0.04118},
	{-10.50000, -0.04118, -0.04118},
	{-10.00588, 1.68824, 2.75882},
	{-10.08823, -2.84118, -0.04118},
	{-10.00588, -1.68824, 2.75882},
	{-9.01765, 4.57059, -2.75882},
	{-8.52353, 3.25294, -5.22941},
	{-10.00588, 1.68824, -2.75882},
	{-5.55882, -0.04118, -8.93529},
	{-7.28824, 1.68824, -7.37059},
	{-7.28824, -1.68824, -7.37059},
	{-8.52353, -3.25294, -5.22941},
	{-8.93529, -0.04118, -5.55882},
	{-9.01765, -4.57059, -2.75882},
	{-10.00588, -1.68824, -2.75882},
	{-4.57059, -2.75882, -9.01765},
	{-3.25294, -5.22941, -8.52353},
	{-6.13529, -4.48823, -7.20588},
	{-1.68824, -7.37059, -7.28824},
	{-4.48823, -7.20588, -6.13529},
	{-7.20588, -6.13529, -4.48823},
	{-2.84118, -0.04118, -10.08823},
	{-0.04118, -0.04118, -10.50000},
	{-1.68824, -2.75882, -10.00588},
	{5.55882, -0.04118, -8.93529},
	{2.84118, -0.04118, -10.08823},
	{4.57059, -2.75882, -9.01765},
	{3.25294, -5.22941, -8.52353},
	{1.68824, -2.75882, -10.00588},
	{1.68824, -7.37059, -7.28824},
	{-0.04118, -5.55882, -8.93529},
	{-4.57059, 2.75882, -9.01765},
	{-3.25294, 5.22941, -8.52353},
	{-1.68824, 2.75882, -10.00588},
	{-0.04118, 8.93530, -5.55882},
	{-1.68824, 7.37059, -7.28824},
	{1.68824, 7.37059, -7.28824},
	{3.25294, 5.22941, -8.52353},
	{-0.04118, 5.55882, -8.93529},
	{4.57059, 2.75882, -9.01765},
	{1.68824, 2.75882, -10.00588},
	{2.75882, 9.01765, -4.57059},
	{5.22941, 8.52353, -3.25294},
	{4.48824, 7.20588, -6.13529},
	{7.37059, 7.28824, -1.68824},
	{9.01765, 4.57059, -2.75882},
	{8.52353, 3.25294, -5.22941},
	{7.20588, 6.13529, -4.48823},
	{7.28824, 1.68824, -7.37059},
	{6.13529, 4.48824, -7.20588},
	{10.00588, 1.68824, -2.75882},
	{9.01765, -4.57059, -2.75882},
	{8.52353, -3.25294, -5.22941},
	{10.00588, -1.68824, -2.75882},
	{7.28824, -1.68824, -7.37059},
	{8.93530, -0.04118, -5.55882},
	{6.13529, -4.48823, -7.20588},
	{7.20588, -6.13529, -4.48823},
	{4.48824, -7.20588, -6.13529},
	{-0.04118, 10.08824, 2.84118},
	{2.75882, 10.00588, 1.68824},
	{-0.04118, 10.50000, -0.04118},
	{5.55882, 8.93530, -0.04118},
	{-0.04118, 10.08824, -2.84118},
	{2.75882, 10.00588, -1.68824},
	{-2.75882, 9.01765, 4.57059},
	{-2.75882, 10.00588, 1.68824},
	{-5.22941, 8.52353, 3.25294},
	{-2.75882, 9.01765, -4.57059},
	{-5.22941, 8.52353, -3.25294},
	{-2.75882, 10.00588, -1.68824},
	{-7.37059, 7.28824, -1.68824},
	{-7.37059, 7.28824, 1.68824},
	{-5.55882, 8.93530, -0.04118},
	{-4.48823, 7.20588, 6.13529},
	{-7.20588, 6.13529, 4.48824},
	{-6.13529, 4.48824, 7.20588},
	{-6.13529, 4.48824, -7.20588},
	{-7.20588, 6.13529, -4.48823},
	{-4.48823, 7.20588, -6.13529}
};

char *DirtModeName(dirtMode_t *mode)
{
	if( *mode == DIRTMODE_ORDERED_CONE ) return "ordered cone dirtmaping";
	if( *mode == DIRTMODE_RANDOM_CONE ) return "random cone dirtmaping";
	if( *mode == DIRTMODE_AMBIENT_SPHERE_LOWQUALITY ) return "low quality spherical ambient occlusion";
	if( *mode == DIRTMODE_AMBIENT_SPHERE_HIGHQUALITY ) return "high quality spherical ambient occlusion";
	/* unknown mode */
	Sys_Warning( "DirtModeName: unknown mode %i", (int)*mode);
	*mode = DIRTMODE_ORDERED_CONE;
	return "ordered cone dirtmaping";
}

char *DirtFilterName(dirtFilter_t *filter)
{
	if( *filter == DIRTFILTER_NONE ) return "no filter";
	if( *filter == DIRTFILTER_AVERAGE ) return "average";
	/* unknown filter */
	Sys_Warning( "DirtFilterName: unknown filter %i", (int)*filter );
	*filter = DIRTFILTER_AVERAGE;
	return "average";
}

void EnableDirtForEntity(int num)
{
	/* bail if bad entitiy num */
	if (num < 0 || num > numEntities)
		return;

	/* bail if already allocated */
	if (dirtSettings[num].enabled)
		return;

	/* allocate */
	dirtSettings[num].enabled = qtrue;
	dirtSettings[num].mode = DIRTMODE_ORDERED_CONE;
	dirtSettings[num].depth = 128.0f;
	dirtSettings[num].depthExponent = 2.0f;
	dirtSettings[num].scale = 1.0f;
	VectorSet(dirtSettings[num].scaleMask, 1.0f, 1.0f, 1.0f);
	dirtSettings[num].gain = 1.0f;
	VectorSet(dirtSettings[num].gainMask, 1.0f, 1.0f, 1.0f);
	dirtSettings[num].filter = DIRTFILTER_AVERAGE;
	numDirtEntities++;
	dirty = qtrue;
}

void SetupDirtForEntity(int num)
{
	int i, j, v1;
	float angle, elevation, angleStep, elevationStep;
	double v2, v3, v4;
	const char *value;

	/* read entity keys */
	value = ValueForKey(&entities[num], "_dirty");
	if (value[0] == '\0')
		value = ValueForKey(&entities[num], "_ao");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		sscanf(value, "%i %lf %lf %lf", &v1, &v2, &v3, &v4);
		dirtSettings[num].mode = (dirtMode_t)v1;
		dirtSettings[num].depth = v2;
		dirtSettings[num].gain = v3;
		dirtSettings[num].scale = v4;
	}
	value = ValueForKey(&entities[num], "_aoMode");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].mode = (dirtMode_t)atoi(value);
	}
	value = ValueForKey(&entities[num], "_aoFilter");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].filter = (dirtFilter_t)atoi(value);
	}
	value = ValueForKey(&entities[num], "_aoDepth");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].depth = atof(value);
	}
	value = ValueForKey(&entities[num], "_aoDepthExponent");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].depthExponent = atof(value);
	}
	value = ValueForKey(&entities[num], "_aoGain");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].gain = atof(value);
	}
	value = ValueForKey(&entities[num], "_aoGainMask");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		sscanf(value, "%f %f %f", &dirtSettings[num].gainMask[0], &dirtSettings[num].gainMask[1], &dirtSettings[num].gainMask[2]); 
	}
	value = ValueForKey(&entities[num], "_aoScale");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		dirtSettings[num].scale = atof(value);
	}
	value = ValueForKey(&entities[num], "_aoScaleMask");
	if (value[0] != '\0')
	{
		EnableDirtForEntity(num);
		sscanf(value, "%f %f %f", &dirtSettings[num].scaleMask[0], &dirtSettings[num].scaleMask[1], &dirtSettings[num].scaleMask[2]); 
	}
	if (!dirtSettings[num].enabled)
		return;

	/* fix up wrong values */
	DirtModeName(&dirtSettings[num].mode);
	DirtFilterName(&dirtSettings[num].filter);
	if (dirtSettings[num].depth < 1.0f)
		dirtSettings[num].depth = 128.0f;
	if (dirtSettings[num].gain < 0.0f)
		dirtSettings[num].gain = 1.0f;
	if (dirtSettings[num].scale < 0.01f)
		dirtSettings[num].scale = 0.01f;
	if (dirtSettings[num].depthExponent <= 0.0f)
		dirtSettings[num].depthExponent = 2.0f;
	dirtSettings[num].depth = min(1024.0f, dirtSettings[num].depth);

	/* setup dirt vectors storage */
	if ( dirtSettings[num].mode == DIRTMODE_AMBIENT_SPHERE_LOWQUALITY || dirtSettings[num].mode == DIRTMODE_AMBIENT_SPHERE_HIGHQUALITY )
	{
		/* geosphere dirt vectors */
		if (dirtSettings[num].mode == DIRTMODE_AMBIENT_SPHERE_LOWQUALITY)
		{
			dirtSettings[num].vectors = dirtVectors2;
			dirtSettings[num].numVectors = 42;
		}
		else
		{
			dirtSettings[num].vectors = dirtVectors3;
			dirtSettings[num].numVectors = 162;
		}
		for( i = 0; i < dirtSettings[num].numVectors; i++)
		{
			VectorNormalize(dirtSettings[num].vectors[i], dirtSettings[num].vectors[i]);
			VectorScale(dirtSettings[num].vectors[i], dirtSettings[num].depth, dirtSettings[num].vectors[i]);
		}
		dirtSettings[num].wholeDepth = pow(dirtSettings[num].depth, dirtSettings[num].depthExponent) * dirtSettings[num].numVectors;
	}
	else
	{
		/* calculate angular steps */
		angleStep = DEG2RAD( 360.0f / float(DIRT_NUM_ANGLE_STEPS ) );
		elevationStep = DEG2RAD( float(DIRT_CONE_ANGLE) / float(DIRT_NUM_ELEVATION_STEPS ) );

		/* default cone-based dirt vectors */
		/* get number of dirt vectors */
		angle = 0.0f;
		dirtSettings[num].numVectors = 0;
		for( i = 0, angle = 0.0f; i < DIRT_NUM_ANGLE_STEPS; i++, angle += angleStep )
			for( j = 0, elevation = elevationStep * 0.5f; j < DIRT_NUM_ELEVATION_STEPS; j++, elevation += elevationStep)
				dirtSettings[num].numVectors++;
		dirtSettings[num].vectors = (vec3_t *)safe_malloc(sizeof(vec3_t) * dirtSettings[num].numVectors);

		/* iterate angle */
		angle = 0.0f;
		dirtSettings[num].numVectors = 0;
		for( i = 0, angle = 0.0f; i < DIRT_NUM_ANGLE_STEPS; i++, angle += angleStep )
		{
			/* iterate elevation */
			for( j = 0, elevation = elevationStep * 0.5f; j < DIRT_NUM_ELEVATION_STEPS; j++, elevation += elevationStep )
			{
				dirtSettings[num].vectors[ dirtSettings[num].numVectors ][ 0 ] = sin( elevation ) * cos( angle );
				dirtSettings[num].vectors[ dirtSettings[num].numVectors ][ 1 ] = sin( elevation ) * sin( angle );
				dirtSettings[num].vectors[ dirtSettings[num].numVectors ][ 2 ] = cos( elevation );
				dirtSettings[num].numVectors++;
			}
		}
	}
}

void SetupDirt( void )
{
	int i;

	/* init */
	memset(&dirtSettings, 0, sizeof(dirtSettings));

	/* disabled? */
	if (nodirt)
	{
		dirty = qfalse;
		return;
	}

	/* note it */
	Sys_PrintHeadingVerbose( "--- SetupDirt ---\n" );

	/* setup global dirt */
	if (dirty)
	{
		EnableDirtForEntity(0);
		dirtSettings[0].mode = dirtMode;
		dirtSettings[0].depth = dirtDepth;
		dirtSettings[0].depthExponent = dirtDepthExponent;
		dirtSettings[0].scale = dirtScale;
		VectorCopy(dirtScaleMask, dirtSettings[0].scaleMask);
		dirtSettings[0].gain = dirtGain;
		dirtSettings[0].filter = dirtFilter;
		VectorCopy(dirtGainMask, dirtSettings[0].gainMask);
	}

	/* setup fo entities */
	for (i = 0; i < numEntities; i++)
		SetupDirtForEntity(i);

	/* emit some statistics */
	if (dirtSettings[0].enabled)
	{
		Sys_Printf( "World mode = %s\n", DirtModeName(&dirtSettings[0].mode) );
		Sys_Printf( "Filter mode = %s\n", DirtFilterName(&dirtSettings[0].filter) );
		Sys_Printf( "Gain mask = ( %1.1f %1.1f %1.1f )\n", dirtSettings[0].gainMask[0], dirtSettings[0].gainMask[1], dirtSettings[0].gainMask[2] );
		Sys_Printf( "Scale mask = %1.1f %1.1f %1.1f )\n", dirtSettings[0].scaleMask[0], dirtSettings[0].scaleMask[1], dirtSettings[0].scaleMask[2] );
		Sys_Printf( "%9d vectors\n", dirtSettings[0].numVectors );
		Sys_Printf( "%9.1f depth\n", dirtSettings[0].depth );
		Sys_Printf( "%9.1f depth exponent\n", dirtSettings[0].depthExponent );
		Sys_Printf( "%9.2f gain\n", dirtSettings[0].gain );
		Sys_Printf( "%9.2f scale\n", dirtSettings[0].scale );
	}
	Sys_Printf( "%9i entities with custom dirtmapping\n", numDirtEntities );
}


/*
DirtForSample()
calculates dirt value for a given sample
*/

float DirtForSample( trace_t *trace )
{
	int i;
	dirtSettings_t *dirt;
	float gatherDirt, outDirt, angle, elevation, ooDepth, depth1, depth2;
	vec3_t normal, worldUp, myUp, myRt, temp, direction, displacement;
	qboolean oldTestAll;
	vec_t oldInhibitRadius;
	
	/* dummy check */
	dirt = &dirtSettings[ trace->entityNum ];
	if (!dirt->enabled)
	{
		dirt = &dirtSettings[0];
		if (!dirt->enabled)
			return 1.0f;
	}
	if (trace == NULL || trace->cluster < 0)
		return 0.0f;

	/* q3map2 original dirtmapping */
	if (dirt->mode == 0 || dirt->mode == 1)
	{
		/* setup */
		gatherDirt = 0.0f;
		ooDepth = 1.0f / dirt->depth;
		VectorCopy( trace->normal, normal );
	
		/* check if the normal is aligned to the world-up */
		if( normal[ 0 ] == 0.0f && normal[ 1 ] == 0.0f )
		{
			if( normal[ 2 ] == 1.0f )		
			{
				VectorSet( myRt, 1.0f, 0.0f, 0.0f );
				VectorSet( myUp, 0.0f, 1.0f, 0.0f );
			}
			else if( normal[ 2 ] == -1.0f )
			{
				VectorSet( myRt, -1.0f, 0.0f, 0.0f );
				VectorSet( myUp,  0.0f, 1.0f, 0.0f );
			}
		}
		else
		{
			VectorSet( worldUp, 0.0f, 0.0f, 1.0f );
			CrossProduct( normal, worldUp, myRt );
			VectorNormalize( myRt, myRt );
			CrossProduct( myRt, normal, myUp );
			VectorNormalize( myUp, myUp );
		}
	
		/* 1 = random mode, 0 = non-random mode */
		if( dirt->mode == 1 )
		{
			/* iterate */
			for( i = 0; i < dirt->numVectors; i++ )
			{
				/* get random vector */
				angle = Random() * DEG2RAD( 360.0f );
				elevation = Random() * DEG2RAD( DIRT_CONE_ANGLE );
				temp[ 0 ] = cos( angle ) * sin( elevation );
				temp[ 1 ] = sin( angle ) * sin( elevation );
				temp[ 2 ] = cos( elevation );
				
				/* transform into tangent space */
				direction[ 0 ] = myRt[ 0 ] * temp[ 0 ] + myUp[ 0 ] * temp[ 1 ] + normal[ 0 ] * temp[ 2 ];
				direction[ 1 ] = myRt[ 1 ] * temp[ 0 ] + myUp[ 1 ] * temp[ 1 ] + normal[ 1 ] * temp[ 2 ];
				direction[ 2 ] = myRt[ 2 ] * temp[ 0 ] + myUp[ 2 ] * temp[ 1 ] + normal[ 2 ] * temp[ 2 ];
				
				/* set endpoint */
				VectorMA( trace->origin, dirt->depth, direction, trace->end );
				SetupTrace( trace );
				
				/* trace */
				TraceLine( trace );
				if( trace->opaque || trace->passSolid )
				{
					VectorSubtract( trace->hit, trace->origin, displacement );
					gatherDirt += 1.0f - ooDepth * VectorLength( displacement );
				}
			}
		}
		else
		{
			/* iterate through ordered vectors */
			for( i = 0; i < dirt->numVectors; i++ )
			{
				/* transform vector into tangent space */
				direction[ 0 ] = myRt[ 0 ] * dirt->vectors[ i ][ 0 ] + myUp[ 0 ] * dirt->vectors[ i ][ 1 ] + normal[ 0 ] * dirt->vectors[ i ][ 2 ];
				direction[ 1 ] = myRt[ 1 ] * dirt->vectors[ i ][ 0 ] + myUp[ 1 ] * dirt->vectors[ i ][ 1 ] + normal[ 1 ] * dirt->vectors[ i ][ 2 ];
				direction[ 2 ] = myRt[ 2 ] * dirt->vectors[ i ][ 0 ] + myUp[ 2 ] * dirt->vectors[ i ][ 1 ] + normal[ 2 ] * dirt->vectors[ i ][ 2 ];
				
				/* set endpoint */
				VectorMA( trace->origin, dirt->depth, direction, trace->end );
				SetupTrace( trace );
				
				/* trace */
				TraceLine( trace );
				if( trace->opaque || trace->passSolid )
				{
					VectorSubtract( trace->hit, trace->origin, displacement );
					gatherDirt += (1.0f - ooDepth) * VectorLength( displacement );
				}
			}
		}

		/* direct ray */
		VectorMA( trace->origin, dirt->depth, normal, trace->end );
		SetupTrace( trace );
		
		/* trace */
		TraceLine( trace );
		if( trace->opaque || trace->passSolid )
		{
			VectorSubtract( trace->hit, trace->origin, displacement );
			gatherDirt += (1.0f - ooDepth) * VectorLength( displacement );
		}
		
		/* early out */
		if( gatherDirt <= 0.0f )
			return 1.0f;
		
		/* apply gain (does this even do much? heh) */
		outDirt = pow( gatherDirt / (dirt->numVectors + 1), dirt->gain );
		if( outDirt > 1.0f )
			outDirt = 1.0f;
		
		/* apply scale */
		outDirt *= dirt->scale;
		if( outDirt > 1.0f )
			outDirt = 1.0f;
		
		/* return to sender */
		return 1.0f - outDirt;
	}

	/* blood omnicide ambient occlusion */
	VectorCopy(trace->origin, temp);

	/* setup trace */
	oldTestAll = trace->testAll;
	oldInhibitRadius = trace->inhibitRadius;
	trace->inhibitRadius = 0;
	trace->testAll = qfalse;

	/* iterate through uniform vectors */
	gatherDirt = 0.0;
	for( i = 0; i < dirt->numVectors; i++ )
	{
		/* direct ray */
		VectorCopy(temp, trace->origin);
		trace->end[0] = trace->origin[0] + dirt->vectors[i][0];
		trace->end[1] = trace->origin[1] + dirt->vectors[i][1];
		trace->end[2] = trace->origin[2] + dirt->vectors[i][2];
		SetupTrace(trace);
		TraceLine(trace);
		depth1 = dirt->depth;
		if( trace->passSolid == qtrue )
			depth1 = 0.0f;
		else if (trace->opaque)
		{
			VectorSubtract(trace->hit, trace->origin, displacement);
			depth1 = VectorLength(displacement);
			depth1 = min(dirt->depth, depth1);
		}

		/* back ray */
		if( trace->opaque == qfalse )
		{
			VectorCopy( trace->hit, trace->origin );
			VectorCopy( temp, trace->end );
			SetupTrace( trace );
			TraceLine(trace);
			depth2 = dirt->depth;
			if( trace->passSolid == qtrue )
				depth2 = 0.0f;
			else if (trace->opaque)
			{
				VectorSubtract(trace->hit, trace->origin, displacement);
				depth2 = VectorLength(displacement);
				depth2 = min(dirt->depth, depth2);
			}
			depth1 = min( depth1, depth2 );
		}

		/* add dirt */
		gatherDirt += pow(depth1, dirt->depthExponent);
	}
	VectorCopy(temp, trace->origin);

	/* rollback trace settings */
	trace->inhibitRadius = oldInhibitRadius;
	trace->testAll = oldTestAll;

	/* resulting volume is our dirt */
	#define DIRT_SCALE_START 1.0
	#define DIRT_GAIN_START  1.08
	gatherDirt = sqrt((gatherDirt / dirt->wholeDepth) / 0.55);
	if (gatherDirt <= DIRT_SCALE_START)
		return pow(gatherDirt, dirt->scale);
	if (gatherDirt <= DIRT_GAIN_START || !trace->recvShadows || noSurfaces)
	{
		/* also fix gain effect on surfaces not receiving shadows */
		return DIRT_SCALE_START;
	}
	/* gain */
	return 1 + max(0, (gatherDirt - DIRT_GAIN_START)) * (DIRT_GAIN_START / DIRT_SCALE_START) * dirt->gain;
}

/*
DirtyRawLightmap()
calculates dirty fraction for each luxel
*/

void DirtyRawLightmap(int rawLightmapNum)
{
	int					i, x, y, sx, sy, *cluster;
	float				*origin, *normal, *dirt, *dirt2, average, samples;
	rawLightmap_t		*lm;
	surfaceInfo_t		*info;
	trace_t				trace;

	/* bail if this number exceeds the number of raw lightmaps */
	if( rawLightmapNum >= numRawLightmaps )
		return;
	
	/* get lightmap */
	lm = &rawLightmaps[rawLightmapNum];
	if (!dirtSettings[lm->entityNum].enabled && !dirtSettings[0].enabled)
		return;

	/* setup trace */
	trace.entityNum = lm->entityNum;
	trace.testOcclusion = qtrue;
	trace.occlusionBias = -0.01;
	trace.forceSunlight = qfalse;
	trace.forceSelfShadow = qtrue;
	trace.recvShadows = lm->recvShadows;
	trace.numSurfaces = lm->numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
	trace.inhibitRadius = 0;
	trace.testAll = qfalse;
	
	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	trace.twoSided = qfalse;
	for( i = 0; i < trace.numSurfaces; i++ )
	{
		/* get surface */
		info = &surfaceInfos[ trace.surfaces[ i ] ];
		
		/* check twosidedness */
		if( info->si->twoSided )
		{
			trace.twoSided = qtrue;
			break;
		}
	}
	
	/* gather dirt */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			origin = SUPER_TRIORIGIN( x, y );
			normal = SUPER_TRINORMAL( x, y );
			cluster = SUPER_CLUSTER( x, y );
			dirt = SUPER_DIRT( x, y );

			/* bad cluster? */
			if( *cluster < 0 )
			{
				*dirt = 0.0f;
				continue;
			}

			/* set up trace */
			VectorMA( origin, 1.5f, normal, trace.origin);
			VectorCopy( normal, trace.normal );
			trace.cluster = ClusterForPointExt( trace.origin, 0.0f );

			/* get dirt */
			*dirt = DirtForSample( &trace );
		}
	}


	/* filter dirt */
	if ( dirtSettings[lm->entityNum].filter == DIRTFILTER_AVERAGE )
	{
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get luxel */
				cluster = SUPER_CLUSTER( x, y );
				dirt = SUPER_DIRT( x, y );
				
				/* filter dirt by adjacency to unmapped luxels */
				average = *dirt;
				samples = 1.0f;
				for( sy = (y - 1); sy <= (y + 1); sy++ )
				{
					if( sy < 0 || sy >= lm->sh )
						continue;
					
					for( sx = (x - 1); sx <= (x + 1); sx++ )
					{
						if( sx < 0 || sx >= lm->sw || (sx == x && sy == y) )
							continue;
						
						/* get neighboring luxel */
						cluster = SUPER_CLUSTER( sx, sy );
						dirt2 = SUPER_DIRT( sx, sy );
						if( /**cluster < 0 ||*/ *dirt2 <= 0.0f )
							continue;
						
						/* add it */
						average += *dirt2;
						samples += 1.0f;
					}
					
					/* bail */
					if( samples <= 0.0f )
						break;
				}
				
				/* bail */
				if( samples <= 0.0f )
					continue;
				
				/* scale dirt */
				*dirt = *dirt * 0.25 + (average / samples) * 0.75;
			}
		}
	}
}

/*
ApplyDirt
applies dirtmapping to a surface
*/
void ApplyDirt(float *r, float *g, float *b, float *dirt, int entitynum, float scalemod, float gainmod)
{
	dirtSettings_t *dirtsettings;
	vec3_t temp, temp2, scaled;
	float scale;

	#define mix(a, b, f) (a*(1.0 - f) + b*f)

	dirtsettings = &dirtSettings[entitynum];
	if (!dirtsettings->enabled)
		dirtsettings = &dirtSettings[0];

	if (dirtsettings->mode == DIRTMODE_AMBIENT_SPHERE_LOWQUALITY || dirtsettings->mode == DIRTMODE_AMBIENT_SPHERE_HIGHQUALITY)
	{
		/* debug */
		if( dirtDebug )
		{
			*r = 64.0f;
			*g = 64.0f;
			*b = 64.0f;
		}
		/* scale */
		if (*dirt <= 1.0)
		{
			//Sys_Printf("Scale %f %f %f %f\n", *dirt, dirtsettings->scaleMask[0], dirtsettings->scaleMask[1], dirtsettings->scaleMask[2]);
			scale = mix(1.0f, *dirt, scalemod);
			if (scale != 1.0f )
			{
				scaled[0] = mix(0.0f, *r, scale); *r = mix(*r, scaled[0], dirtsettings->scaleMask[0]);
				scaled[1] = mix(0.0f, *g, scale); *g = mix(*g, scaled[1], dirtsettings->scaleMask[1]);
				scaled[2] = mix(0.0f, *b, scale); *b = mix(*b, scaled[2], dirtsettings->scaleMask[2]);
			}
		}
		else
		{
			/* add, keep color */
			//Sys_Printf("Gain %f %f %f %f\n", *dirt, dirtsettings->gainMask[0], dirtsettings->gainMask[1], dirtsettings->gainMask[2]);
			scale = mix(1.0f, *dirt, gainmod);
			VectorSet(temp, *r, *g, *b);
			VectorNormalize(temp, temp2);
			temp[0] += (*dirt - 1.0f);
			temp[1] += (*dirt - 1.0f);
			temp[2] += (*dirt - 1.0f);
			VectorScale(temp2, VectorLength(temp), temp);
			if (scale != 1.0f )
			{
				scaled[0] = mix(0.0f, *r, scale); *r = mix(*r, scaled[0], dirtsettings->gainMask[0]);
				scaled[1] = mix(0.0f, *g, scale); *g = mix(*g, scaled[1], dirtsettings->gainMask[1]);
				scaled[2] = mix(0.0f, *b, scale); *b = mix(*b, scaled[2], dirtsettings->gainMask[2]);
			}
		}
		return;
	}

	/* old q3map2 dirt */
	if (!scalemod)
		return;
	if( dirtDebug )
	{
		*r = *dirt * dirtsettings->scaleMask[0]*scalemod * 64.0f;
		*g = *dirt * dirtsettings->scaleMask[1]*scalemod * 64.0f;
		*b = *dirt * dirtsettings->scaleMask[2]*scalemod * 64.0f;
		return;
	}
	*r *= mix(1, *dirt, dirtsettings->scaleMask[0]*scalemod);
	*g *= mix(1, *dirt, dirtsettings->scaleMask[1]*scalemod);
	*b *= mix(1, *dirt, dirtsettings->scaleMask[2]*scalemod);

	#undef mix
}

/*
SubmapRawLuxel()
calculates the pvs cluster, origin, normal of a sub-luxel
*/

static qboolean SubmapRawLuxel( rawLightmap_t *lm, int x, int y, float bx, float by, int *sampleCluster, vec3_t sampleOrigin, vec3_t sampleNormal )
{
	int			i, *cluster, *cluster2;
	float		*origin, *origin2, *normal;	//%	, *normal2;
	vec3_t		originVecs[ 2 ];			//%	, normalVecs[ 2 ];
	
	
	/* calulate x vector */
	if( (x < (lm->sw - 1) && bx >= 0.0f) || (x == 0 && bx <= 0.0f) )
	{
		cluster = SUPER_CLUSTER( x, y );
		origin = SUPER_ORIGIN( x, y );
		//%	normal = SUPER_NORMAL( x, y );
		cluster2 = SUPER_CLUSTER( x + 1, y );
		origin2 = *cluster2 < 0 ? SUPER_ORIGIN( x, y ) : SUPER_ORIGIN( x + 1, y );
		//%	normal2 = *cluster2 < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x + 1, y );
	}
	else if( (x > 0 && bx <= 0.0f) || (x == (lm->sw - 1) && bx >= 0.0f) )
	{
		cluster = SUPER_CLUSTER( x - 1, y );
		origin = *cluster < 0 ? SUPER_ORIGIN( x, y ) : SUPER_ORIGIN( x - 1, y );
		//%	normal = *cluster < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x - 1, y );
		cluster2 = SUPER_CLUSTER( x, y );
		origin2 = SUPER_ORIGIN( x, y );
		//%	normal2 = SUPER_NORMAL( x, y );
	}
	else
		Sys_Warning( lm->entityNum, "Spurious lightmap S vector" );
	
	VectorSubtract( origin2, origin, originVecs[ 0 ] );
	//%	VectorSubtract( normal2, normal, normalVecs[ 0 ] );
	/* calulate y vector */
	if( (y < (lm->sh - 1) && bx >= 0.0f) || (y == 0 && bx <= 0.0f) )
	{
		cluster = SUPER_CLUSTER( x, y );
		origin = SUPER_ORIGIN( x, y );
		//%	normal = SUPER_NORMAL( x, y );
		cluster2 = SUPER_CLUSTER( x, y + 1 );
		origin2 = *cluster2 < 0 ? SUPER_ORIGIN( x, y ) : SUPER_ORIGIN( x, y + 1 );
		//%	normal2 = *cluster2 < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x, y + 1 );
	}
	else if( (y > 0 && bx <= 0.0f) || (y == (lm->sh - 1) && bx >= 0.0f) )
	{
		cluster = SUPER_CLUSTER( x, y - 1 );
		origin = *cluster < 0 ? SUPER_ORIGIN( x, y ) : SUPER_ORIGIN( x, y - 1 );
		//%	normal = *cluster < 0 ? SUPER_NORMAL( x, y ) : SUPER_NORMAL( x, y - 1 );
		cluster2 = SUPER_CLUSTER( x, y );
		origin2 = SUPER_ORIGIN( x, y );
		//%	normal2 = SUPER_NORMAL( x, y );
	}
	else
		Sys_Warning( lm->entityNum, "Spurious lightmap T vector" );
	
	VectorSubtract( origin2, origin, originVecs[ 1 ] );
	//%	VectorSubtract( normal2, normal, normalVecs[ 1 ] );
	
	/* calculate new origin */
	//%	VectorMA( origin, bx, originVecs[ 0 ], sampleOrigin );
	//%	VectorMA( sampleOrigin, by, originVecs[ 1 ], sampleOrigin );
	for( i = 0; i < 3; i++ )
		sampleOrigin[ i ] = sampleOrigin[ i ] + (bx * originVecs[ 0 ][ i ]) + (by * originVecs[ 1 ][ i ]);
	
	/* get cluster */
	*sampleCluster = ClusterForPointExtFilter( sampleOrigin, (LUXEL_EPSILON * 2), lm->numLightClusters, lm->lightClusters );
	if( *sampleCluster < 0 )
		return qfalse;
	
	/* calculate new normal */
	//%	VectorMA( normal, bx, normalVecs[ 0 ], sampleNormal );
	//%	VectorMA( sampleNormal, by, normalVecs[ 1 ], sampleNormal );
	//%	if( VectorNormalize( sampleNormal, sampleNormal ) <= 0.0f )
	//%		return qfalse;
	normal = SUPER_NORMAL( x, y );
	VectorCopy( normal, sampleNormal );
	
	/* return ok */
	return qtrue;
}

/*
SubsampleRawLuxel_r()
recursively subsamples a luxel until its color gradient is low enough or subsampling limit is reached
*/

static void SubsampleRawLuxel_r( rawLightmap_t *lm, trace_t *trace, vec3_t sampleOrigin, int x, int y, float bias, float *lightLuxel )
{
	int			b, samples, mapped, lighted;
	int			cluster[ 4 ];
	vec4_t		luxel[ 4 ];
	vec3_t		origin[ 4 ], normal[ 4 ];
	float		biasDirs[ 4 ][ 2 ] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } };
	vec3_t		color, total;
	
	/* limit check */
	if( lightLuxel[ 3 ] >= lightSamples )
		return;
	
	/* setup */
	VectorClear( total );
	mapped = 0;
	lighted = 0;
	
	/* make 2x2 subsample stamp */
	for( b = 0; b < 4; b++ )
	{
		/* set origin */
		VectorCopy( sampleOrigin, origin[ b ] );
		
		/* calculate position */
		if( !SubmapRawLuxel( lm, x, y, (bias * biasDirs[ b ][ 0 ]), (bias * biasDirs[ b ][ 1 ]), &cluster[ b ], origin[ b ], normal[ b ] ) )
		{
			cluster[ b ] = -1;
			continue;
		}
		mapped++;
		
		/* increment sample count */
		luxel[ b ][ 3 ] = lightLuxel[ 3 ] + 1.0f;
		
		/* setup trace */
		trace->cluster = cluster[0];
		VectorCopy( origin[ b ], trace->origin );
		VectorCopy( normal[ b ], trace->normal );
		
		/* sample light */
		LightContribution( trace, LIGHT_SURFACES, qfalse );
		
		/* add to totals (fixme: make contrast function) */
		VectorCopy( trace->color, luxel[ b ] );
		VectorAdd( total, trace->color, total );
		if( (luxel[ b ][ 0 ] + luxel[ b ][ 1 ] + luxel[ b ][ 2 ]) > 0.0f )
			lighted++;
	}
	
	/* subsample further? */
	if( (lightLuxel[ 3 ] + 1.0f) < lightSamples && (total[ 0 ] > 4.0f || total[ 1 ] > 4.0f || total[ 2 ] > 4.0f) && lighted != 0 && lighted != mapped )
	{
		for( b = 0; b < 4; b++ )
		{
			if( cluster[ b ] < 0 )
				continue;
			SubsampleRawLuxel_r( lm, trace, origin[ b ], x, y, (bias * 0.25f), luxel[ b ] );
		}
	}
	
	/* average */
	//%	VectorClear( color );
	//%	samples = 0;
	VectorCopy( lightLuxel, color );
	samples = 1;
	for( b = 0; b < 4; b++ )
	{
		if( cluster[ b ] < 0 )
			continue;
		VectorAdd( color, luxel[ b ], color );
		samples++;
	}
	
	/* add to luxel */
	if( samples > 0 )
	{
		/* average */
		color[ 0 ] /= samples;
		color[ 1 ] /= samples;
		color[ 2 ] /= samples;
		
		/* add to color */
		VectorCopy( color, lightLuxel );
		lightLuxel[ 3 ] += 1.0f;
		lightLuxel[ 4 ] += 1.0f;
	}
}

/*
LightGridRawLightmap()
samples lightmap from lightgrid
*/

void LightGridRawLightmap(int rawLightmapNum)
{
	vec3_t ambient, diffuse, dir;
	int	i, x, y, *cluster;
	float *origin, *luxel, *normal, *deluxel, brightness, shade;
	rawLightmap_t *lm;
	qboolean twoSided;
	surfaceInfo_t *info;

	/* bail if this number exceeds the number of raw lightmaps */
	if( rawLightmapNum >= numRawLightmaps )
		return;
	
	/* get lightmap */
	lm = &rawLightmaps[rawLightmapNum];

	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	twoSided = qfalse;
	for( i = 0; i < lm->numLightSurfaces; i++ )
	{
		/* get surface */
		info = &surfaceInfos[ lightSurfaces[ i ] ];
		
		/* check twosidedness */
		if( info->si->twoSided )
		{
			twoSided = qtrue;
			break;
		}
	}

	/* sample luxels from lightgrid */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			cluster = SUPER_CLUSTER( x, y );
			luxel = SUPER_LUXEL( 0, x, y );
			origin = SUPER_ORIGIN( x, y );
			normal = SUPER_NORMAL( x, y );
			deluxel = SUPER_DELUXEL( x, y );

			/* sample */
			if( *cluster < 0 )
			{
				/* blacken unmapped clusters */
				VectorClear( luxel );
				VectorClear( deluxel );
			}
			else
			{
				/* sample grid */
				SampleGrid( origin, ambient, diffuse, dir );

				/* apply shading */
				shade = DotProduct( normal, dir );
				if (twoSided == qtrue)
					shade = fabs(shade);
				shade = max(0, shade);

				/* color the luxel */
				luxel[ 0 ] = (ambient[0] + shade * diffuse[0]) * 255.0f;
				luxel[ 1 ] = (ambient[1] + shade * diffuse[1]) * 255.0f;
				luxel[ 2 ] = (ambient[2] + shade * diffuse[2]) * 255.0f;

				/* add to light direction */
				if( deluxemap )
				{
					/* ambient */
					brightness = ambient[ 0 ] * 0.3f + ambient[ 1 ] * 0.59f + ambient[ 2 ] * 0.11f;
					VectorMA( deluxel, brightness, normal, deluxel );

					/* diffuse */
					brightness = (diffuse[ 0 ] * 0.3f + diffuse[ 1 ] * 0.59f + diffuse[ 2 ] * 0.11f) * shade;
					VectorMA( deluxel, brightness, dir, deluxel );
				}
			}
		}
	}

	/* set counts */
	numLuxelsIlluminated += (lm->sw * lm->sh);
}

/*
IlluminateRawLightmap()
illuminates the luxels
*/

#define STACK_LL_SIZE			(SUPER_LUXEL_SIZE * 64 * 64)
#define LIGHT_LUXEL( x, y )		(lightLuxels + ((((y) * lm->sw) + (x)) * SUPER_LUXEL_SIZE))

void IlluminateRawLightmap(int rawLightmapNum)
{
	int	i, t, x, y, sx, sy, size, llSize, lightmapNum, luxelFilterRadius, weight;
	int	*cluster, mapped, lighted, totalLighted;
	rawLightmap_t *lm;
	surfaceInfo_t *info;
	float *origin, *lightLuxels, *lightLuxel, *normal, *luxel, *deluxel, filterRadius, brightness, samples;
	vec3_t color, total, temp, temp2;
	float tests[ 4 ][ 2 ] = { { 0.0f, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	float averageColor[ 5 ];
	trace_t	trace;
	float stackLightLuxels[ STACK_LL_SIZE ];
	
	/* bail if this number exceeds the number of raw lightmaps */
	if( rawLightmapNum >= numRawLightmaps )
		return;

	/* illuminate by grid */
	if( gridOnly )
	{
		LightGridRawLightmap( rawLightmapNum );
		return;
	}

	/* get lightmap */
	lm = &rawLightmaps[rawLightmapNum];
	
	/* setup trace */
	trace.entityNum = lm->entityNum;
	trace.testOcclusion = noTrace ? qfalse : qtrue;
	trace.occlusionBias = 0;
	trace.forceSunlight = qfalse;
	trace.forceSelfShadow = qfalse;
	trace.recvShadows = lm->recvShadows;
	trace.numSurfaces = lm->numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
	trace.inhibitRadius = 0;
	
	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	trace.twoSided = qfalse;
	for( i = 0; i < trace.numSurfaces; i++ )
	{
		/* get surface */
		info = &surfaceInfos[ trace.surfaces[ i ] ];
		
		/* check twosidedness */
		if( info->si->twoSided )
		{
			trace.twoSided = qtrue;
			break;
		}
	}

	/* test debugging state */
	if( lightmapDebugState )
	{
		/* debug fill the luxels */
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get cluster */
				cluster = SUPER_CLUSTER( x, y );

				/* only fill mapped luxels */
				if( *cluster < 0 )
					continue;
				
				/* get particulars */
				luxel = SUPER_LUXEL( 0, x, y );
				deluxel = SUPER_DELUXEL( x, y );
				origin = SUPER_ORIGIN( x, y );
				normal = SUPER_NORMAL( x, y );

				/* set plain deluxemap */
				if( deluxemap )
					VectorScale( normal, 0.00390625f, deluxel );

				/* color the luxel with raw lightmap num? */
				if( debugSurfaces )
					VectorCopy( debugColors[ rawLightmapNum % 12 ], luxel );
				
				/* color the luxel with lightmap axis? */
				else if( debugAxis )
				{
					luxel[ 0 ] = (lm->axis[ 0 ] + 1.0f) * 127.5f;
					luxel[ 1 ] = (lm->axis[ 1 ] + 1.0f) * 127.5f;
					luxel[ 2 ] = (lm->axis[ 2 ] + 1.0f) * 127.5f;
				}
				
				/* color the luxel with luxel cluster? */
				else if( debugCluster )
					VectorCopy( debugColors[ *cluster % 12 ], luxel );
				
				/* color the luxel with luxel origin? */
				else if( debugOrigin )
				{
					VectorSubtract( lm->maxs, lm->mins, temp );
					VectorScale( temp, (1.0f / 255.0f), temp );
					VectorSubtract( origin, lm->mins, temp2 );
					luxel[ 0 ] = lm->mins[ 0 ] + (temp[ 0 ] * temp2[ 0 ]);
					luxel[ 1 ] = lm->mins[ 1 ] + (temp[ 1 ] * temp2[ 1 ]);
					luxel[ 2 ] = lm->mins[ 2 ] + (temp[ 2 ] * temp2[ 2 ]);
				}

				/* color the luxel with raw lightmap index */
				else if( debugRawLightmap )
				{
					int k = (int)CRC_Block((unsigned char *)&rawLightmapNum, sizeof(rawLightmapNum));
					luxel[ 0 ] = (k & 0xF) * (1.0f / 16.0f) * 128.0f;
					luxel[ 1 ] = (k & 0xF0) * (1.0f / 256.0f) * 128.0f;
					luxel[ 2 ] = (k & 0xF00) * (1.0f / 4096.0f) * 128.0f;
				}
				
				/* color the luxel with the normal */
				else if( normalmap )
				{
					luxel[ 0 ] = (normal[ 0 ] + 1.0f) * 127.5f;
					luxel[ 1 ] = (normal[ 1 ] + 1.0f) * 127.5f;
					luxel[ 2 ] = (normal[ 2 ] + 1.0f) * 127.5f;
				}

				/* color the luxel by lightgrid sample */
				else if( debugGrid )
				{
					vec3_t ambient, diffuse, dir;
					float shade;

					/* sample grid */
					SampleGrid( origin, ambient, diffuse, dir );

					/* apply shading */
					shade = DotProduct( normal, dir );
					if (trace.twoSided == qtrue)
						shade = fabs(shade);
					shade = max(0, shade);

					/* color the luxel */
					luxel[ 0 ] = (ambient[0] + shade * diffuse[0]) * 255.0f;
					luxel[ 1 ] = (ambient[1] + shade * diffuse[1]) * 255.0f;
					luxel[ 2 ] = (ambient[2] + shade * diffuse[2]) * 255.0f;

					/* add to light direction */
					if( deluxemap )
						VectorMA( deluxel, shade, dir, deluxel );
				}
				
				/* otherwise clear it */
				else
					VectorClear( luxel );
				
				/* add to counts */
				luxel[ 3 ] = 1.0f;
				luxel[ 4 ] += 1.0f;
			}
		}

		/* set counts */
		numLuxelsIlluminated += (lm->sw * lm->sh);

		/* return to sender */
		return;
	}

	/* create a culled light list for this raw lightmap */
	CreateTraceLightsForBounds( qfalse, lm->mins, lm->maxs, lm->plane, lm->numLightClusters, lm->lightClusters, LIGHT_SURFACES, &trace );

	/* allocate temporary per-light luxel storage */
	llSize = lm->sw * lm->sh * SUPER_LUXEL_SIZE * sizeof( float );
	if( llSize <= (STACK_LL_SIZE * sizeof( float )) )
		lightLuxels = stackLightLuxels;
	else
		lightLuxels = (float *)safe_malloc( llSize );
	
	/* clear luxels */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			cluster = SUPER_CLUSTER( x, y );
			luxel = SUPER_LUXEL( 0, x, y );
			normal = SUPER_NORMAL( x, y );
			if( deluxemap )
				deluxel = SUPER_DELUXEL( x, y );

			/* clear color */
			if( *cluster < 0 )
			{
				/* blacken unmapped clusters */
				VectorClear( luxel );
				if( deluxemap )
					VectorClear( deluxel );
			}
			else
			{
				/* set ambient color */
				VectorCopy( lm->ambient, luxel );
				if( deluxemap )
					VectorScale( normal, 0.00390625f, deluxel );
				luxel[ 3 ] = 1.0f;
			}
		}
	}
	
	/* clear styled lightmaps */
	size = lm->sw * lm->sh * SUPER_LUXEL_SIZE * sizeof( float );
	for( lightmapNum = 1; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
	{
		if( lm->superLuxels[ lightmapNum ] != NULL )
			memset( lm->superLuxels[ lightmapNum ], 0, size );
	}
	
	/* debugging code */
	//%	if( trace.numLights <= 0 )
	//%		Sys_Printf( "Lightmap %9d: 0 lights, axis: %.2f, %.2f, %.2f\n", rawLightmapNum, lm->axis[ 0 ], lm->axis[ 1 ], lm->axis[ 2 ] );
	
	/* walk light list */
	for( i = 0; i < trace.numLights; i++ )
	{
		/* setup trace */
		trace.light = trace.lights[ i ];
		
		/* style check */
		for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		{
			if( lm->styles[ lightmapNum ] == trace.light->style ||
				lm->styles[ lightmapNum ] == LS_NONE )
				break;
		}
		
		/* max of MAX_LIGHTMAPS (4) styles allowed to hit a surface/lightmap */
		if( lightmapNum >= MAX_LIGHTMAPS )
		{
			Sys_Warning( lm->entityNum, "Hit per-surface style limit (%d)", MAX_LIGHTMAPS );
			continue;
		}
		
		/* setup */
		memset( lightLuxels, 0, llSize );
		totalLighted = 0;
		
		/* initial pass, one sample per luxel */
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get cluster */
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;
				
				/* get particulars */
				lightLuxel = LIGHT_LUXEL( x, y );
				if( deluxemap )
					deluxel = SUPER_DELUXEL( x, y );
				origin = SUPER_ORIGIN( x, y );
				normal = SUPER_NORMAL( x, y );

				/* set contribution count */
				lightLuxel[ 3 ] = 1.0f;

				/* setup trace */
				trace.cluster = *cluster;
				VectorCopy( origin, trace.origin );
				VectorCopy( normal, trace.normal );
					
				/* get light for this sample */
				LightContribution( &trace, LIGHT_SURFACES, qfalse );
				VectorCopy( trace.color, lightLuxel );

				/* add to count */
				if( trace.color[ 0 ] || trace.color[ 1 ] || trace.color[ 2 ] )
				{
					lightLuxel[ 4 ] += 1.0f;
					totalLighted++;
				}
			
				/* add to light direction map */
				if( deluxemap )
				{
					/* vortex: use noShadow color */
					/* color to grayscale */
					brightness = trace.colorNoShadow[ 0 ] * 0.3f + trace.colorNoShadow[ 1 ] * 0.59f + trace.colorNoShadow[ 2 ] * 0.11f;
					brightness *= (1.0 / 255.0);
					VectorScale( trace.direction, brightness, trace.direction );
					if( deluxemap )
						VectorAdd( deluxel, trace.direction, deluxel );
				}
			}
		}
		
		/* don't even bother with everything else if nothing was lit */
		if( totalLighted == 0 )
			continue;
		
		/* determine filter radius */
		filterRadius = lm->filterRadius > trace.light->filterRadius ? lm->filterRadius : trace.light->filterRadius;
		if( filterRadius < 0.0f )
			filterRadius = 0.0f;
		
		/* set luxel filter radius */
		luxelFilterRadius = superSample * filterRadius / lm->sampleSize;
		if( luxelFilterRadius == 0 && (filterRadius > 0.0f || filter) )
			luxelFilterRadius = 1;
		
		/* secondary pass, adaptive supersampling (fixme: use a contrast function to determine if subsampling is necessary) */
		/* 2003-09-27: changed it so filtering disamples supersampling, as it would waste time */
		if( lightSamples > 1 && luxelFilterRadius == 0 )
		{
			/* walk luxels */
			for( y = 0; y < (lm->sh - 1); y++ )
			{
				for( x = 0; x < (lm->sw - 1); x++ )
				{
					/* setup */
					mapped = 0;
					lighted = 0;
					VectorClear( total );
					
					/* test 2x2 stamp */
					for( t = 0; t < 4; t++ )
					{
						/* set sample coords */
						sx = x + tests[ t ][ 0 ];
						sy = y + tests[ t ][ 1 ];
						
						/* get cluster */
						cluster = SUPER_CLUSTER( sx, sy );
						if( *cluster < 0 )
							continue;
						mapped++;
						
						/* get luxel */
						lightLuxel = LIGHT_LUXEL( sx, sy );
						VectorAdd( total, lightLuxel, total );
						if( (lightLuxel[ 0 ] + lightLuxel[ 1 ] + lightLuxel[ 2 ]) > 0.0f )
							lighted++;
					}
					
					/* if total color is under a certain amount, then don't bother subsampling */
					if( total[ 0 ] <= 4.0f && total[ 1 ] <= 4.0f && total[ 2 ] <= 4.0f )
						continue;
					
					/* if all 4 pixels are either in shadow or light, then don't subsample */
					if( lighted != 0 && lighted != mapped )
					{
						for( t = 0; t < 4; t++ )
						{
							/* set sample coords */
							sx = x + tests[ t ][ 0 ];
							sy = y + tests[ t ][ 1 ];
							
							/* get luxel */
							cluster = SUPER_CLUSTER( sx, sy );
							if( *cluster < 0 )
								continue;
							lightLuxel = LIGHT_LUXEL( sx, sy );
							origin = SUPER_ORIGIN( sx, sy );
							
							/* only subsample shadowed luxels */
							//%	if( (lightLuxel[ 0 ] + lightLuxel[ 1 ] + lightLuxel[ 2 ]) <= 0.0f )
							//%		continue;
							
							/* subsample it */
							SubsampleRawLuxel_r( lm, &trace, origin, sx, sy, 0.25f, lightLuxel );
							
							/* debug code to colorize subsampled areas to yellow */
							//%	luxel = SUPER_LUXEL( lightmapNum, sx, sy );
							//%	VectorSet( luxel, 255, 204, 0 );
						}
					}
				}
			}
		}
		
		/* allocate sampling lightmap storage */
		if( lm->superLuxels[ lightmapNum ] == NULL )
		{
			/* allocate sampling lightmap storage */
			size = lm->sw * lm->sh * SUPER_LUXEL_SIZE * sizeof( float );
			lm->superLuxels[ lightmapNum ] = (float *)safe_malloc( size );
			memset( lm->superLuxels[ lightmapNum ], 0, size );
		}
		
		/* set style */
		if( lightmapNum > 0 )
		{
			lm->styles[ lightmapNum ] = trace.light->style;
			//%	Sys_Printf( "Surface %6d has lightstyle %d\n", rawLightmapNum, trace.light->style );
		}
		
		/* copy to permanent luxels */
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get cluster and origin */
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;
				origin = SUPER_ORIGIN( x, y );
				
				/* filter? */
				if( luxelFilterRadius )
				{
					/* setup */
					VectorClear( averageColor );
					averageColor[ 4 ] = 0.0f;
					samples = 0.0f;
					
					/* cheaper distance-based filtering */
					for( sy = (y - luxelFilterRadius); sy <= (y + luxelFilterRadius); sy++ )
					{
						if( sy < 0 || sy >= lm->sh )
							continue;
						
						for( sx = (x - luxelFilterRadius); sx <= (x + luxelFilterRadius); sx++ )
						{
							if( sx < 0 || sx >= lm->sw )
								continue;
							
							/* get particulars */
							cluster = SUPER_CLUSTER( sx, sy );
							if( *cluster < 0 )
								continue;
							lightLuxel = LIGHT_LUXEL( sx, sy );
							
							/* create weight */
							weight = (abs( sx - x ) == luxelFilterRadius ? 0.5f : 1.0f);
							weight *= (abs( sy - y ) == luxelFilterRadius ? 0.5f : 1.0f);
							
							/* scale luxel by filter weight */
							VectorScale( lightLuxel, weight, color );
							VectorAdd( averageColor, color, averageColor );
							averageColor[ 4 ] += lightLuxel[ 4 ];
							samples += weight;
						}
					}
					
					/* any samples? */
					if( samples <= 0.0f	)
						continue;
					
					/* scale into luxel */
					luxel = SUPER_LUXEL( lightmapNum, x, y );
					luxel[ 3 ] = 1.0f;
					luxel[ 4 ] = averageColor[ 4 ] / samples;
					
					/* handle negative light */
					if( trace.light->flags & LIGHT_NEGATIVE )
					{ 
						luxel[ 0 ] -= averageColor[ 0 ] / samples;
						luxel[ 1 ] -= averageColor[ 1 ] / samples;
						luxel[ 2 ] -= averageColor[ 2 ] / samples;
					}
					
					/* handle normal light */
					else
					{ 
						luxel[ 0 ] += averageColor[ 0 ] / samples;
						luxel[ 1 ] += averageColor[ 1 ] / samples;
						luxel[ 2 ] += averageColor[ 2 ] / samples;
					}
				}
				
				/* single sample */
				else
				{
					/* get particulars */
					lightLuxel = LIGHT_LUXEL( x, y );
					luxel = SUPER_LUXEL( lightmapNum, x, y );
					
					/* handle negative light */
					if( trace.light->flags & LIGHT_NEGATIVE )
						VectorNegate( averageColor, averageColor );

					/* add color */
					luxel[ 3 ] = 1.0f;
					luxel[ 4 ] += lightLuxel[ 4 ];
					
					/* handle negative light */
					if( trace.light->flags & LIGHT_NEGATIVE )
						VectorSubtract( luxel, lightLuxel, luxel );
					
					/* handle normal light */
					else
						VectorAdd( luxel, lightLuxel, luxel );
				}
			}
		}
	}
	
	/* free temporary luxels */
	if( lightLuxels != stackLightLuxels )
		free( lightLuxels );

	/* free light list */
	FreeTraceLights( &trace );

	/* set counts */
	numLuxelsIlluminated += (lm->sw * lm->sh);
}

/*
FilterRawLightmap
applies postprocess filtering on luxels
*/

void FilterRawLightmap(int rawLightmapNum)
{
	int x, y, sx, sy, lightmapNum, *cluster, *cluster2;
	rawLightmap_t *lm;
	qboolean filterColor, filterDir;
	float *normal, *normal2, *dirt, *luxel, *luxel2, *deluxel, *deluxel2, *floodlight, brightness, samples;
	vec3_t averageColor, averageDir, averageNorm, lightvector;
	float tests[ 4 ][ 2 ] = { { 0.0f, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
	
	/* get lightmap */
	lm = &rawLightmaps[ rawLightmapNum ];

	/* stitch */
	if( !gridOnly && lm->stitch == qtrue && noStitch == qfalse )
		StitchRawLightmap( rawLightmapNum );

	/* apply floodlight */
	for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
	{
		/* early out */
		if( lm->superLuxels[ lightmapNum ] == NULL )
			continue;

		/* apply floodlight to each luxel */
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get floodlight */
				floodlight = SUPER_FLOODLIGHT( x, y );
				if (!floodlight[0] && !floodlight[1] && !floodlight[2])
					continue;
						
				/* get cluster */
				cluster	= SUPER_CLUSTER( x, y );

				/* only process mapped luxels */
				if( *cluster < 0 )
					continue;

				/* get particulars */
				luxel = SUPER_LUXEL( lightmapNum, x, y );
				deluxel = SUPER_DELUXEL( x, y );

				/* add to lightmap */
				luxel[0] += floodlight[0];
				luxel[1] += floodlight[1];
				luxel[2] += floodlight[2];
				if (luxel[ 3 ] == 0)
					luxel[ 3 ] = 1.0f;

				/* add to deluxemap */
				/* todo: implement different direction handling (most 'free' space) */
				if (deluxemap && floodlight[3] > 0)
				{
					normal = SUPER_NORMAL( x, y );
					brightness = floodlight[ 0 ] * 0.3f + floodlight[ 1 ] * 0.59f + floodlight[ 2 ] * 0.11f;
					brightness *= ( 1.0f / 255.0f ) * floodlight[3];
					VectorScale( normal, brightness, lightvector );
					VectorAdd( deluxel, lightvector, deluxel );
				}
			}
		}
	}

	/* apply dirt */
	if( dirtSettings[lm->entityNum].enabled || dirtSettings[0].enabled )
	{
		/* walk lightmaps */
		for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		{
			/* early out */
			if( lm->superLuxels[ lightmapNum ] == NULL )
				continue;

			/* apply dirt to each luxel */
			for( y = 0; y < lm->sh; y++ )
			{
				for( x = 0; x < lm->sw; x++ )
				{
					/* get cluster */
					cluster	= SUPER_CLUSTER( x, y );

					/* get particulars */
					luxel = SUPER_LUXEL( lightmapNum, x, y );
					dirt = SUPER_DIRT( x, y );
					
					/* apply dirt */
					ApplyDirt(&luxel[0], &luxel[1], &luxel[2], dirt, lm->entityNum, lm->aoScale, lm->aoScale * lm->aoGainScale);
				}
			}
		}
	}

	/* debug normals pass */
	if (debugnormals)
	{
		for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		{
			/* early out */
			if( lm->superLuxels[ lightmapNum ] == NULL )
				continue;
			
			for( y = 0; y < lm->sh; y++ )
			{
				for( x = 0; x < lm->sw; x++ )
				{
					/* get cluster */
					cluster	= SUPER_CLUSTER( x, y );
					//%	if( *cluster < 0 )
					//%		continue;
					
					/* get particulars */
					luxel = SUPER_LUXEL( lightmapNum, x, y );
					normal = SUPER_NORMAL (  x, y );
               
					luxel[0]=(normal[0]*127)+127;
					luxel[1]=(normal[1]*127)+127;
					luxel[2]=(normal[2]*127)+127;
				}
			}
		}
	}

	/* filter pass */
	for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
	{
		/* early out */
		if( lm->superLuxels[ lightmapNum ] == NULL )
			continue;
		
		/* average occluded luxels from neighbors */
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* get particulars */
				cluster = SUPER_CLUSTER( x, y );
				luxel = SUPER_LUXEL( lightmapNum, x, y );
				deluxel = SUPER_DELUXEL( x, y);
				normal = SUPER_NORMAL( x, y);
				
				/* determine if filtering is necessary */
				filterColor = qfalse;
				filterDir = qfalse;
				if( *cluster < 0 || (lm->splotchFix && (luxel[ 0 ] <= lm->ambient[ 0 ] || luxel[ 1 ] <= lm->ambient[ 1 ] || luxel[ 2 ] <= lm->ambient[ 2 ])) )
					filterColor = qtrue;
				if( deluxemap && lightmapNum == 0 && (*cluster < 0 || filter) )
					filterDir = qtrue;
				
				if( !filterColor && !filterDir )
					continue;
				
				/* choose seed amount */
				VectorClear( averageColor );
				VectorClear( averageDir );
				VectorClear( averageNorm );

				/* walk 3x3 matrix */
				samples = 0.0f;
				for( sy = (y - 1); sy <= (y + 1); sy++ )
				{
					if( sy < 0 || sy >= lm->sh )
						continue;
					
					for( sx = (x - 1); sx <= (x + 1); sx++ )
					{
						if( sx < 0 || sx >= lm->sw || (sx == x && sy == y) )
							continue;
						
						/* get neighbor's particulars */
						cluster2 = SUPER_CLUSTER( sx, sy );
						luxel2 = SUPER_LUXEL( lightmapNum, sx, sy );
						deluxel2 = SUPER_DELUXEL( sx, sy );
						normal2 = SUPER_NORMAL( sx, sy );
						
						/* ignore unmapped/unlit luxels */
						if( *cluster2 < 0 || luxel2[ 3 ] == 0.0f || (lm->splotchFix && VectorCompare( luxel2, lm->ambient )) )
							continue;
						
						/* add its distinctiveness to our own */
						VectorAdd( averageColor, luxel2, averageColor );
						samples += luxel2[ 3 ];
						if( filterDir )
						{
							VectorAdd( averageDir, deluxel2, averageDir );
							VectorAdd( averageNorm, normal2, averageNorm );
						}
					}
				}
				
				/* fall through */
				if( samples <= 0.0f )
					continue;

				/* average it */
				if( filterColor )
				{
					VectorDivide( averageColor, samples, luxel );
					luxel[ 3 ] = 1.0f;
				}
				if( filterDir )
				{
					VectorDivide( averageDir, samples, deluxel );
					VectorDivide( averageNorm, samples, normal );
				}
			
				/* set cluster to -3 */
				if( *cluster < 0 )
					*cluster = CLUSTER_FLOODED;
			}
		}
	}
}

/*
WriteRawLightmapsFile()
creates debug drawsurfaces for raw lightmaps
*/

void WriteRawLightmapsFile( const char *outFile )
{
	rawLightmap_t *lm;
	int i, x, y, fOld, start, *cluster, numTris, numVerts;
	int  addedDrawSurfaces = 0, addedDrawTriangles = 0, addedDrawVerts = 0;
	float *origin, *normal;
	vec3_t base, org, right, up;
	char *shaderName;
	byte color[4];
	FILE *ase;

	/* note it */
	Sys_PrintHeading ( "--- DebugRawLightmap ---\n" );

	/* open it */
	ase = fopen( outFile, "wb" );
	if( ase == NULL )
		Error( "Open failed on %s\n", outFile );
	Sys_Printf( "writing %s\n", outFile );

	/* print header */
	fprintf( ase, "*3DSMAX_ASCIIEXPORT\t200\r\n" );
	fprintf( ase, "*COMMENT\t\"Generated by WzMap -light -debuglightmap\"\r\n" );
	fprintf( ase, "*SCENE\t{\r\n" );
	fprintf( ase, "\t*SCENE_FILENAME\t\"%s\"\r\n", "luxels" );
	fprintf( ase, "\t*SCENE_FIRSTFRAME\t0\r\n" );
	fprintf( ase, "\t*SCENE_LASTFRAME\t100\r\n" );
	fprintf( ase, "\t*SCENE_FRAMESPEED\t30\r\n" );
	fprintf( ase, "\t*SCENE_TICKSPERFRAME\t160\r\n" );
	fprintf( ase, "\t*SCENE_BACKGROUND_STATIC\t0.0000\t0.0000\t0.0000\r\n" );
	fprintf( ase, "\t*SCENE_AMBIENT_STATIC\t0.0000\t0.0000\t0.0000\r\n" );
	fprintf( ase, "}\r\n" );

	/* print material */
	fprintf( ase, "*MATERIAL_LIST\t{\r\n" );
	fprintf( ase, "\t*MATERIAL_COUNT\t%d\r\n", 1 );
	fprintf( ase, "\t*MATERIAL\t%d\t{\r\n", 0 );
	fprintf( ase, "\t\t*MATERIAL_NAME\t\"%s\"\r\n", "noshader" );
	fprintf( ase, "\t\t*MATERIAL_CLASS\t\"Standard\"\r\n" );
	fprintf( ase, "\t\t*MATERIAL_DIFFUSE\t%f\t%f\t%f\r\n", 1, 1, 1 );
	fprintf( ase, "\t\t*MATERIAL_SHADING Phong\r\n" );
	fprintf( ase, "\t\t*MAP_DIFFUSE\t{\r\n" );
	fprintf( ase, "\t\t\t*MAP_NAME\t\"%s\"\r\n", "noshader" );
	fprintf( ase, "\t\t\t*MAP_CLASS\t\"Bitmap\"\r\n");
	fprintf( ase, "\t\t\t*MAP_SUBNO\t1\r\n" );
	fprintf( ase, "\t\t\t*MAP_AMOUNT\t1.0\r\n" );
	fprintf( ase, "\t\t\t*MAP_TYPE\tScreen\r\n" );
	fprintf( ase, "\t\t\t*BITMAP\t\"..\\%s\"\r\n", "noshader" );
	fprintf( ase, "\t\t\t*BITMAP_FILTER\tPyramidal\r\n" );
	fprintf( ase, "\t\t}\r\n" );
	fprintf( ase, "\t}\r\n" );
	fprintf( ase, "}\r\n" );

	/* start pacifier */
	fOld = -1;
	start = I_FloatTime();

	/* generate geomobjects for raw lightmaps */
	for( i = 0; i < numRawLightmaps; i++ )
	{
		printLabelledProgress("DebugRawLightmap", i, numRawLightmaps);

		/* get lightmap */
		lm = &rawLightmaps[ i ];

		/* calc stats */
		numTris = 0;
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;
				numTris++;
			}
		}

		/* any luxels? */
		//if( numTris == 0 )
		//	continue;

		/* add to global stats */
		addedDrawSurfaces++;
		addedDrawTriangles += numTris;
		addedDrawVerts += numTris * 3;

		/* get metrial name */
		shaderName = "noshader";
		if( surfaceInfos[ lightSurfaces[ lm->firstLightSurface ] ].si != NULL )
			shaderName = surfaceInfos[ lightSurfaces[ lm->firstLightSurface ] ].si->shader;

		/* print object header */
		fprintf( ase, "*GEOMOBJECT\t{\r\n" );
		fprintf( ase, "\t*NODE_NAME\t\"%s_%i\"\r\n", shaderName, i );
		fprintf( ase, "\t*NODE_TM\t{\r\n" );
		fprintf( ase, "\t\t*NODE_NAME\t\"%s_%i\"\r\n", shaderName, i );
		fprintf( ase, "\t\t*INHERIT_POS\t0\t0\t0\r\n" );
		fprintf( ase, "\t\t*INHERIT_ROT\t0\t0\t0\r\n" );
		fprintf( ase, "\t\t*INHERIT_SCL\t0\t0\t0\r\n" );
		fprintf( ase, "\t\t*TM_ROW0\t1.0\t0\t0\r\n" );
		fprintf( ase, "\t\t*TM_ROW1\t0\t1.0\t0\r\n" );
		fprintf( ase, "\t\t*TM_ROW2\t0\t0\t1.0\r\n" );
		fprintf( ase, "\t\t*TM_ROW3\t0\t0\t0\r\n" );
		fprintf( ase, "\t\t*TM_POS\t%f\t%f\t%f\r\n", 0, 0, 0 );
		fprintf( ase, "\t}\r\n" );

		/* print mesh header */
		fprintf( ase, "\t*MESH\t{\r\n" );
		fprintf( ase, "\t\t*TIMEVALUE\t0\r\n" );

		/* vertex coords list */
		numVerts = 0;
		fprintf( ase, "\t\t*MESH_NUMVERTEX\t%d\r\n", numTris * 3 );
		fprintf( ase, "\t\t*MESH_NUMFACES\t%d\r\n", numTris );
		fprintf( ase, "\t\t*MESH_VERTEX_LIST\t{\r\n" );
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				/* only valid luxels */
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;

				/* get particulars */
#if 1
				origin = SUPER_ORIGIN( x, y );
				VectorCopy( origin, base );
				normal = SUPER_NORMAL( x, y );
#else
				/* original sample position */
				origin = SUPER_TRIORIGIN( x, y );
				normal = SUPER_TRINORMAL( x, y );
				VectorAdd( origin, normal, base );
				normal = SUPER_NORMAL( x, y );
#endif
				/* create verts */
				MakeNormalVectors( normal, right, up );
				fprintf( ase, "\t\t\t*MESH_VERTEX\t%d\t%f\t%f\t%f\r\n", numVerts++, base[ 0 ], base[ 1 ], base[ 2 ] );
				VectorMA( base, lm->actualSampleSize, up, org );
				fprintf( ase, "\t\t\t*MESH_VERTEX\t%d\t%f\t%f\t%f\r\n", numVerts++, org[ 0 ], org[ 1 ], org[ 2 ] );
				VectorMA( base, lm->actualSampleSize, right, org );
				fprintf( ase, "\t\t\t*MESH_VERTEX\t%d\t%f\t%f\t%f\r\n", numVerts++, org[ 0 ], org[ 1 ], org[ 2 ] );
			}
		}
		fprintf( ase, "\t\t}\r\n" );

		/* face list */
		fprintf( ase, "\t\t*MESH_FACE_LIST\t{\r\n" );
		for( x = 0; x < numVerts; x += 3 )
			fprintf( ase, "\t\t\t*MESH_FACE\t%d\tA:\t%d\tB:\t%d\tC:\t%d\tAB:\t1\tBC:\t1\tCA:\t1\t*MESH_SMOOTHING\t0\t*MESH_MTLID\t0\r\n", x / 3, x, x + 1, x + 2 );
		fprintf( ase, "\t\t}\r\n" );

		/* vertex normals */
		numVerts = 0;
		fprintf( ase, "\t\t*MESH_NORMALS\t{\r\n" );
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;
				normal = SUPER_NORMAL( x, y );
				fprintf( ase, "\t\t\t*MESH_FACENORMAL\t%d\t%f\t%f\t%f\r\n", numVerts / 3, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
				fprintf( ase, "\t\t\t\t*MESH_VERTEXNORMAL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
				fprintf( ase, "\t\t\t\t*MESH_VERTEXNORMAL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
				fprintf( ase, "\t\t\t\t*MESH_VERTEXNORMAL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
			}
		}
		fprintf( ase, "\t\t}\r\n" );

		/* vertex color list */
		fprintf( ase, "\t\t*MESH_NUMCVERTEX\t%d\r\n", numVerts );
		fprintf( ase, "\t\t*MESH_CVERTLIST\t{\r\n" );
		numVerts = 0;
		for( y = 0; y < lm->sh; y++ )
		{
			for( x = 0; x < lm->sw; x++ )
			{
				cluster = SUPER_CLUSTER( x, y );
				if( *cluster < 0 )
					continue;
				ColorToBytes( SUPER_LUXEL( 0, x, y ), color, 1.0f, qfalse );
				normal[ 0 ] = (float)color[ 0 ] / 255.0f;
				normal[ 1 ] = (float)color[ 1 ] / 255.0f;
				normal[ 2 ] = (float)color[ 2 ] / 255.0f;
				fprintf( ase, "\t\t\t*MESH_VERTCOL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
				fprintf( ase, "\t\t\t*MESH_VERTCOL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
				fprintf( ase, "\t\t\t*MESH_VERTCOL\t%d\t%f\t%f\t%f\r\n", numVerts++, normal[ 0 ], normal[ 1 ], normal[ 2 ] );
			}
		}
		fprintf( ase, "\t\t}\r\n" );

		/* mesh footer */
		fprintf( ase, "\t}\r\n" );

		/* object footer */
		fprintf( ase, "\t*PROP_MOTIONBLUR\t0\r\n" );
		fprintf( ase, "\t*PROP_CASTSHADOW\t1\r\n" );
		fprintf( ase, "\t*PROP_RECVSHADOW\t1\r\n" );
		fprintf( ase, "\t*MATERIAL_REF\t%d\r\n", 0 );
		fprintf( ase, "}\r\n" );
	}

	/* print time */
	//Sys_Printf (" (%d)\n", (int) (I_FloatTime() - start) );

	/* emit some statistics */
	Sys_Printf( "%9d objects\n", addedDrawSurfaces );
	Sys_Printf( "%9d triangles\n", addedDrawTriangles );
	Sys_Printf( "%9d verts\n", addedDrawVerts );

	/* close the file and return */
	fclose( ase );
}

/*
IlluminateVertexes()
light the surface vertexes
*/

#define VERTEX_NUDGE	4.0f

void IlluminateVertexes(int num)
{
	int					i, x, y, z, x1, y1, z1, sx, sy, radius, maxRadius, *cluster;
	int					lightmapNum, numAvg;
	float				samples, *vertLuxel, *radVertLuxel, *luxel, dirt;
	vec3_t				origin, temp, temp2, colors[ MAX_LIGHTMAPS ], avgColors[ MAX_LIGHTMAPS ];
	bspDrawSurface_t	*ds;
	surfaceInfo_t		*info;
	rawLightmap_t		*lm;
	bspDrawVert_t		*verts;
	trace_t				trace;
	
	
	/* get surface, info, and raw lightmap */
	ds = &bspDrawSurfaces[ num ];
	info = &surfaceInfos[ num ];
	lm = info->lm;

	/* no vertex light */
	if (info->si->noVertexLight)
		return;

	/* -----------------------------------------------------------------
	   gridOnly mode (sample vertex lighting from lightgrid)
	   ----------------------------------------------------------------- */
	if( gridOnly )
	{
		vec3_t ambient, diffuse, dir;
		float shade;

		/* setup */
		verts = yDrawVerts + ds->firstVert;

		/* walk the surface verts */
		for( i = 0; i < ds->numVerts; i++ )
		{
			/* get vertex luxels */
			vertLuxel = VERTEX_LUXEL( 0, ds->firstVert + i );
			radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );

			/* sample grid */
			SampleGrid( verts[ i ].xyz, ambient, diffuse, dir );

			/* apply shading */
			shade = 1.0f;
			if( !info->si->vertexPointSample )
			{
				shade = DotProduct( verts[ i ].normal, dir );
				if (info->si->twoSided == qtrue)
					shade = fabs(shade);
				shade = max(0, shade);
			}

			/* color the luxel */
			vertLuxel[ 0 ] = radVertLuxel[ 0 ] = (ambient[0] + shade * diffuse[0]) * 255.0f;
			vertLuxel[ 1 ] = radVertLuxel[ 1 ] = (ambient[1] + shade * diffuse[1]) * 255.0f;
			vertLuxel[ 2 ] = radVertLuxel[ 2 ] = (ambient[2] + shade * diffuse[2]) * 255.0f;

			/* store it */
			ColorToBytes( vertLuxel, verts[ i ].color[ 0 ], 1.0f, qfalse );

			/* another happy customer */
			numVertsIlluminated++;
		}

		/* return to sender */
		return;
	}

	/* -----------------------------------------------------------------
	   illuminate the vertexes
	   ----------------------------------------------------------------- */
	
	/* calculate vertex lighting for surfaces without lightmaps */
	if( lm == NULL || cpmaHack )
	{
		/* setup trace */
		trace.entityNum = info->entityNum;
		trace.testOcclusion = (cpmaHack && lm != NULL) ? qfalse : (noTrace ? qfalse : qtrue);
		trace.occlusionBias = info->si->vertexOcclusionBias;
		trace.forceSunlight = info->si->forceSunlight ? qtrue : qfalse;
		trace.forceSelfShadow = qfalse;
		trace.recvShadows = info->recvShadows;
		trace.numSurfaces = 1;
		trace.surfaces = &num;
		trace.inhibitRadius = 0;

		/* vertexShadows */
		if (info->si->vertexShadows == qfalse)
			trace.testOcclusion = qfalse;
		
		/* twosided lighting */
		trace.twoSided = info->si->twoSided ? qtrue : qfalse;
		
		/* make light list for this surface */
		CreateTraceLightsForSurface( num, &trace );
		
		/* setup */
		verts = yDrawVerts + ds->firstVert;
		numAvg = 0;
		memset( avgColors, 0, sizeof( avgColors ) );
		
		/* walk the surface verts */
		for( i = 0; i < ds->numVerts; i++ )
		{
			/* get vertex luxel */
			radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );
			
			/* color the luxel with raw lightmap num? */
			if( debugSurfaces )
				VectorCopy( debugColors[ num % 12 ], radVertLuxel );
			
			/* color the luxel with luxel origin? */
			else if( debugOrigin )
			{
				VectorSubtract( info->maxs, info->mins, temp );
				VectorScale( temp, (1.0f / 255.0f), temp );
				VectorSubtract( origin, lm->mins, temp2 );
				radVertLuxel[ 0 ] = info->mins[ 0 ] + (temp[ 0 ] * temp2[ 0 ]);
				radVertLuxel[ 1 ] = info->mins[ 1 ] + (temp[ 1 ] * temp2[ 1 ]);
				radVertLuxel[ 2 ] = info->mins[ 2 ] + (temp[ 2 ] * temp2[ 2 ]);
			}
			
			/* color the luxel with the normal */
			else if( normalmap )
			{
				radVertLuxel[ 0 ] = (verts[ i ].normal[ 0 ] + 1.0f) * 127.5f;
				radVertLuxel[ 1 ] = (verts[ i ].normal[ 1 ] + 1.0f) * 127.5f;
				radVertLuxel[ 2 ] = (verts[ i ].normal[ 2 ] + 1.0f) * 127.5f;
			}
			
			/* illuminate the vertex */
			else
			{
				/* clear vertex luxel */
				VectorSet( radVertLuxel, -1.0f, -1.0f, -1.0f );
				
				/* try at initial origin */
				trace.cluster = ClusterForPointExtFilter( verts[ i ].xyz, VERTEX_EPSILON, info->numSurfaceClusters, &surfaceClusters[ info->firstSurfaceCluster ] );
				if( trace.cluster >= 0 )
				{
					/* setup trace */
					VectorCopy( verts[ i ].xyz, trace.origin );
					VectorCopy( verts[ i ].normal, trace.normal );
					
					/* dirty */
					if( dirty )
						dirt = DirtForSample( &trace );

					/* trace */
					LightContributionAllStyles( &trace, ds->vertexStyles, colors, LIGHT_SURFACES, info->si->vertexPointSample ? qtrue : qfalse );
					
					/* store */
					for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
					{
						radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
						VectorCopy( colors[ lightmapNum ], radVertLuxel );
						VectorAdd( avgColors[ lightmapNum ], colors[ lightmapNum ], colors[ lightmapNum ] );
					}
				}
				
				/* is this sample bright enough? */
				radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );
				if( radVertLuxel[ 0 ] <= info->ambient[ 0 ] && radVertLuxel[ 1 ] <= info->ambient[ 1 ] && radVertLuxel[ 2 ] <= info->ambient[ 2 ] )
				{
					/* nudge the sample point around a bit */
					for( x = 0; x < 4; x++ )
					{
						/* two's complement 0, 1, -1, 2, -2, etc */
						x1 = ((x >> 1) ^ (x & 1 ? -1 : 0)) + (x & 1);
						
						for( y = 0; y < 4; y++ )
						{
							y1 = ((y >> 1) ^ (y & 1 ? -1 : 0)) + (y & 1);
							
							for( z = 0; z < 4; z++ )
							{
								z1 = ((z >> 1) ^ (z & 1 ? -1 : 0)) + (z & 1);
								
								/* nudge origin */
								trace.origin[ 0 ] = verts[ i ].xyz[ 0 ] + (VERTEX_NUDGE * x1);
								trace.origin[ 1 ] = verts[ i ].xyz[ 1 ] + (VERTEX_NUDGE * y1);
								trace.origin[ 2 ] = verts[ i ].xyz[ 2 ] + (VERTEX_NUDGE * z1);
								
								/* try at nudged origin */
								trace.cluster = ClusterForPointExtFilter( origin, VERTEX_EPSILON, info->numSurfaceClusters, &surfaceClusters[ info->firstSurfaceCluster ] );
								if( trace.cluster < 0 )
									continue;
															
								/* trace */
								LightContributionAllStyles( &trace, ds->vertexStyles, colors, LIGHT_SURFACES, info->si->vertexPointSample ? qtrue : qfalse );
								
								/* store */
								for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
								{
									/* r7 dirt */
									VectorScale( colors[ lightmapNum ], dirt, colors[ lightmapNum ] );
									
									/* store */
									radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
									VectorCopy( colors[ lightmapNum ], radVertLuxel );
								}
								
								/* bright enough? */
								radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );
								if( radVertLuxel[ 0 ] > info->ambient[ 0 ] || radVertLuxel[ 1 ] > info->ambient[ 1 ] || radVertLuxel[ 2 ] > info->ambient[ 2 ] )
									x = y = z = 1000;
							}
						}
					}
				}
				
				/* add to average? */
				radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );
				if( radVertLuxel[ 0 ] > info->ambient[ 0 ] || radVertLuxel[ 1 ] > info->ambient[ 1 ] || radVertLuxel[ 2 ] > info->ambient[ 2 ] )
				{
					numAvg++;
					for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
					{
						radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
						VectorAdd( avgColors[ lightmapNum ], radVertLuxel, avgColors[ lightmapNum ] );
					}
				}
			}
			
			/* another happy customer */
			numVertsIlluminated++;
		}
		
		/* set average color */
		if( numAvg > 0 )
		{
			for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
				VectorScale( avgColors[ lightmapNum ], (1.0f / numAvg), avgColors[ lightmapNum ] );
		}
		else
		{
			/* set ambient color */
			VectorCopy( info->ambient, avgColors[ 0 ] );
		}
		
		/* clean up and store vertex color */
		for( i = 0; i < ds->numVerts; i++ )
		{
			/* get vertex luxel */
			radVertLuxel = RAD_VERTEX_LUXEL( 0, ds->firstVert + i );
			
			/* store average in occluded vertexes */
			if( radVertLuxel[ 0 ] < 0.0f )
			{
				for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
				{
					radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
					VectorCopy( avgColors[ lightmapNum ], radVertLuxel );
					
					/* debug code */
					//%	VectorSet( radVertLuxel, 255.0f, 0.0f, 0.0f );
				}
			}
			
			/* store it */
			for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
			{
				/* get luxels */
				vertLuxel = VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
				radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
				
				/* store */
				if( bouncing || bounce == 0 || !bounceOnly )
					VectorAdd( vertLuxel, radVertLuxel, vertLuxel );
				ColorToBytes( vertLuxel, verts[ i ].color[ lightmapNum ], info->si->vertexScale * vertexScale, qfalse );
			}
		}
		
		/* free light list */
		FreeTraceLights( &trace );
		
		/* return to sender */
		return;
	}
	
	/* -----------------------------------------------------------------
	   reconstitute vertex lighting from the luxels
	   ----------------------------------------------------------------- */
	
	/* set styles from lightmap */
	for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		ds->vertexStyles[ lightmapNum ] = lm->styles[ lightmapNum ];
	
	/* get max search radius */
	maxRadius = lm->sw;
	maxRadius = maxRadius > lm->sh ? maxRadius : lm->sh;
	
	/* walk the surface verts */
	verts = yDrawVerts + ds->firstVert;
	for( i = 0; i < ds->numVerts; i++ )
	{
		/* do each lightmap */
		for( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		{
			/* early out */
			if( lm->superLuxels[ lightmapNum ] == NULL )
				continue;
			
			/* get luxel coords */
			x = verts[ i ].lightmap[ lightmapNum ][ 0 ];
			y = verts[ i ].lightmap[ lightmapNum ][ 1 ];
			if( x < 0 )
				x = 0;
			else if( x >= lm->sw )
				x = lm->sw - 1;
			if( y < 0 )
				y = 0;
			else if( y >= lm->sh )
				y = lm->sh - 1;
			
			/* get vertex luxels */
			vertLuxel = VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
			radVertLuxel = RAD_VERTEX_LUXEL( lightmapNum, ds->firstVert + i );
			
			/* color the luxel with the normal? */
			if( normalmap )
			{
				radVertLuxel[ 0 ] = (verts[ i ].normal[ 0 ] + 1.0f) * 127.5f;
				radVertLuxel[ 1 ] = (verts[ i ].normal[ 1 ] + 1.0f) * 127.5f;
				radVertLuxel[ 2 ] = (verts[ i ].normal[ 2 ] + 1.0f) * 127.5f;
			}
			
			/* color the luxel with surface num? */
			else if( debugSurfaces )
				VectorCopy( debugColors[ num % 12 ], radVertLuxel );
			
			/* divine color from the superluxels */
			else
			{
				/* increasing radius */
				VectorClear( radVertLuxel );
				samples = 0.0f;
				for( radius = 0; radius < maxRadius && samples <= 0.0f; radius++ )
				{
					/* sample within radius */
					for( sy = (y - radius); sy <= (y + radius); sy++ )
					{
						if( sy < 0 || sy >= lm->sh )
							continue;
						
						for( sx = (x - radius); sx <= (x + radius); sx++ )
						{
							if( sx < 0 || sx >= lm->sw )
								continue;
							
							/* get luxel particulars */
							luxel = SUPER_LUXEL( lightmapNum, sx, sy );
							cluster = SUPER_CLUSTER( sx, sy );
							if( *cluster < 0 )
								continue;
							
							/* testing: must be brigher than ambient color */
							//%	if( luxel[ 0 ] <= lm->ambient[ 0 ] || luxel[ 1 ] <= lm->ambient[ 1 ] || luxel[ 2 ] <= lm->ambient[ 2 ] )
							//%		continue;
							
							/* add its distinctiveness to our own */
							VectorAdd( radVertLuxel, luxel, radVertLuxel );
							samples += luxel[ 3 ];
						}
					}
				}
				
				/* any color? */
				if( samples > 0.0f )
					VectorDivide( radVertLuxel, samples, radVertLuxel );
				else
				{
					/* set ambient color */
					VectorCopy( info->ambient, radVertLuxel );
				}
			}
			
			/* store into floating point storage */
			VectorAdd( vertLuxel, radVertLuxel, vertLuxel );
			numVertsIlluminated++;
			
			/* store into bytes (for vertex approximation) */
			ColorToBytes( vertLuxel, verts[ i ].color[ lightmapNum ], 1.0f, qfalse );
		}
	}
}



/* -------------------------------------------------------------------------------

light optimization (-fast)

creates a list of lights that will affect a surface and stores it in tw
this is to optimize surface lighting by culling out as many of the
lights in the world as possible from further calculation

------------------------------------------------------------------------------- */

/*
SetupBrushes()
determines opaque brushes in the world and find sky shaders for sunlight calculations
*/

void SetupBrushes( void )
{
	int				i, j, b, compileFlags;
	qboolean		inside;
	bspBrush_t		*brush;
	bspBrushSide_t	*side;
	bspShader_t		*shader;
	shaderInfo_t	*si;
	
	
	/* note it */
	Sys_PrintHeadingVerbose( "--- SetupBrushes ---\n" );
	
	/* allocate */
	if( opaqueBrushes == NULL )
		opaqueBrushes = (byte *)safe_malloc( numBSPBrushes / 8 + 1 );
	
	/* clear */
	memset( opaqueBrushes, 0, numBSPBrushes / 8 + 1 );
	numOpaqueBrushes = 0;
	
	/* walk the list of worldspawn brushes */
	for( i = 0; i < bspModels[ 0 ].numBSPBrushes; i++ )
	{
		printLabelledProgress("SetupBrushes", i, bspModels[ 0 ].numBSPBrushes);

		/* get brush */
		b = bspModels[ 0 ].firstBSPBrush + i;
		brush = &bspBrushes[ b ];
		
		/* check all sides */
		inside = qtrue;
		compileFlags = 0;
		for( j = 0; j < brush->numSides && inside; j++ )
		{
			/* do bsp shader calculations */
			side = &bspBrushSides[ brush->firstSide + j ];
			shader = &bspShaders[ side->shaderNum ];
			
			/* get shader info */
			si = ShaderInfoForShader( shader->shader );
			if( si == NULL )
				continue;
			
			/* or together compile flags */
			compileFlags |= si->compileFlags;
		}
		
		/* determine if this brush is opaque to light */
		if( !(compileFlags & C_TRANSLUCENT) )
		{
			opaqueBrushes[ b >> 3 ] |= (1 << (b & 7));
			numOpaqueBrushes++;
			maxOpaqueBrush = i;
		}
	}
	
	/* emit some statistics */
	Sys_Printf( "%9d opaque brushes\n", numOpaqueBrushes );
}



/*
ClusterVisible()
determines if two clusters are visible to each other using the PVS
*/

qboolean ClusterVisible( int a, int b )
{
	int			portalClusters, leafBytes;
	byte		*pvs;
	
	
	/* dummy check */
	if( a < 0 || b < 0 )
		return qfalse;
	
	/* early out */
	if( a == b )
		return qtrue;
	
	/* not vised? */
	if( numBSPVisBytes <=8 )
		return qtrue;

	/* get pvs data */
	portalClusters = ((int *) bspVisBytes)[ 0 ];
	leafBytes = ((int*) bspVisBytes)[ 1 ];
	if ((VIS_HEADER_SIZE + (a * leafBytes)) > MAX_MAP_VISIBILITY)
		Error("ClusterVisible: broken cluster %i\n", a);
	pvs = bspVisBytes + VIS_HEADER_SIZE + (a * leafBytes);
	
	/* check */
	if( (pvs[ b >> 3 ] & (1 << (b & 7))) )
		return qtrue;
	return qfalse;
}



/*
PointInLeafNum_r()
borrowed from vlight.c
*/

int	PointInLeafNum_r( vec3_t point, int nodenum )
{
	int			leafnum;
	vec_t		dist;
	bspNode_t		*node;
	bspPlane_t	*plane;
	
	
	while( nodenum >= 0 )
	{
		node = &bspNodes[ nodenum ];
		plane = &bspPlanes[ node->planeNum ];
		dist = DotProduct( point, plane->normal ) - plane->dist;
		if( dist > 0.1 )
			nodenum = node->children[ 0 ];
		else if( dist < -0.1 )
			nodenum = node->children[ 1 ];
		else
		{
			leafnum = PointInLeafNum_r( point, node->children[ 0 ] );
			if( bspLeafs[ leafnum ].cluster != -1 )
				return leafnum;
			nodenum = node->children[ 1 ];
		}
	}
	
	leafnum = -nodenum - 1;
	return leafnum;
}



/*
PointInLeafnum()
borrowed from vlight.c
*/

int	PointInLeafNum( vec3_t point )
{
	return PointInLeafNum_r( point, 0 );
}



/*
ClusterVisibleToPoint() - ydnar
returns qtrue if point can "see" cluster
*/

qboolean ClusterVisibleToPoint( vec3_t point, int cluster )
{
	int		pointCluster;
	

	/* get leafNum for point */
	pointCluster = ClusterForPoint( point );
	if( pointCluster < 0 )
		return qfalse;
	
	/* check pvs */
	return ClusterVisible( pointCluster, cluster );
}



/*
ClusterForPoint() - ydnar
returns the pvs cluster for point
*/

int ClusterForPoint( vec3_t point )
{
	int		leafNum;
	

	/* get leafNum for point */
	leafNum = PointInLeafNum( point );
	if( leafNum < 0 )
		return -1;
	
	/* return the cluster */
	return bspLeafs[ leafNum ].cluster;
}



/*
ClusterForPointExt() - ydnar
also takes brushes into account for occlusion testing
*/

int ClusterForPointExt( vec3_t point, float epsilon )
{
	int				i, j, b, leafNum, cluster;
	float			dot;
	qboolean		inside;
	int				*brushes, numBSPBrushes;
	bspLeaf_t		*leaf;
	bspBrush_t		*brush;
	bspPlane_t		*plane;
	
	
	/* get leaf for point */
	leafNum = PointInLeafNum( point );
	if( leafNum < 0 )
		return -1;
	leaf = &bspLeafs[ leafNum ];
	
	/* get the cluster */
	cluster = leaf->cluster;
	if( cluster < 0 )
		return -1;
	
	/* transparent leaf, so check point against all brushes in the leaf */
	brushes = &bspLeafBrushes[ leaf->firstBSPLeafBrush ];
	numBSPBrushes = leaf->numBSPLeafBrushes;
	for( i = 0; i < numBSPBrushes; i++ )
	{
		/* get parts */
		b = brushes[ i ];
		if( b > maxOpaqueBrush )
			continue;
		brush = &bspBrushes[ b ];
		if( !(opaqueBrushes[ b >> 3 ] & (1 << (b & 7))) )
			continue;
		
		/* check point against all planes */
		inside = qtrue;
		for( j = 0; j < brush->numSides && inside; j++ )
		{
			plane = &bspPlanes[ bspBrushSides[ brush->firstSide + j ].planeNum ];
			dot = DotProduct( point, plane->normal );
			dot -= plane->dist;
			if( dot > epsilon )
				inside = qfalse;
		}
		
		/* if inside, return bogus cluster */
		if( inside )
			return -1 - b;
	}
	
	/* if the point made it this far, it's not inside any opaque brushes */
	return cluster;
}



/*
ClusterForPointExtFilter() - ydnar
adds cluster checking against a list of known valid clusters
*/

int ClusterForPointExtFilter( vec3_t point, float epsilon, int numClusters, int *clusters )
{
	int		i, cluster;
	
	/* get cluster for point */
	cluster = ClusterForPointExt( point, epsilon );
	
	/* check if filtering is necessary */
	if( cluster < 0 || numClusters <= 0 || clusters == NULL )
		return cluster;
	
	/* filter */
	for( i = 0; i < numClusters; i++ )
		if( cluster == clusters[ i ] || ClusterVisible( cluster, clusters[ i ] ) )
			return cluster;
	
	/* failed */
	return -1;
}



/*
ShaderForPointInLeaf() - ydnar
checks a point against all brushes in a leaf, returning the shader of the brush
also sets the cumulative surface and content flags for the brush hit
*/

int ShaderForPointInLeaf( vec3_t point, int leafNum, float epsilon, int wantContentFlags, int wantSurfaceFlags, int *contentFlags, int *surfaceFlags )
{
	int				i, j;
	float			dot;
	qboolean		inside;
	int				*brushes, numBSPBrushes;
	bspLeaf_t			*leaf;
	bspBrush_t		*brush;
	bspBrushSide_t	*side;
	bspPlane_t		*plane;
	bspShader_t		*shader;
	int				allSurfaceFlags, allContentFlags;

	
	/* clear things out first */
	*surfaceFlags = 0;
	*contentFlags = 0;
	
	/* get leaf */
	if( leafNum < 0 )
		return -1;
	leaf = &bspLeafs[ leafNum ];
	
	/* transparent leaf, so check point against all brushes in the leaf */
	brushes = &bspLeafBrushes[ leaf->firstBSPLeafBrush ];
	numBSPBrushes = leaf->numBSPLeafBrushes;
	for( i = 0; i < numBSPBrushes; i++ )
	{
		/* get parts */
		brush = &bspBrushes[ brushes[ i ] ];
		
		/* check point against all planes */
		inside = qtrue;
		allSurfaceFlags = 0;
		allContentFlags = 0;
		for( j = 0; j < brush->numSides && inside; j++ )
		{
			side = &bspBrushSides[ brush->firstSide + j ];
			plane = &bspPlanes[ side->planeNum ];
			dot = DotProduct( point, plane->normal );
			dot -= plane->dist;
			if( dot > epsilon )
				inside = qfalse;
			else
			{
				shader = &bspShaders[ side->shaderNum ];
				allSurfaceFlags |= shader->surfaceFlags;
				allContentFlags |= shader->contentFlags;
			}
		}
		
		/* handle if inside */
		if( inside )
		{
			/* if there are desired flags, check for same and continue if they aren't matched */
			if( wantContentFlags && !(wantContentFlags & allContentFlags) )
				continue;
			if( wantSurfaceFlags && !(wantSurfaceFlags & allSurfaceFlags) )
				continue;
			
			/* store the cumulative flags and return the brush shader (which is mostly useless) */
			*surfaceFlags = allSurfaceFlags;
			*contentFlags = allContentFlags;
			return brush->shaderNum;
		}
	}
	
	/* if the point made it this far, it's not inside any brushes */
	return -1;
}



/*
ChopBounds()
chops a bounding box by the plane defined by origin and normal
returns qfalse if the bounds is entirely clipped away

this is not exactly the fastest way to do this...
*/

qboolean ChopBounds( vec3_t mins, vec3_t maxs, vec3_t origin, vec3_t normal )
{
	/* FIXME: rewrite this so it doesn't use bloody brushes */
	return qtrue;
}



/*
SetupEnvelopes()
calculates each light's effective envelope,
taking into account brightness, type, and pvs.
*/

#define LIGHT_EPSILON	0.125f
#define LIGHT_NUDGE		2.0f

void SetupEnvelopes( qboolean forGrid, qboolean fastFlag )
{
	int			i, x, y, z, x1, y1, z1;
	light_t		*light, *light2, **owner;
	bspLeaf_t	*leaf;
	vec3_t		origin, dir, mins, maxs;
	float		radius, intensity;
	light_t		*buckets[ 256 ];
	char		headingString[256] = { 0 };

	/* early out for weird cases where there are no lights */
	if( lights == NULL )
		return;
	
	/* note it */
	sprintf(headingString, "--- SetupEnvelopes%s%s ---\n", forGrid ? " (lightgrid)" : " (lightmaps)", fastFlag ? " (fast)" : "" );
	Sys_PrintHeading ( headingString );
	
	/* count lights */
	numLights = 0;
	numCulledLights = 0;
	numNegativeLights = 0;
	numPointLights = 0;
	numAreaLights = 0;
	numSpotLights  = 0;
	numSunLights = 0;
	numPointLights = 0;
	owner = &lights;
	while( *owner != NULL )
	{
		/* get light */
		light = *owner;
		
		/* handle negative lights */
		if( light->photons < 0.0f || light->add < 0.0f )
		{
			light->photons = fabs(light->photons);
			light->add = fabs(light->add);
			light->flags = light->flags | LIGHT_NEGATIVE;
		}

		/* sunlight? */
		if( light->type == EMIT_SUN )
		{
			/* special cased */
			light->cluster = 0;
			light->envelope = MAX_WORLD_LIGHT_COORD * 8.0f;
			VectorSet( light->mins, MIN_WORLD_LIGHT_COORD * 8.0f, MIN_WORLD_LIGHT_COORD * 8.0f, MIN_WORLD_LIGHT_COORD * 8.0f );
			VectorSet( light->maxs, MAX_WORLD_LIGHT_COORD * 8.0f, MAX_WORLD_LIGHT_COORD * 8.0f, MAX_WORLD_LIGHT_COORD * 8.0f );
		}
		
		/* everything else */
		else
		{
			/* get pvs cluster for light */
			light->cluster = ClusterForPointExt( light->origin, LIGHT_EPSILON );
			
			/* invalid cluster? */
			if( light->cluster < 0 )
			{
				/* nudge the sample point around a bit */
				for( x = 0; x < 4; x++ )
				{
					/* two's complement 0, 1, -1, 2, -2, etc */
					x1 = ((x >> 1) ^ (x & 1 ? -1 : 0)) + (x & 1);
					
					for( y = 0; y < 4; y++ )
					{
						y1 = ((y >> 1) ^ (y & 1 ? -1 : 0)) + (y & 1);
						
						for( z = 0; z < 4; z++ )
						{
							z1 = ((z >> 1) ^ (z & 1 ? -1 : 0)) + (z & 1);
							
							/* nudge origin */
							origin[ 0 ] = light->origin[ 0 ] + (LIGHT_NUDGE * x1);
							origin[ 1 ] = light->origin[ 1 ] + (LIGHT_NUDGE * y1);
							origin[ 2 ] = light->origin[ 2 ] + (LIGHT_NUDGE * z1);
							
							/* try at nudged origin */
							light->cluster = ClusterForPointExt( origin, LIGHT_EPSILON );
							if( light->cluster < 0 )
								continue;
									
							/* set origin */
							VectorCopy( origin, light->origin );
						}
					}
				}
			}
			
			/* only calculate for lights in pvs and outside of opaque brushes */
			if( light->cluster >= 0 )
			{
				/* set light fast flag */
				if( fastFlag )
					light->flags |= LIGHT_FAST_TEMP;
				else
					light->flags &= ~LIGHT_FAST_TEMP;
				if( light->si && light->si->noFast )
					light->flags &= ~(LIGHT_FAST | LIGHT_FAST_TEMP);
				
				/* clear light envelope */
				light->envelope = 0;
				
				/* handle area lights */
				if( exactPointToPolygon && light->type == EMIT_AREA && light->w != NULL )
				{
					/* ugly hack to calculate extent for area lights, but only done once */
					VectorScale( light->normal, -1.0f, dir );
					for( radius = 100.0f; radius < 130000.0f && light->envelope == 0; radius += 10.0f )
					{
						float	factor;
						
						VectorMA( light->origin, radius, light->normal, origin );
						factor = PointToPolygonFormFactor( origin, dir, light->w );
						if( factor < 0.0f )
							factor *= -1.0f;
						if( (factor * light->add) <= light->falloffTolerance )
							light->envelope = radius;
					}
					
					/* check for fast mode */
					if( !(light->flags & LIGHT_FAST) && !(light->flags & LIGHT_FAST_TEMP) )
						light->envelope = MAX_WORLD_LIGHT_COORD * 8.0f;
				}
				else
				{
					radius = 0.0f;
					intensity = light->photons;
				}
				
				/* other calcs */
				if( light->envelope <= 0.0f )
				{
					/* solve distance for non-distance lights */
					if( !(light->flags & LIGHT_ATTEN_DISTANCE) )
						light->envelope = MAX_WORLD_LIGHT_COORD * 8.0f;
					
					/* solve distance for linear lights */
					else if( (light->flags & LIGHT_ATTEN_LINEAR ) )
						//% light->envelope = ((intensity / light->falloffTolerance) * linearScale - 1 + radius) / light->fade;
						light->envelope = ((intensity * linearScale) - light->falloffTolerance) / light->fade;

						/*
						add = angle * light->photons * linearScale - (dist * light->fade);
						T = (light->photons * linearScale) - (dist * light->fade);
						T + (dist * light->fade) = (light->photons * linearScale);
						dist * light->fade = (light->photons * linearScale) - T;
						dist = ((light->photons * linearScale) - T) / light->fade;
						*/
					
					/* solve for inverse square falloff */
					else
						light->envelope = sqrt( intensity / light->falloffTolerance ) + radius;
						
						/*
						add = light->photons / (dist * dist);
						T = light->photons / (dist * dist);
						T * (dist * dist) = light->photons;
						dist = sqrt( light->photons / T );
						*/
				}
				
				/* chop radius against pvs */
				{
					/* clear bounds */
					ClearBoundsLighting( mins, maxs );
					
					/* check all leaves */
					for( i = 0; i < numBSPLeafs; i++ )
					{
						/* get test leaf */
						leaf = &bspLeafs[ i ];
						
						/* in pvs? */
						if( leaf->cluster < 0 )
							continue;
						if( ClusterVisible( light->cluster, leaf->cluster ) == qfalse )	/* ydnar: thanks Arnout for exposing my stupid error (this never failed before) */
							continue;
						
						/* add this leafs bbox to the bounds */
						VectorCopy( leaf->mins, origin );
						AddPointToBoundsLighting( origin, mins, maxs );
						VectorCopy( leaf->maxs, origin );
						AddPointToBoundsLighting( origin, mins, maxs );
					}
					
					/* test to see if bounds encompass light */
					for( i = 0; i < 3; i++ )
					{
						if( mins[ i ] > light->origin[ i ] || maxs[ i ] < light->origin[ i ] )
						{
							//% Sys_Warning( "Light PVS bounds (%.0f, %.0f, %.0f) -> (%.0f, %.0f, %.0f)\ndo not encompass light %d (%f, %f, %f)",
							//% 	mins[ 0 ], mins[ 1 ], mins[ 2 ],
							//% 	maxs[ 0 ], maxs[ 1 ], maxs[ 2 ],
							//% 	numLights, light->origin[ 0 ], light->origin[ 1 ], light->origin[ 2 ] );
							AddPointToBoundsLighting( light->origin, mins, maxs );
						}
					}
					
					/* chop the bounds by a plane for area lights and spotlights */
					if( light->type == EMIT_AREA || light->type == EMIT_SPOT )
						ChopBounds( mins, maxs, light->origin, light->normal );
					
					/* copy bounds */
					VectorCopy( mins, light->mins );
					VectorCopy( maxs, light->maxs );
					
					/* reflect bounds around light origin */
					//%	VectorMA( light->origin, -1.0f, origin, origin );
					VectorScale( light->origin, 2, origin );
					VectorSubtract( origin, maxs, origin );
					AddPointToBoundsLighting( origin, mins, maxs );
					//%	VectorMA( light->origin, -1.0f, mins, origin );
					VectorScale( light->origin, 2, origin );
					VectorSubtract( origin, mins, origin );
					AddPointToBoundsLighting( origin, mins, maxs );
					 
					/* calculate spherical bounds */
					VectorSubtract( maxs, light->origin, dir );
					radius = (float) VectorLength( dir );
					
					/* if this radius is smaller than the envelope, then set the envelope to it */
					if( radius < light->envelope )
					{
						light->envelope = radius;
						//%	Sys_FPrintf( SYS_VRB, "PVS Cull (%d): culled\n", numLights );
					}
					//%	else
					//%		Sys_FPrintf( SYS_VRB, "PVS Cull (%d): failed (%8.0f > %8.0f)\n", numLights, radius, light->envelope );
				}
				
				/* add grid/surface only check */
				if (!(light->flags & (forGrid ? LIGHT_GRID : LIGHT_SURFACES)))
					light->envelope = 0.0f;
			}
			
			/* remove culled light? */
			if( light->cluster < 0 || light->envelope <= 0.0f )
			{
				owner = &((**owner).next);
				numCulledLights++;
			#if 0
				/* debug code */
				//%	Sys_Printf( "Culling light: Cluster: %d Envelope: %f\n", light->cluster, light->envelope );
				/* delete the light */
				*owner = light->next;
				if( light->w != NULL )
					free( light->w );
				free( light );
			#endif
				continue;
			}
		}
		
		/* square envelope */
		light->envelope2 = (light->envelope * light->envelope);
		
		/* increment light count */
		if (light->flags & LIGHT_NEGATIVE)
			numNegativeLights++;
		if (light->type == EMIT_POINT)
			numPointLights++;
		else if (light->type == EMIT_AREA)
			numAreaLights++;
		else if (light->type == EMIT_SPOT)
			numSpotLights++;
		else if (light->type == EMIT_SUN)
			numSunLights++;
		numLights++;
		
		/* set next light */
		owner = &((**owner).next);
	}
	
	/* bucket sort lights by style */
	memset( buckets, 0, sizeof( buckets ) );
	light2 = NULL;
	for( light = lights; light != NULL; light = light2 )
	{
		/* get next light */
		light2 = light->next;
		
		/* filter into correct bucket */
		light->next = buckets[ light->style ];
		buckets[ light->style ] = light;
		
		/* if any styled light is present, automatically set nocollapse */
		if( light->style != LS_NORMAL )
			noCollapse = qtrue;
	}
	
	/* filter back into light list */
	lights = NULL;
	for( i = 255; i >= 0; i-- )
	{
		light2 = NULL;
		for( light = buckets[ i ]; light != NULL; light = light2 )
		{
			light2 = light->next;
			light->next = lights;
			lights = light;
		}
	}
	
	/* emit some statistics */
	if( numPointLights || verbose )
		Sys_Printf( "%9d point lights\n", numPointLights );
	if( numAreaLights || verbose )
		Sys_Printf( "%9d area lights\n", numAreaLights );
	if( numSpotLights || verbose )
		Sys_Printf( "%9d spot lights\n", numSpotLights );
	if( numSunLights || verbose )
		Sys_Printf( "%9d sun lights\n", numSunLights );
	if( numNegativeLights || verbose )
		Sys_Printf( "%9d negative lights\n", numNegativeLights );
	if( numLights || verbose )
		Sys_Printf( "%9d total lights\n", numLights );
	if( numCulledLights || verbose )
		Sys_Printf( "%9d culled lights\n", numCulledLights );
}

/*
CreateTraceLightsForBounds()
creates a list of lights that affect the given bounding box and pvs clusters (bsp leaves)
*/

void CreateTraceLightsForBounds( qboolean forGrid, vec3_t mins, vec3_t maxs, vec3_t normal, int numClusters, int *clusters, int flags, trace_t *trace )
{
	int			i;
	light_t		*light;
	vec3_t		origin, dir, nullVector = { 0.0f, 0.0f, 0.0f };
	float		radius, dist, length;
	
	/* potential pre-setup  */
	if( numLights < 0 )
		SetupEnvelopes( forGrid, fast );
	
	/* debug code */
	//% Sys_Printf( "CTWLFB: (%4.1f %4.1f %4.1f) (%4.1f %4.1f %4.1f)\n", mins[ 0 ], mins[ 1 ], mins[ 2 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	
	/* allocate the light list */
	trace->lights = (light_t **)safe_malloc( sizeof( light_t* ) * (numLights + 1) );
	trace->numLights = 0;
	
	/* calculate spherical bounds */
	VectorAdd( mins, maxs, origin );
	VectorScale( origin, 0.5f, origin );
	VectorSubtract( maxs, origin, dir );
	radius = (float) VectorLength( dir );
	
	/* get length of normal vector */
	if( normal != NULL )
		length = VectorLength( normal );
	else
	{
		normal = nullVector;
		length = 0;
	}
	
	/* test each light and see if it reaches the sphere */
	/* note: the attenuation code MUST match LightContributionAllStyles() */
	for( light = lights; light; light = light->next )
	{
		/* check zero sized envelope */
		if( light->envelope <= 0 )
		{
			lightsEnvelopeCulled++;
			continue;
		}
		
		/* check flags */
		if( !(light->flags & flags) )
			continue;
		
		/* sunlight skips all this nonsense */
		if( light->type != EMIT_SUN )
		{
			/* sun only? */
			if( sunOnly )
				continue;
			
			/* check against pvs cluster */
			if( numClusters > 0 && clusters != NULL )
			{
				for( i = 0; i < numClusters; i++ )
				{
					if( ClusterVisible( light->cluster, clusters[ i ] ) )
						break;
				}
				
				/* fixme! */
				if( i == numClusters )
				{
					lightsClusterCulled++;
					continue;
				}
			}
			
			/* if the light's bounding sphere intersects with the bounding sphere then this light needs to be tested */
			VectorSubtract( light->origin, origin, dir );
			dist = VectorLength( dir );
			dist -= light->envelope;
			dist -= radius;
			if( dist > 0 )
			{
				lightsEnvelopeCulled++;
				continue;
			}
			
			/* check bounding box against light's pvs envelope (note: this code never eliminated any lights, so disabling it) */
			#if 0
			skip = qfalse;
			for( i = 0; i < 3; i++ )
			{
				if( mins[ i ] > light->maxs[ i ] || maxs[ i ] < light->mins[ i ] )
					skip = qtrue;
			}
			if( skip )
			{
				lightsBoundsCulled++;
				continue;
			}
			#endif
		}
		
		/* planar surfaces (except twosided surfaces) have a couple more checks */
		if( length > 0.0f && trace->twoSided == qfalse )
		{
			/* lights coplanar with a surface won't light it */
			if( !(light->flags & LIGHT_TWOSIDED) && DotProduct( light->normal, normal ) > 0.999f )
			{
				lightsPlaneCulled++;
				continue;
			}
			
			/* check to see if light is behind the plane */
			if( DotProduct( light->origin, normal ) - DotProduct( origin, normal ) < -1.0f )
			{
				lightsPlaneCulled++;
				continue;
			}
		}
		
		/* add this light */
		trace->lights[ trace->numLights++ ] = light;
	}
	
	/* make last night null */
	trace->lights[ trace->numLights ] = NULL;
}



void FreeTraceLights( trace_t *trace )
{
	if( trace->lights != NULL )
		free( trace->lights );
}



/*
CreateTraceLightsForSurface()
creates a list of lights that can potentially affect a drawsurface
*/

void CreateTraceLightsForSurface( int num, trace_t *trace )
{
	int					i;
	vec3_t				mins, maxs, normal;
	bspDrawVert_t		*dv;
	bspDrawSurface_t	*ds;
	surfaceInfo_t		*info;
	
	
	/* dummy check */
	if( num < 0 )
		return;
	
	/* get drawsurface and info */
	ds = &bspDrawSurfaces[ num ];
	info = &surfaceInfos[ num ];
	
	/* get the mins/maxs for the dsurf */
	ClearBoundsLighting( mins, maxs );
	VectorCopy( bspDrawVerts[ ds->firstVert ].normal, normal );
	for( i = 0; i < ds->numVerts; i++ )
	{
		dv = &yDrawVerts[ ds->firstVert + i ];
		AddPointToBoundsLighting( dv->xyz, mins, maxs );
		if( !VectorCompare( dv->normal, normal ) )
			VectorClear( normal );
	}
	
	/* create the lights for the bounding box */
	CreateTraceLightsForBounds( qfalse, mins, maxs, normal, info->numSurfaceClusters, &surfaceClusters[ info->firstSurfaceCluster ], LIGHT_SURFACES, trace );
}

/////////////////////////////////////////////////////////////

#define FLOODLIGHT_CONE_ANGLE			88	/* degrees */
#define FLOODLIGHT_NUM_ANGLE_STEPS		16
#define FLOODLIGHT_NUM_ELEVATION_STEPS	4
#define FLOODLIGHT_NUM_VECTORS			(FLOODLIGHT_NUM_ANGLE_STEPS * FLOODLIGHT_NUM_ELEVATION_STEPS)

static vec3_t	floodVectors[ FLOODLIGHT_NUM_VECTORS ];
static int		numFloodVectors = 0;

void SetupFloodLight( void )
{
	int		i, j;
	float	angle, elevation, angleStep, elevationStep;
	const char	*value;
	double v1,v2,v3,v4,v5;
	
	/* note it */
	Sys_PrintHeadingVerbose( "--- SetupFloodLight ---\n" );
	
	/* calculate angular steps */
	angleStep = DEG2RAD( 360.0f / FLOODLIGHT_NUM_ANGLE_STEPS );
	elevationStep = DEG2RAD( FLOODLIGHT_CONE_ANGLE / FLOODLIGHT_NUM_ELEVATION_STEPS );
	
	/* iterate angle */
	angle = 0.0f;
	for( i = 0, angle = 0.0f; i < FLOODLIGHT_NUM_ANGLE_STEPS; i++, angle += angleStep )
	{
		/* iterate elevation */
		for( j = 0, elevation = elevationStep * 0.5f; j < FLOODLIGHT_NUM_ELEVATION_STEPS; j++, elevation += elevationStep )
		{
			floodVectors[ numFloodVectors ][ 0 ] = sin( elevation ) * cos( angle );
			floodVectors[ numFloodVectors ][ 1 ] = sin( elevation ) * sin( angle );
			floodVectors[ numFloodVectors ][ 2 ] = cos( elevation );
			numFloodVectors++;
		}
	}
	
	/* emit some statistics */
	Sys_Printf( "%9d numFloodVectors\n", numFloodVectors );

    /* floodlight */
	value = ValueForKey( &entities[ 0 ], "_floodlight" );
	
	if( value[ 0 ] != '\0' )
	{
		v1=v2=v3=0;
		v4=floodlightDistance;
		v5=floodlightIntensity;
		
		sscanf( value, "%lf %lf %lf %lf %lf", &v1, &v2, &v3, &v4, &v5);
		
		floodlightRGB[0]=v1;
		floodlightRGB[1]=v2;
		floodlightRGB[2]=v3;
		
		if (VectorIsNull(floodlightRGB))
		{
			VectorSet(floodlightRGB,240,240,255);
		}
		
		if (v4<1) v4=1024;
		if (v5<1) v5=128;
		
		floodlightDistance = v4;
		floodlightIntensity = v5;
    
		floodlighty = qtrue;
		Sys_Printf( "FloodLighting enabled via worldspawn _floodlight key.\n" );
	}
	else
	{
		VectorSet(floodlightRGB,240,240,255);
		//floodlighty = qtrue;
		//Sys_Printf( "FloodLighting enabled via worldspawn _floodlight key.\n" );
	}
	if( colorsRGB )
	{
		floodlightRGB[0] = srgb_to_linear( floodlightRGB[0] );
		floodlightRGB[1] = srgb_to_linear( floodlightRGB[1] );
		floodlightRGB[2] = srgb_to_linear( floodlightRGB[2] );
	}
	VectorNormalize(floodlightRGB,floodlightRGB);
}

/*
FloodLightForSample()
calculates floodlight value for a given sample
once again, kudos to the dirtmapping coder
*/

float FloodLightForSample( trace_t *trace , float floodLightDistance, qboolean floodLightLowQuality)
{
	int		i;
	float 	d;
	float 	contribution;
	int 	sub = 0;
	float	gatherLight, outLight;
	vec3_t	normal, worldUp, myUp, myRt, direction, displacement;
	float 	dd;
	int 	vecs = 0;
 
	gatherLight=0;
	/* dummy check */
	//if( !dirty )
	//	return 1.0f;
	if( trace == NULL || trace->cluster < 0 )
		return 0.0f;
	

	/* setup */
	dd = floodLightDistance;
	VectorCopy( trace->normal, normal );
	
	/* check if the normal is aligned to the world-up */
	if( normal[ 0 ] == 0.0f && normal[ 1 ] == 0.0f )
	{
		if( normal[ 2 ] == 1.0f )		
		{
			VectorSet( myRt, 1.0f, 0.0f, 0.0f );
			VectorSet( myUp, 0.0f, 1.0f, 0.0f );
		}
		else if( normal[ 2 ] == -1.0f )
		{
			VectorSet( myRt, -1.0f, 0.0f, 0.0f );
			VectorSet( myUp,  0.0f, 1.0f, 0.0f );
		}
	}
	else
	{
		VectorSet( worldUp, 0.0f, 0.0f, 1.0f );
		CrossProduct( normal, worldUp, myRt );
		VectorNormalize( myRt, myRt );
		CrossProduct( myRt, normal, myUp );
		VectorNormalize( myUp, myUp );
	}

	/* vortex: optimise floodLightLowQuality a bit */
	if ( floodLightLowQuality == qtrue )
    {
		/* iterate through ordered vectors */
		for( i = 0; i < numFloodVectors; i++ )
			if (rand()%10 != 0 ) continue;
	}
	else
	{
		/* iterate through ordered vectors */
		for( i = 0; i < numFloodVectors; i++ )
		{
			vecs++;
	         
			/* transform vector into tangent space */
			direction[ 0 ] = myRt[ 0 ] * floodVectors[ i ][ 0 ] + myUp[ 0 ] * floodVectors[ i ][ 1 ] + normal[ 0 ] * floodVectors[ i ][ 2 ];
			direction[ 1 ] = myRt[ 1 ] * floodVectors[ i ][ 0 ] + myUp[ 1 ] * floodVectors[ i ][ 1 ] + normal[ 1 ] * floodVectors[ i ][ 2 ];
			direction[ 2 ] = myRt[ 2 ] * floodVectors[ i ][ 0 ] + myUp[ 2 ] * floodVectors[ i ][ 1 ] + normal[ 2 ] * floodVectors[ i ][ 2 ];

			/* set endpoint */
			VectorMA( trace->origin, dd, direction, trace->end );

			//VectorMA( trace->origin, 1, direction, trace->origin );
				
			SetupTrace( trace );
			/* trace */
	  		TraceLine( trace );
			contribution=1;

			if (trace->compileFlags & C_SKY )
			{
				contribution=1.0f;
			}
			else if ( trace->opaque )
			{
				VectorSubtract( trace->hit, trace->origin, displacement );
				d=VectorLength( displacement );
				contribution=d/dd;
				if (contribution>1)
					contribution=1.0f; 
			}
	         
			gatherLight+=contribution;
		}
	}
   
	/* early out */
	if( gatherLight <= 0.0f )
		return 0.0f;
   	
	sub=vecs;

	if (sub<1) sub=1;
	gatherLight/=(sub);

	outLight=gatherLight;
	if( outLight > 1.0f )
		outLight = 1.0f;
	
	/* return to sender */
	return outLight;
}

/*
FloodLightRawLightmap
lighttracer style ambient occlusion light hack.
Kudos to the dirtmapping author for most of this source.
VorteX: modified to floodlight up custom surfaces (q3map_floodLight)
VorteX: fixed problems with deluxemapping
*/

// floodlight pass on a lightmap
void FloodLightRawLightmapPass( rawLightmap_t *lm , vec3_t lmFloodLightRGB, float lmFloodLightIntensity, float lmFloodLightDistance, qboolean lmFloodLightLowQuality, float floodlightDirectionScale)
{
	int					i, x, y, *cluster;
	float				*origin, *normal, *floodlight, floodLightAmount;
	surfaceInfo_t		*info;
	trace_t				trace;
	// int sx, sy;
	// float samples, average, *floodlight2;
	
	memset(&trace,0,sizeof(trace_t));

	/* setup trace */
	trace.entityNum = lm->entityNum;
	trace.testOcclusion = qtrue;
	trace.occlusionBias = 0;
	trace.forceSunlight = qfalse;
	trace.forceSelfShadow = qfalse;
	trace.recvShadows = lm->recvShadows;
	trace.twoSided = qtrue;
	trace.numSurfaces = lm->numLightSurfaces;
	trace.surfaces = &lightSurfaces[ lm->firstLightSurface ];
	trace.inhibitRadius = 0;
	trace.testAll = qfalse;
	trace.distance = 1024;
	
	/* twosided lighting (may or may not be a good idea for lightmapped stuff) */
	//trace.twoSided = qfalse;
	for( i = 0; i < trace.numSurfaces; i++ )
	{
		/* get surface */
		info = &surfaceInfos[ trace.surfaces[ i ] ];
		
		/* check twosidedness */
		if( info->si->twoSided )
		{
			trace.twoSided = qtrue;
			break;
		}
	}
	
	/* gather floodlight */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			cluster = SUPER_CLUSTER( x, y );
			origin = SUPER_ORIGIN( x, y );
			normal = SUPER_NORMAL( x, y );
			floodlight = SUPER_FLOODLIGHT( x, y );
			
			/* set default dirt */
			*floodlight = 0.0f;
			
			/* only look at mapped luxels */
			if( *cluster < 0 )
				continue;
			
			/* copy to trace */
			trace.cluster = *cluster;
			VectorCopy( origin, trace.origin );
			VectorCopy( normal, trace.normal );
   
			/* get floodlight */
			floodLightAmount = FloodLightForSample( &trace , lmFloodLightDistance, lmFloodLightLowQuality)*lmFloodLightIntensity;
			
			/* add floodlight */
			floodlight[0] += lmFloodLightRGB[0]*floodLightAmount;
			floodlight[1] += lmFloodLightRGB[1]*floodLightAmount;
			floodlight[2] += lmFloodLightRGB[2]*floodLightAmount;
			floodlight[3] += floodlightDirectionScale;
		}
	}
	
	/* testing no filtering */
	return;

#if 0
	
	/* filter "dirt" */
	for( y = 0; y < lm->sh; y++ )
	{
		for( x = 0; x < lm->sw; x++ )
		{
			/* get luxel */
			cluster = SUPER_CLUSTER( x, y );
			floodlight = SUPER_FLOODLIGHT(x, y );
			
			/* filter dirt by adjacency to unmapped luxels */
			average = *floodlight;
			samples = 1.0f;
			for( sy = (y - 1); sy <= (y + 1); sy++ )
			{
				if( sy < 0 || sy >= lm->sh )
					continue;
				
				for( sx = (x - 1); sx <= (x + 1); sx++ )
				{
					if( sx < 0 || sx >= lm->sw || (sx == x && sy == y) )
						continue;
					
					/* get neighboring luxel */
					cluster = SUPER_CLUSTER( sx, sy );
					floodlight2 = SUPER_FLOODLIGHT( sx, sy );
					if( *cluster < 0 || *floodlight2 <= 0.0f )
						continue;
					
					/* add it */
					average += *floodlight2;
					samples += 1.0f;
				}
				
				/* bail */
				if( samples <= 0.0f )
					break;
			}
			
			/* bail */
			if( samples <= 0.0f )
				continue;
			
			/* scale dirt */
			*floodlight = average / samples;
		}
	}
#endif
}

void FloodLightRawLightmap(int rawLightmapNum)
{
	rawLightmap_t		*lm;

	/* bail if this number exceeds the number of raw lightmaps */
	if( rawLightmapNum >= numRawLightmaps )
		return;

	/* get lightmap */
	lm = &rawLightmaps[rawLightmapNum];

	/* global pass */
	if (floodlighty && floodlightIntensity)
		FloodLightRawLightmapPass(lm, floodlightRGB, floodlightIntensity, floodlightDistance, floodlight_lowquality, 0);

	/* custom pass */
	if (lm->floodlightIntensity)
	{
		FloodLightRawLightmapPass(lm, lm->floodlightRGB, lm->floodlightIntensity, lm->floodlightDistance, qfalse, lm->floodlightDirectionScale);
		numSurfacesFloodlighten += 1;
	}
}

/*
FloodlightRawLightmaps()
does the job
*/

void FloodlightRawLightmaps(void)
{
	Sys_PrintHeading ( "--- FloodlightRawLightmap ---\n" );
	numSurfacesFloodlighten = 0;
	RunThreadsOnIndividual( "FloodLightRawLightmap", numRawLightmaps, qtrue, FloodLightRawLightmap );
	Sys_Printf( "%9d custom lightmaps floodlighted\n", numSurfacesFloodlighten );
}