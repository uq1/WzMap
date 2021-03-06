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
#define BSP_C



/* dependencies */
#include "q3map2.h"

qboolean				GENERATING_SECONDARY_BSP = qfalse;
extern qboolean			USE_SECONDARY_BSP;
vec3_t					USE_SECONDARY_FIRST_RUN_ORIGIN;
vec3_t					USE_SECONDARY_FIRST_RUN_BOUNDS_MINS;
vec3_t					USE_SECONDARY_FIRST_RUN_BOUNDS_MAXS;

extern qboolean FORCED_STRUCTURAL;

extern void GenerateChunks(void);
extern void GenerateCenterChunk(void);
extern void ProceduralGenFoliage(void);
extern void GenerateCliffFaces ( void );
extern void GenerateLedgeFaces(void);
extern void GenerateCityRoads(void);
extern void GenerateMapCity(void);
extern void GenerateMapForest ( void );
extern void GenerateStaticEntities(void);
extern void GenerateSkyscrapers(void);


/* -------------------------------------------------------------------------------

functions

------------------------------------------------------------------------------- */



/*
SetCloneModelNumbers() - ydnar
sets the model numbers for brush entities
*/

void SetCloneModelNumbers( void )
{
	int			i, j;
	int			models;
	char		modelValue[ 10 ];
	const char	*value, *value2, *value3;
	
	
	/* start with 1 (worldspawn is model 0) */
	models = 1;
	for( i = 1; i < numEntities; i++ )
	{
		/* only entities with brushes or patches get a model number */
		if( entities[ i ].brushes == NULL && entities[ i ].patches == NULL && entities[ i ].forceSubmodel == qfalse)
			continue;
		
		/* is this a clone? */
		value = ValueForKey( &entities[ i ], "_clone" );
		if( value[ 0 ] != '\0' )
			continue;
		
		/* add the model key */
		sprintf( modelValue, "*%d", models );
		SetKeyValue( &entities[ i ], "model", modelValue );
		
		/* increment model count */
		models++;
	}
	
	/* fix up clones */
	for( i = 1; i < numEntities; i++ )
	{
		/* only entities with brushes or patches get a model number */
		if( entities[ i ].brushes == NULL && entities[ i ].patches == NULL && entities[ i ].forceSubmodel == qfalse)
			continue;
		
		/* is this a clone? */
		value = ValueForKey( &entities[ i ], "_ins" );
		if( value[ 0 ] == '\0' )
			value = ValueForKey( &entities[ i ], "_instance" );
		if( value[ 0 ] == '\0' )
			value = ValueForKey( &entities[ i ], "_clone" );
		if( value[ 0 ] == '\0' )
			continue;
		
		/* find an entity with matching clone name */
		for( j = 0; j < numEntities; j++ )
		{
			/* is this a clone parent? */
			value2 = ValueForKey( &entities[ j ], "_clonename" );
			if( value2[ 0 ] == '\0' )
				continue;
			
			/* do they match? */
			if( strcmp( value, value2 ) == 0 )
			{
				/* get the model num */
				value3 = ValueForKey( &entities[ j ], "model" );
				if( value3[ 0 ] == '\0' )
				{
					Sys_Warning( entities[ j ].mapEntityNum, "Cloned entity %s referenced entity without model", value2 );
					continue;
				}
				models = atoi( &value2[ 1 ] );
				
				/* add the model key */
				sprintf( modelValue, "*%d", models );
				SetKeyValue( &entities[ i ], "model", modelValue );
				
				/* nuke the brushes/patches for this entity (fixme: leak!) */
				entities[ i ].brushes = NULL;
				entities[ i ].patches = NULL;
			}
		}
	}
}



/*
FixBrushSides() - ydnar
matches brushsides back to their appropriate drawsurface and shader
*/

static void FixBrushSides( entity_t *e )
{
	int numTotal = numMapDrawSurfs - e->firstDrawSurf;
	int numCompleted = 0;

	/* walk list of drawsurfaces */
//#pragma omp parallel for ordered num_threads(numthreads)
	for (int zzz = 0; zzz < numTotal; zzz++)
	{
		mapDrawSurface_t	*ds;
		sideRef_t			*sideRef;
		bspBrushSide_t		*side;

		int i = e->firstDrawSurf + zzz;

		numCompleted++;

#pragma omp critical (__FixBrushSides__)
		{
			printLabelledProgress("FixBrushSides", numCompleted, numTotal);
		}

		/* get surface and try to early out */
		ds = &mapDrawSurfs[ i ];
		if( ds->outputNum < 0 )
			continue;
		
		/* walk sideref list */
		for( sideRef = ds->sideRef; sideRef != NULL; sideRef = sideRef->next )
		{
			/* get bsp brush side */
			if( sideRef->side == NULL || sideRef->side->outputNum < 0 )
				continue;
			side = &bspBrushSides[ sideRef->side->outputNum ];
			
			/* set drawsurface */
			side->surfaceNum = ds->outputNum;
			//%	Sys_FPrintf( SYS_VRB, "DS: %7d Side: %7d     ", ds->outputNum, sideRef->side->outputNum );
			
			/* set shader */
			if(ds->shaderInfo && strcmp( bspShaders[ side->shaderNum ].shader, ds->shaderInfo->shader ) )
			{
				//%	Sys_FPrintf( SYS_VRB, "Remapping %s to %s\n", bspShaders[ side->shaderNum ].shader, ds->shaderInfo->shader );
				side->shaderNum = EmitShader( ds->shaderInfo->shader, &ds->shaderInfo->contentFlags, &ds->shaderInfo->surfaceFlags );
			}
			else if (!ds->shaderInfo)
			{
				side->shaderNum = 0;
			}
		}
	}

	printLabelledProgress("FixBrushSides", numMapDrawSurfs-e->firstDrawSurf, numMapDrawSurfs-e->firstDrawSurf); // finish bar...
}

/*
FixBrushFaces() - vortex
performs a bugfixing of brush faces
*/

#define STITCH_DISTANCE         0.05f    /* lower than min grid */
#define STITCH_NORMAL_EPSILON   0.05f    /* if triangle normal is changed above this, vertex is rolled back */
#define STITCH_MAX_TRIANGLES    64      /* max triangles formed from a stiched vertex to be checked */
//#define STITCH_USE_TRIANGLE_NORMAL_CHECK

static void FixBrushFaces( entity_t *e )
{
	int numVertsStitched = 0, numSurfacesStitched = 0;

	/* note it */
	Sys_PrintHeadingVerbose( "--- FixBrushFaces ---\n" );

	if (numMapDrawSurfs > 0 && e && e->firstDrawSurf >= 0)
	{
		/* loop drawsurfaces */
		int numTotal = numMapDrawSurfs - e->firstDrawSurf;
		int numCompleted = 0;

		/* walk list of drawsurfaces */
#pragma omp parallel for ordered num_threads(numthreads)
		for (int zzz = 0; zzz < numTotal; zzz++)
		{
			mapDrawSurface_t *ds, *ds2;
			shaderInfo_t *si;
			bspDrawVert_t *dv, *dv2;
			vec3_t mins, maxs, sub;
			int j, k, m, best;
			double dist, bestdist;
			qboolean stitched;

			int i = e->firstDrawSurf + numCompleted;

#pragma omp critical (__PROGRESS_BAR__)
			{
				printLabelledProgress("FixBrushFaces", numCompleted, numTotal);
			}

			numCompleted++;

			/* get surface and early out if possible */
			ds = &mapDrawSurfs[i];

			if (!ds)
				continue;

			si = ds->shaderInfo;
			if (!si || (si->compileFlags & C_NODRAW) || si->autosprite || ds->numVerts == 0 || ds->type != SURFACE_FACE)
				continue;

			/* get bounds, add little bevel */
			VectorCopy(ds->mins, mins);
			VectorCopy(ds->maxs, maxs);
			mins[0] -= 4;
			mins[1] -= 4;
			mins[2] -= 4;
			maxs[0] += 4;
			maxs[1] += 4;
			maxs[2] += 4;

			/* stitch with neighbour drawsurfaces */
			stitched = qfalse;

			for (j = e->firstDrawSurf; j < numMapDrawSurfs; j++)
			{
				/* get surface */
				ds2 = &mapDrawSurfs[j];

				/* test bounds */
				if (!ds2 ||
					ds2->mins[0] > maxs[0] || ds2->maxs[0] < mins[0] ||
					ds2->mins[1] > maxs[1] || ds2->maxs[1] < mins[1] ||
					ds2->mins[2] > maxs[2] || ds2->maxs[2] < mins[2])
					continue;

				/* loop verts */
				for (k = 0; k < ds->numVerts; k++)
				{
					dv = &ds->verts[k];

					if (!dv)
						continue;

					/* find candidate */
					best = -1;
					for (m = 0; m < ds2->numVerts; m++)
					{
						/* don't stitch if origins match completely */
						dv2 = &ds2->verts[m];

						if (!dv2 || (dv2->xyz[0] == dv->xyz[0] && dv2->xyz[1] == dv->xyz[1] && dv2->xyz[2] == dv->xyz[2]))
							continue;

						if (VectorCompareExt(dv2->xyz, dv->xyz, STITCH_DISTANCE) == qfalse)
							continue;

						/* get closest one */
						VectorSubtract(dv2->xyz, dv->xyz, sub);
						dist = VectorLength(sub);

						if (best < 0 || dist < bestdist)
						{
							best = m;
							bestdist = dist;
						}
					}

					/* nothing found? */
					if (best < 0)
						continue;

					/* stitch */
					VectorCopy(dv->xyz, sub);
					VectorCopy(ds2->verts[best].xyz, dv->xyz);

					numVertsStitched++;
					stitched = qtrue;
				}
			}

			/* add to stats */
			if (stitched)
				numSurfacesStitched++;
		}

		printLabelledProgress("FixBrushFaces", numMapDrawSurfs - e->firstDrawSurf, numMapDrawSurfs - e->firstDrawSurf); // finish bar...
	}

	/* emit some statistics */
	Sys_Printf( "%9d verts stitched\n", numVertsStitched );
	Sys_Printf( "%9d surfaces stitched\n", numSurfacesStitched );
}

extern int						c_structural;
extern int						c_detail;
extern int						c_boxbevels;
extern int						c_edgebevels;
extern int						c_areaportals;

void ShowDetailedStats ( void )
{
	Sys_Printf( "%9d total world brushes\n", CountBrushList( entities[ 0 ].brushes ) );

	Sys_Printf( "%9d structural brushes\n", c_structural );
	Sys_Printf( "%9d detail brushes\n", c_detail );
	Sys_Printf( "%9d patches\n", numMapPatches);
	Sys_Printf( "%9d boxbevels\n", c_boxbevels);
	Sys_Printf( "%9d edgebevels\n", c_edgebevels);
	Sys_Printf( "%9d entities\n", numEntities );
	Sys_Printf( "%9d planes\n", nummapplanes);
	Sys_Printf( "%9d areaportals\n", c_areaportals);
}

#ifdef USE_OPENGL
extern void Draw_Scene(void(*drawFunc)(void));
extern void Draw_Winding(winding_t* w, float r, float g, float b, float a);
extern void Draw_AABB(const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec4_t color);

int PortalVisibleSides(portal_t* p)
{
	int             fcon, bcon;

	if (!p->onnode)
		return 0;				// outside

	fcon = p->nodes[0]->opaque;
	bcon = p->nodes[1]->opaque;

	// same contents never create a face
	if (fcon == bcon)
		return 0;

	if (!fcon)
		return 1;
	if (!bcon)
		return 2;
	return 0;
}

static void DrawPortal(portal_t* p, qboolean areaportal)
{
	winding_t*      w;
	int             sides;

	sides = PortalVisibleSides(p);
	if (!sides)
		return;

	w = p->winding;

	if (sides == 2)				// back side
		w = ReverseWinding(w);

	if (areaportal)
	{
		Draw_Winding(w, 1, 0, 0, 0.3);
	}
	else
	{
		Draw_Winding(w, 0, 0, 1, 0.3);
	}

	if (sides == 2)
		FreeWinding(w);
}

static void DrawTreePortals_r(node_t* node)
{
	int             s;
	portal_t*       p, *nextp;
	winding_t*      w;

	if (node->planenum != PLANENUM_LEAF)
	{
		DrawTreePortals_r(node->children[0]);
		DrawTreePortals_r(node->children[1]);
		return;
	}

	// draw all the portals
	for (p = node->portals; p; p = p->next[s])
	{
		w = p->winding;
		s = (p->nodes[1] == node);

		if (w)					// && p->nodes[0] == node)
		{
			//if(PortalPassable(p))
			//	continue;

			DrawPortal(p, node->areaportal);
		}
	}
}

static tree_t*  drawTree = NULL;
static int		drawTreeNodesNum;
static void DrawTreePortals(void)
{
	DrawTreePortals_r(drawTree->headnode);
}


static void DrawTreeNodes_r(node_t* node)
{
	int             i, s;
	brush_t*        b;
	winding_t*      w;
	vec4_t			nodeColor = { 1, 1, 0, 0.3 };
	vec4_t			leafColor = { 0, 0, 1, 0.3 };

	if (!node)
		return;

	drawTreeNodesNum++;

	if (node->planenum == PLANENUM_LEAF)
	{
		//VectorCopy(debugColors[drawTreeNodesNum % 12], leafColor);

		Draw_AABB(vec3_origin, node->mins, node->maxs, leafColor);

		for (b = node->brushlist; b != NULL; b = b->next)
		{
			for (i = 0; i < b->numsides; i++)
			{
				w = b->sides[i].winding;
				if (!w)
					continue;

				if (node->areaportal)
				{
					Draw_Winding(w, 1, 0, 0, 0.3);
				}
				else if (b->detail)
				{
					Draw_Winding(w, 0, 1, 0, 0.3);
				}
				else
				{
					// opaque
					Draw_Winding(w, 0, 0, 0, 0.1);
				}
			}
		}
		return;
	}

	//Draw_AABB(vec3_origin, node->mins, node->maxs, nodeColor);

	DrawTreeNodes_r(node->children[0]);
	DrawTreeNodes_r(node->children[1]);
}
static void DrawNodes(void)
{
	drawTreeNodesNum = 0;
	DrawTreeNodes_r(drawTree->headnode);
}

static void DrawTree(void)
{
	DrawNodes();
}
#endif //USE_OPENGL

/*
ProcessWorldModel()
creates a full bsp + surfaces for the worldspawn entity
*/

extern float subdivisionMult;
extern vec3_t        mergeBlock;
void MergeDrawSurfaces( void );
void MergeDrawVerts( void );
extern void CaulkifyStuff(qboolean findBounds);
extern void OptimizeDrawSurfs(void);
extern void GenerateSmoothNormals(void);
extern void FindWaterLevel(void);

void ProcessWorldModel( void )
{
	int			i, s, start;
	entity_t	*e;
	tree_t		*tree;
	face_t		*faces;
	qboolean	ignoreLeaks, leaked, filled, oldVerbose;
	xmlNodePtr	polyline, leaknode;
	char		level[ 2 ], shader[ MAX_OS_PATH ];
	const char	*value;

	/* note it */
	Sys_PrintHeading ( "--- ProcessWorld ---\n" );
	
	/* sets integer blockSize from worldspawn "_blocksize" key if it exists */
	value = ValueForKey( &entities[ 0 ], "_blocksize" );
	if( value[ 0 ] == '\0' )
		value = ValueForKey( &entities[ 0 ], "blocksize" );
	if( value[ 0 ] == '\0' )
		value = ValueForKey( &entities[ 0 ], "chopsize" );	/* sof2 */
	if( value[ 0 ] != '\0' )
	{
		/* scan 3 numbers */
		s = sscanf( value, "%d %d %d", &blockSize[ 0 ], &blockSize[ 1 ], &blockSize[ 2 ] );
		
		/* handle legacy case */
		if( s == 1 )
		{
			blockSize[ 1 ] = blockSize[ 0 ];
			blockSize[ 2 ] = blockSize[ 0 ];
		}
	}
	Sys_Printf( "BSP block size = { %d %d %d }\n", blockSize[ 0 ], blockSize[ 1 ], blockSize[ 2 ] );
	
	/* sof2: ignore leaks? */
	value = ValueForKey( &entities[ 0 ], "_ignoreleaks" );	/* ydnar */
	if( value[ 0 ] == '\0' )
		value = ValueForKey( &entities[ 0 ], "ignoreleaks" );
	if( value[ 0 ] == '1' )
		ignoreLeaks = qtrue;
	else
		ignoreLeaks = qfalse;

	if (USE_SECONDARY_BSP && GENERATING_SECONDARY_BSP)
	{// Reset all counts...
		{// UQ: Re-init BSP draw verts and buffer on secondary passes...
			free(bspDrawVerts);
			numBSPDrawVerts = 0;
			bspDrawVerts = NULL;

			extern int numBSPDrawVertsBuffer;
			numBSPDrawVertsBuffer = 0;
			SetDrawVerts(MAX_MAP_DRAW_VERTS / 37);
		}

		memset(&entities, 0, sizeof(entity_t) * MAX_MAP_ENTITIES);

		mapEntityNum = 0;

		bspEntDataSize = 0;
		numEntities = 0;
		numBSPEntities = 0;
		numBSPModels = 0;
		numBSPLeafs = 0;
		numBSPPlanes = 0;
		numBSPNodes = 0;
		numBSPLeafSurfaces = 0;
		numBSPLeafBrushes = 0;
		numBSPBrushes = 0;
		numBSPBrushSides = 0;
		numBSPLightBytes = 0;
		numBSPGridPoints = 0;
		numBSPVisBytes = 0;
		numBSPDrawVerts = 0;
		numBSPDrawIndexes = 0;
		numBSPDrawSurfaces = 0;
		numBSPFogs = 0;

		entities[0].brushes = NULL;
		entities[0].firstBrush = NULL;
		entities[0].lastBrush = NULL;
		entities[0].numBrushes = 0;
		entities[0].epairs = NULL;
		entities[0].colorModBrushes = NULL;
		entities[0].forceSubmodel = qfalse;
		entities[0].patches = NULL;
		VectorClear(entities[0].origin);
		VectorClear(entities[0].originbrush_origin);

		portalclusters = 0;
		numMapDrawSurfs = 0;
		numMapPatches = 0;
		nummapplanes = 0;
		numMapEntities = 0;
		numClearedSurfaces = 0;
		numStripSurfaces = 0;
		numFanSurfaces = 0;
		numMergedSurfaces = 0;
		numMergedVerts = 0;
		numRedundantIndexes = 0;
		numSurfaceModels = 0;
		numStrippedLights = 0;
		numportals = 0;
		numfaces = 0;
		numActiveBrushes = 0;
		numMapFogs = 0;
		defaultFogNum = -1;

		c_structural = 0;
		c_detail = 0;
		c_boxbevels = 0;
		c_edgebevels = 0;
		c_areaportals = 0;

		extern int		num_visclusters;				// clusters the player can be in
		extern int		num_visportals;
		extern int		num_solidfaces;

		num_visclusters = 0;
		num_visportals = 0;
		num_solidfaces = 0;

		extern int numSolidSurfs;
		extern int numHeightCulledSurfs;
		extern int numSizeCulledSurfs;
		extern int numExperimentalCulled;
		extern int numBoundsCulledSurfs;

		numSolidSurfs = 0;
		numHeightCulledSurfs = 0;
		numSizeCulledSurfs = 0;
		numExperimentalCulled = 0;
		numBoundsCulledSurfs = 0;

		extern int removed_numHiddenFaces;
		extern int removed_numCoinFaces;

		removed_numHiddenFaces = 0;
		removed_numCoinFaces = 0;

		extern int					numMetaSurfaces;
		extern int					numPatchMetaSurfaces;

		extern int					maxMetaVerts;
		extern int					numMetaVerts;
		extern int					firstSearchMetaVert;
		extern bspDrawVert_t		*metaVerts;

		extern int					maxMetaTriangles;
		extern int					numMetaTriangles;
		
		extern metaTriangle_t		*metaTriangles;
		
		metaTriangles = NULL;
		metaVerts = NULL;

		numMetaSurfaces = 0;
		numStripSurfaces = 0;
		numFanSurfaces = 0;
		numPatchMetaSurfaces = 0;
		numMetaVerts = 0;
		numMetaTriangles = 0;
		numMergedSurfaces = 0;
		numMergedVerts = 0;
		maxMetaVerts = 0;
		firstSearchMetaVert = 0;
		maxMetaTriangles = 0;

		for (i = 0; i < NUM_SURFACE_TYPES; i++)
		{
			numSurfacesByType[i] = 0;
		}

		//ignoreLeaks = qtrue;
	}
	
	/* begin worldspawn model */
	BeginModel();
	e = &entities[ 0 ];
	e->firstDrawSurf = 0;
	if( ignoreLeaks )
		Sys_Printf( "Ignoring leaks\n" );

	/* ydnar: gs mods */
	ClearMetaTriangles();

	/* check for patches with adjacent edges that need to lod together */
	PatchMapDrawSurfs( e );

	/* build an initial bsp tree using all of the sides of all of the structural brushes */
	faces = MakeStructuralBSPFaceList( entities[ 0 ].brushes );
	tree = FaceBSP( faces, qfalse );
	MakeTreePortals( tree, qfalse );
	FilterStructuralBrushesIntoTree( e, tree, qfalse );

	/* note BSP phase (non-verbose-mode) */
	if( !verbose )
		Sys_PrintHeading ( "--- BuildBSP ---\n" );
	
	/* see if the bsp is completely enclosed */
	filled = ignoreLeaks;
	if( filled )
		FloodEntities( tree, qtrue );
	else
		filled = FloodEntities( tree, qfalse );
	if( filled )
	{
		Sys_PrintHeadingVerbose( "--- RebuildBSP ---\n" );

		/* rebuild a better bsp tree using only the sides that are visible from the inside */
		FillOutside( tree->headnode );

		/* chop the sides to the convex hull of their visible fragments, giving us the smallest polygons */
		ClipSidesIntoTree( e, tree, qtrue );
		
		/* build a visible face tree */
		faces = MakeVisibleBSPFaceList( entities[ 0 ].brushes );
		FreeTree( tree );
		tree = FaceBSP( faces, qtrue );
		MakeTreePortals( tree, qtrue );
		FilterStructuralBrushesIntoTree( e, tree, qtrue );
		leaked = qfalse;
		
		/* ydnar: flood again for skybox */
		if( skyboxPresent )
			FloodEntities( tree, qtrue );

		/* emit stats */
		oldVerbose = verbose;
		verbose = qtrue;
		FillOutsideStats();
		ClipSidesIntoTreeStats();
		FaceBSPStats();
		MakeTreePortalsStats();
		FilterStructuralBrushesIntoTreeStats();
		FloodEntitiesStats();
		verbose = oldVerbose;
	}
	else
	{
		Sys_FPrintf( SYS_NOXML, "**********************\n" );
		Sys_FPrintf( SYS_NOXML, "******* leaked *******\n" );
		Sys_FPrintf( SYS_NOXML, "**********************\n" );
		polyline = LeakFile( tree );
		leaknode = xmlNewNode( NULL, (xmlChar *)"message" );
		xmlNodeSetContent( leaknode, (xmlChar *)"MAP LEAKED\n" );
		xmlAddChild( leaknode, polyline );
		level[0] = (int) '0' + SYS_ERR;
		level[1] = 0;
		xmlSetProp( leaknode, (xmlChar *)"level", (xmlChar *)&level );
		xml_SendNode( leaknode );
		if( leaktest )
		{
			Sys_Printf ("MAP LEAKED, ABORTING LEAKTEST\n");
			exit( 0 );
		}
		leaked = qtrue;
		
		/* chop the sides to the convex hull of their visible fragments, giving us the smallest polygons */
		ClipSidesIntoTree( e, tree, qfalse );
	}

#ifdef USE_OPENGL
	if (drawBSP)
	{
		Sys_PrintHeading("--- DrawBSP ---\n");
		drawTree = tree;
		Sys_Printf("Drawing nodes. Hit ESC to exit.\n");
		Draw_Scene(DrawNodes);
		Sys_Printf("Drawing Tree Portals. Hit ESC to exit.\n");
		Draw_Scene(DrawTreePortals);
	}
#endif //USE_OPENGL
	
	/* save out information for visibility processing */
	NumberClusters( tree );

	if (!USE_SECONDARY_BSP || !GENERATING_SECONDARY_BSP)
	{
		//if( !leaked ) // UQ1: I'm gonna write the portal file, regardless of leaks...
		WritePortalFile(tree);
	}

	/* note BSP phase (non-verbose-mode) */
	if( !verbose )
	{
		Sys_PrintHeading ( "--- CreateMapDrawsurfs ---\n" );
		start = I_FloatTime();
	}
	
	/* flood from entities */
	FloodAreas( tree );

	if (USE_SECONDARY_BSP && GENERATING_SECONDARY_BSP)
	{
		/* UQ1: Generate procedural city objects */
		GenerateMapCity();

		/* UQ1: Generate procedural trees/etc */
		GenerateMapForest();
	}
	else
	{
		// Remove crap...
		CaulkifyStuff(qtrue);
		//OptimizeDrawSurfs();

		/* UQ1: Find water level so that we can skip adding procedural crap underwater */
		FindWaterLevel();

		/* UQ1: Generate procedural map chunks */
		GenerateChunks();
		GenerateCenterChunk();
		ProceduralGenFoliage();

		/* UQ1: Generate procedural cliff faces */
		GenerateCliffFaces();

		/* UQ1: Generate procedural ledge faces */
		GenerateLedgeFaces();

		/* UQ1: Generate procedural skyscrapers */
		GenerateSkyscrapers();

		/* UQ1: Generate static entities from ini */
		GenerateStaticEntities();

		/* UQ1: Generate procedural city roads */
		GenerateCityRoads();

		/* UQ1: Generate procedural city objects */
		GenerateMapCity();

		/* UQ1: Generate procedural trees/etc */
		GenerateMapForest();
	}

	if (USE_SECONDARY_BSP && !GENERATING_SECONDARY_BSP)
	{
		VectorCopy(entities[0].origin, USE_SECONDARY_FIRST_RUN_ORIGIN);
	}
	
	/* create drawsurfs for triangle models */
	AddTriangleModels( 0, qfalse, qfalse, qfalse, qfalse);
	
	/* create drawsurfs for surface models */
	AddEntitySurfaceModels( e );

	ShowDetailedStats();

	// Remove crap...
	CaulkifyStuff(qfalse);
	//OptimizeDrawSurfs();

	//mapplanes = (plane_t*)realloc(mapplanes, sizeof(plane_t)*nummapplanes); // UQ1: Test realloc here
	
	/* generate bsp brushes from map brushes */
	EmitBrushes( e->brushes, &e->firstBrush, &e->numBrushes );
	
	if (FORCED_STRUCTURAL)
	{
		FilterStructuralBrushesIntoTree(e, tree, qfalse);
		//extern void CullSidesStats(void);
		//extern void CullSides(entity_t *e);
		//CullSides(e);
		//CullSidesStats();
		WritePortalFile(tree); // Write final portals file including the new procedural addition changes...
	}

	/* add references to the detail brushes */
	FilterDetailBrushesIntoTree( e, tree );

	/* drawsurfs that cross fog boundaries will need to be split along the fog boundary */
	if( !nofog )
		FogDrawSurfaces( e );
	
	/* subdivide each drawsurf as required by shader tesselation */
	if (!nosubdivide)
	{
		SubdivideFaceSurfaces(e, tree);
	}

	/* vortex: fix degenerate brush faces */
	FixBrushFaces( e );

	/* add in any vertexes required to fix t-junctions */
	if( !noTJunc )
		FixTJunctions( e );

	/* ydnar: classify the surfaces */
	ClassifyEntitySurfaces( e );
	
	/* ydnar: project decals */
	MakeEntityDecals( e );
	
	/* ydnar: meta surfaces */
	MakeEntityMetaTriangles( e );
	SmoothMetaTriangles();
	FixMetaTJunctions();
	MergeMetaTriangles();

	/* ydnar: debug portals */
	if( debugPortals )
		MakeDebugPortalSurfs( tree );
	
	/* ydnar: fog hull */
	value = ValueForKey( &entities[ 0 ], "_foghull" );
	if( value[ 0 ] != '\0' )
	{
		sprintf( shader, "textures/%s", value );
		MakeFogHullSurfs( e, tree, shader );
	}
	
	/* ydnar: bug 645: do flares for lights */
	for( i = 0; i < numEntities && emitFlares; i++ )
	{
		entity_t	*light, *target;
		const char	*value, *flareShader;
		vec3_t		origin, targetOrigin, normal, color;
		int			lightStyle;
		
		printLabelledProgress("AddLightFlares", i, numEntities);
		
		/* get light */
		light = &entities[ i ];
		value = ValueForKey( light, "classname" );
		if( !strcmp( value, "light" ) )
		{
			/* get flare shader */
			flareShader = ValueForKey( light, "_flareshader" );
			value = ValueForKey( light, "_flare" );
			if( flareShader[ 0 ] != '\0' || value[ 0 ] != '\0' )
			{
				/* get specifics */
				GetVectorForKey( light, "origin", origin );
				GetVectorForKey( light, "_color", color );
				lightStyle = IntForKey( light, "_style" );
				if( lightStyle == 0 )
					lightStyle = IntForKey( light, "style" );
				
				/* handle directional spotlights */
				value = ValueForKey( light, "target" );
				if( value[ 0 ] != '\0' )
				{
					/* get target light */
					target = FindTargetEntity( value );
					if( target != NULL )
					{
						GetVectorForKey( target, "origin", targetOrigin );
						VectorSubtract( targetOrigin, origin, normal );
						VectorNormalize( normal, normal );
					}
				}
				else
					//%	VectorClear( normal );
					VectorSet( normal, 0, 0, -1 );
				
				/* convert to sRGB colorspace */
				if ( colorsRGB ) {
					color[0] = srgb_to_linear( color[0] );
					color[1] = srgb_to_linear( color[1] );
					color[2] = srgb_to_linear( color[2] );
				}

				/* create the flare surface (note shader defaults automatically) */
				DrawSurfaceForFlare( mapEntityNum, origin, normal, color, (char*) flareShader, lightStyle );
			}
		}
	}
	if ( !verbose )
	{
		//Sys_Printf( " (%d)\n", (int) (I_FloatTime() - start) );
		Sys_Printf( "%9d drawsurfs\n", numMapDrawSurfs );
	}

	OptimizeDrawSurfs();
	GenerateSmoothNormals();

	/* add references to the final drawsurfs in the apropriate clusters */
	FilterDrawsurfsIntoTree( e, tree, qtrue );

	if( !verbose )
		EmitDrawsurfsSimpleStats();
	else
		EmitDrawsurfsStats();

	/* match drawsurfaces back to original brushsides (sof2) */
	FixBrushSides( e );
	
	/* finish */
	EndModel( e, tree->headnode );

#ifdef USE_OPENGL
	if (drawBSP)
	{
		// draw unoptimized portals in new window
		drawTree = tree;
		Sys_PrintHeading("--- DrawBSP ---\n");
		Sys_Printf("Drawing nodes. Hit ESC to exit.\n");
		Draw_Scene(DrawNodes);
	}
#endif //USE_OPENGL

	FreeTree( tree );

	/* UQ1: merge test */
	//VectorSet(mergeBlock, 1024.0f, 1024.0f, 1024.0f);
	//MergeDrawSurfaces();
	//MergeDrawVerts();
	/* UQ1: merge test */
}



/*
ProcessSubModel()
creates bsp + surfaces for other brush models
*/

void ProcessSubModel( void )
{
	entity_t	*e;
	tree_t		*tree;
	brush_t		*b, *bc;
	node_t		*node;
	int         entityNum;
	
	/* start a brush model */
	BeginModel();
	entityNum = mapEntityNum;
	e = &entities[ entityNum ];
	e->firstDrawSurf = numMapDrawSurfs;
	
	/* ydnar: gs mods */
	ClearMetaTriangles();
	
	/* check for patches with adjacent edges that need to lod together */
	PatchMapDrawSurfs( e );
	
	/* allocate a tree */
	node = AllocNode();
	node->planenum = PLANENUM_LEAF;
	tree = AllocTree();
	tree->headnode = node;
	
	/* add the sides to the tree */
	ClipSidesIntoTree( e, tree, qtrue );
	
	/* ydnar: create drawsurfs for triangle models */
	AddTriangleModels( entityNum, qfalse, qfalse, qfalse, qfalse);
	
	/* create drawsurfs for surface models */
	AddEntitySurfaceModels( e );
	
	/* generate bsp brushes from map brushes */
	EmitBrushes( e->brushes, &e->firstBrush, &e->numBrushes );

	/* just put all the brushes in headnode */
	for( b = e->brushes; b; b = b->next )
	{
		bc = CopyBrush( b );
		bc->next = node->brushlist;
		node->brushlist = bc;
	}
	
	/* subdivide each drawsurf as required by shader tesselation */
	if( !nosubdivide )
		SubdivideFaceSurfaces( e, tree );
	
	/* vortex: fix degenerate brush faces */
	FixBrushFaces( e );
	
	/* add in any vertexes required to fix t-junctions */
	if( !noTJunc )
		FixTJunctions( e );
	
	/* ydnar: classify the surfaces and project lightmaps */
	ClassifyEntitySurfaces( e );
	
	/* ydnar: project decals */
	MakeEntityDecals( e );
	
	/* ydnar: meta surfaces */
	MakeEntityMetaTriangles( e );
	SmoothMetaTriangles();
	FixMetaTJunctions();
	MergeMetaTriangles();
	
	/* add references to the final drawsurfs in the apropriate clusters */
	FilterDrawsurfsIntoTree( e, tree, qfalse );
	
	/* match drawsurfaces back to original brushsides (sof2) */
	FixBrushSides( e );

	/* finish */
	EndModel( e, node );
	FreeTree( tree );
}



/*
ProcessModels()
process world + other models into the bsp
*/

void ProcessModels( void )
{
	int			fOld, start, submodel, submodels;
	qboolean	oldVerbose;
	entity_t	*entity;
	
	/* preserve -v setting */
	oldVerbose = verbose;
	
	/* start a new bsp */
	BeginBSPFile();
	
	/* create map fogs */
	CreateMapFogs();

	/* process world model */
	ProcessWorldModel();

	/* process submodels */
	verbose = qfalse;
	start = I_FloatTime();
	fOld = -1;

	Sys_PrintHeading ( "--- ProcessModels ---\n" );

	/* count */
	for( submodels = 0, mapEntityNum = 1; mapEntityNum < numEntities; mapEntityNum++ )
		if( entities[ mapEntityNum ].brushes != NULL || entities[ mapEntityNum ].patches != NULL || entities[ mapEntityNum ].forceSubmodel != qfalse )
			submodels++;

	/* process */
	for( submodel = 0, mapEntityNum = 1; mapEntityNum < numEntities; mapEntityNum++ )
	{
		printLabelledProgress("ProcessSubModels", mapEntityNum, numEntities);

		entity = &entities[ mapEntityNum ];
		if( entity->brushes == NULL && entity->patches == NULL && entity->forceSubmodel == qfalse )
			continue;
		submodel++;
		ProcessSubModel();
	}

	/* print overall time */
	//if ( submodels > 10 )
	//	Sys_Printf (" (%d)\n", (int) (I_FloatTime() - start) );

	/* restore -v setting */
	verbose = oldVerbose;

	/* emit stats */
	Sys_Printf( "%9i submodels\n", submodels);
	EmitDrawsurfsStats();

	/* write fogs */
	EmitFogs();

	/* vortex: emit meta stats */
	EmitMetaStats();
}

/*
RegionScissor()
remove all stuff outside map region
*/
void RegionScissor( void )
{
	int i, k, entityNum, scissorEntity, c, frame;
	int removedBrushes = 0, removedPatches = 0, removedModels = 0, removedEntities = 0, removedLights = 0;
	const char *classname, *model, *value;
	float temp, intensity;
	m4x4_t transform;
	entity_t *e;
	brush_t *b, *bs, *bl, *nb;
	parseMesh_t *p, *ps, *pl, *np;
	vec3_t mins, maxs, origin, scale, angles, color;
	picoModel_t *picomodel;
	bspDrawVert_t *v;

	/* early out */
	if ( mapRegion == qfalse )
		return;

	/* note it */
	Sys_PrintHeading ( "--- RegionScissor ---\n" );

	/* scissor world brushes */
	e = &entities[ 0 ];
	bs = NULL;
	for( b = e->brushes; b; b = nb )
	{
		nb = b->next;

		/* sky surfaces are never scissored */
		if( b->mins[ 0 ] > mapRegionMaxs[ 0 ] || b->maxs[ 0 ] < mapRegionMins[ 0 ] ||
		    b->mins[ 1 ] > mapRegionMaxs[ 1 ] || b->maxs[ 1 ] < mapRegionMins[ 1 ] ||
			b->mins[ 2 ] > mapRegionMaxs[ 2 ] || b->maxs[ 2 ] < mapRegionMins[ 2 ] )
		{
			/* sky brushes are never scissored */
			for( i = 0; i < b->numsides; i++ )
				if( b->sides[ i ].compileFlags & C_SKY )
					goto keep;
	
			/* remove brush */
			b->next = NULL;
			FreeBrush(b);
			removedBrushes++;
			continue;
		}
		/* keep brush */
keep:
		b->nextColorModBrush = NULL;
		if (bs == NULL)
		{
			bs = b;
			bl = b;
		}
		else
		{
			bl->next = b;
			b->next = NULL;
			bl = b;
		}
	}
	e->brushes = bs;

	/* relink colorMod brushes */
	e->colorModBrushes = NULL;
	for( b = e->brushes; b; b = b->next )
	{
		if( b->contentShader != NULL && b->contentShader->colorMod != NULL && b->contentShader->colorMod->type == CM_VOLUME )
		{
			b->nextColorModBrush = e->colorModBrushes;
			e->colorModBrushes = b;
		}
	}

	/* scissor world patches */
	e = &entities[ 0 ];
	ps = NULL;
	for( p = e->patches; p; p = np )
	{
		np = p->next;
		/* calc patch bounds */
		ClearBounds( mins, maxs );
		c = p->mesh.width * p->mesh.height;
		v = p->mesh.verts;
		for( k = 0; k < c; k++, v++ )
			AddPointToBounds( v->xyz, mins, maxs );
		if( mins[ 0 ] > mapRegionMaxs[ 0 ] || maxs[ 0 ] < mapRegionMins[ 0 ] ||
			mins[ 1 ] > mapRegionMaxs[ 1 ] || maxs[ 1 ] < mapRegionMins[ 1 ] ||
			mins[ 2 ] > mapRegionMaxs[ 2 ] || maxs[ 2 ] < mapRegionMins[ 2 ] )
		{
			/* remove patch */
			/* FIXME: leak */
			p->next = NULL;
			removedPatches++;
			continue;
		}
		/* keep patch */
		if (ps == NULL)
		{
			ps = p;
			pl = p;
		}
		else
		{
			pl->next = p;
			p->next = NULL;
			pl = p;
		}
	}
	e->patches = ps;

	/* scissor entities */
	scissorEntity = 1;
	for( entityNum = 1; entityNum < numEntities; entityNum++ )
	{
		e = &entities[ entityNum ];
		classname = ValueForKey( e, "classname" );

		/* scissor misc_models by model box */
		if( !Q_stricmp( classname, "misc_model" ) || !Q_stricmp( classname, "misc_gamemodel" ) || !Q_stricmp(classname, "misc_lodmodel"))
		{
			/* get model name */
			model = ValueForKey( e, "_model" );	
			if( model[ 0 ] == '\0' )
				model = ValueForKey( e, "model" );
			if( model[ 0 ] == '\0' )
				goto scissorentity;
					
			/* get model frame */
			frame = 0;
			if( KeyExists(e, "frame" ) )
				frame = IntForKey( e, "frame" );
			if( KeyExists(e, "_frame" ) )
				frame = IntForKey( e, "_frame" );

			/* get origin */
			GetVectorForKey( e, "origin", origin );

			/* get scale */
			scale[ 0 ] = scale[ 1 ] = scale[ 2 ] = 1.0f;
			temp = FloatForKey( e, "modelscale" );
			if( temp != 0.0f )
				scale[ 0 ] = scale[ 1 ] = scale[ 2 ] = temp;
			value = ValueForKey( e, "modelscale_vec" );
			if( value[ 0 ] != '\0' )
				sscanf( value, "%f %f %f", &scale[ 0 ], &scale[ 1 ], &scale[ 2 ] );

			/* get "angle" (yaw) or "angles" (pitch yaw roll) */
			angles[ 0 ] = angles[ 1 ] = angles[ 2 ] = 0.0f;
			angles[ 2 ] = FloatForKey( e, "angle" );
			value = ValueForKey( e, "angles" );
			if( value[ 0 ] != '\0' )
				sscanf( value, "%f %f %f", &angles[ 1 ], &angles[ 2 ], &angles[ 0 ] );

			/* get model */
			picomodel = LoadModel( model, frame );
			if( picomodel == NULL )
				goto scissorentity;
			VectorCopy(picomodel->mins, mins);
			VectorCopy(picomodel->maxs, maxs);

			/* transform */
			m4x4_identity( transform );
			m4x4_pivoted_transform_by_vec3( transform, origin, angles, eXYZ, scale, vec3_origin );
			m4x4_transform_point( transform, mins );
			m4x4_transform_point( transform, maxs );

			/* scissor */
			if( mins[ 0 ] > mapRegionMaxs[ 0 ] || maxs[ 0 ] < mapRegionMins[ 0 ] ||
				mins[ 1 ] > mapRegionMaxs[ 1 ] || maxs[ 1 ] < mapRegionMins[ 1 ] ||
				mins[ 2 ] > mapRegionMaxs[ 2 ] || maxs[ 2 ] < mapRegionMins[ 2 ] )
			{
				removedEntities++;
				removedModels++;
				continue;
			}
			goto keepentity;
		}

		/* scissor bmodel entity */
		if ( e->brushes || e->patches )
		{
			/* calculate entity bounds */
			ClearBounds(mins, maxs);
			for( b = e->brushes; b; b = b->next )
			{
				AddPointToBounds(b->mins, mins, maxs);
				AddPointToBounds(b->maxs, mins, maxs);
			}
			for( p = e->patches; p; p = p->next )
			{
				c = p->mesh.width * p->mesh.height;
				v = p->mesh.verts;
				for( k = 0; k < c; k++, v++ )
					AddPointToBounds( v->xyz, mins, maxs );
			}

			/* adjust by origin */
			GetVectorForKey( e, "origin", origin );
			VectorAdd(mins, origin, mins);
			VectorAdd(maxs, origin, maxs);

			/* scissor */
			if( mins[ 0 ] > mapRegionMaxs[ 0 ] || maxs[ 0 ] < mapRegionMins[ 0 ] ||
				mins[ 1 ] > mapRegionMaxs[ 1 ] || maxs[ 1 ] < mapRegionMins[ 1 ] ||
				mins[ 2 ] > mapRegionMaxs[ 2 ] || maxs[ 2 ] < mapRegionMins[ 2 ] )
			{
				removedEntities++;
				continue;
			}

			goto keepentity;
		}

		/* scissor lights by distance */
		if( !Q_strncasecmp( classname, "light", 5 ) )
		{
			/* get origin */
			GetVectorForKey( e, "origin", origin );

			/* get intensity */
			intensity = FloatForKey( e, "_light" );
			if( intensity == 0.0f )
				intensity = FloatForKey( e, "light" );
			if( intensity == 0.0f)
				intensity = 300.0f;

			/* get light color */
			color[ 0 ] = color[ 1 ] = color[ 2 ] = 1.0f;
			value = ValueForKey( e, "_color" );
			if( value && value[ 0 ] )
				sscanf( value, "%f %f %f", &color[ 0 ], &color[ 1 ], &color[ 2 ] );

			/* get distance */
			temp = 0;
			if (origin[ 0 ] > mapRegionMaxs[ 0 ])
				temp = max(temp, origin[ 0 ] - mapRegionMaxs[ 0 ]);
			if (origin[ 1 ] > mapRegionMaxs[ 1 ])
				temp = max(temp, origin[ 1 ] - mapRegionMaxs[ 1 ]);
			if (origin[ 2 ] > mapRegionMaxs[ 2 ])
				temp = max(temp, origin[ 2 ] - mapRegionMaxs[ 2 ]);
			if (origin[ 0 ] < mapRegionMins[ 0 ])
				temp = max(temp, mapRegionMins[ 0 ] - origin[ 0 ]);
			if (origin[ 1 ] < mapRegionMins[ 1 ])
				temp = max(temp, mapRegionMins[ 1 ] - origin[ 1 ]);
			if (origin[ 2 ] < mapRegionMins[ 2 ])
				temp = max(temp, mapRegionMins[ 2 ] - origin[ 2 ]);

			/* cull */
			temp = intensity / ( temp * temp );
			intensity = max(color[0] * temp, max(color[1] * temp, color[2] * temp));
			if (intensity <= 0.002)
			{
				removedLights++;
				removedEntities++;
				continue;
			}
			goto keepentity;
		}

		/* scissor point entities by origin */
scissorentity:
		GetVectorForKey( e, "origin", origin );
		if( VectorCompare( origin, vec3_origin ) == qfalse ) 
		{
			if( origin[ 0 ] > mapRegionMaxs[ 0 ] || origin[ 0 ] < mapRegionMins[ 0 ] ||
				origin[ 1 ] > mapRegionMaxs[ 1 ] || origin[ 1 ] < mapRegionMins[ 1 ] ||
				origin[ 2 ] > mapRegionMaxs[ 2 ] || origin[ 2 ] < mapRegionMins[ 2 ] )
			{
				removedEntities++;
				continue;
			}
		}

		/* entity is keeped */
keepentity:
		if (scissorEntity != entityNum)
			memcpy(&entities[scissorEntity], &entities[entityNum], sizeof(entity_t));
		scissorEntity++;
	}
	numEntities = scissorEntity;


	/* emit some stats */
	Sys_Printf( "%9d brushes removed\n", removedBrushes );
	Sys_Printf( "%9d patches removed\n", removedPatches );
	Sys_Printf( "%9d models removed\n", removedModels );
	Sys_Printf( "%9d lights removed\n", removedLights );
	Sys_Printf( "%9d entities removed\n", removedEntities );
}

/*
BSPMain() - ydnar
handles creation of a bsp from a map file
*/

extern char				currentMapName[256];
extern int				FOLIAGE_NUM_POSITIONS;
extern int				FOLIAGE_NOT_GROUND_COUNT;

extern void FOLIAGE_LoadClimateData( char *filename );
extern qboolean FOLIAGE_LoadFoliagePositions( char *filename );
extern void PreloadClimateModels(void);


int BSPMain( int argc, char **argv )
{
	int			i;
	char		path[ MAX_OS_PATH ], tempSource[ MAX_OS_PATH ];

	/* note it */
	Sys_PrintHeading ( "--- BSP ---\n" );
	
	SetDrawSurfacesBuffer();
	mapDrawSurfs = (mapDrawSurface_t *)safe_malloc( sizeof( mapDrawSurface_t ) * MAX_MAP_DRAW_SURFS );
	memset( mapDrawSurfs, 0, sizeof( mapDrawSurface_t ) * MAX_MAP_DRAW_SURFS );
	numMapDrawSurfs = 0;

	tempSource[ 0 ] = '\0';

	GENERATING_SECONDARY_BSP = qfalse;
	
	/* set standard game flags */
	maxSurfaceVerts = game->maxSurfaceVerts;
	maxLMSurfaceVerts = game->maxLMSurfaceVerts;
	maxSurfaceIndexes = game->maxSurfaceIndexes;
	emitFlares = game->emitFlares;
	colorsRGB = game->colorsRGB;
	texturesRGB = game->texturesRGB;
	Sys_PrintHeading ( "--- GameSpecific ---\n" );
	Sys_Printf( " max surface verts: %i\n" , game->maxSurfaceVerts);
	Sys_Printf( " max lightmapped surface verts: %i\n" , game->maxLMSurfaceVerts);
	Sys_Printf( " max surface indexes: %i\n" , game->maxSurfaceIndexes);
	Sys_Printf( " emit flares: %s\n", emitFlares == qtrue ? "enabled" : "disabled");
	Sys_Printf( " entity _color keys colorspace: %s\n", colorsRGB == qtrue ? "sRGB" : "linear"  );
	Sys_Printf( " texture default colorspace: %s\n", texturesRGB == qtrue ? "sRGB" : "linear" );
#if 0
	{
		unsigned int n, numCycles, f, fOld, start;
		vec3_t testVec;
		numCycles = 4000000000;
		Sys_PrintHeading ( "--- Test Speeds ---\n" );
		Sys_Printf(  "%9i cycles\n", numCycles );

		/* pass 1 */
		Sys_PrintHeading ( "--- Pass 1 ---\n" );
		start = I_FloatTime();
		fOld = -1;
		VectorSet(testVec, 5.0f, 5.0f, 5.0f);
		for( n = 0; n < numCycles; n++ )
		{
			/* print pacifier */
			f = 10 * n / numCycles;
			if( f != fOld )
			{
				fOld = f;
				printProgressVerbose( f );
			}

			/* process */
			VectorCompare(testVec, testVec);
		}
		Sys_FPrintf(SYS_VRB, " (%d)\n", (int) (I_FloatTime() - start) );

		/* pass 2 */
		Sys_PrintHeading ( "--- Pass 2 ---\n" );
		start = I_FloatTime();
		fOld = -1;
		VectorSet(testVec, 5.0f, 5.0f, 5.0f);
		for( n = 0; n < numCycles; n++ )
		{
			/* print pacifier */
			f = 10 * n / numCycles;
			if( f != fOld )
			{
				fOld = f;
				printProgressVerbose( f );
			}

			/* process */
			VectorCompare2(testVec, testVec);
		}
		Sys_FPrintf(SYS_VRB, " (%d)\n", (int) (I_FloatTime() - start) );
		Error("***");
	}
#endif

	randomizeBSP = qfalse;

	Sys_PrintHeading ( "--- CommandLine ---\n" );
	
	/* process arguments */
	for( i = 1; i < (argc - 1) && argv[ i ]; i++ )
	{
		if( !strcmp( argv[ i ], "-tempname" ) )
			strcpy( tempSource, argv[ ++i ] );
		else if( !strcmp( argv[ i ], "-tmpout" ) )
			strcpy( outbase, "/tmp" );
		else if( !strcmp( argv[ i ],  "-nowater" ) )
		{
			Sys_Printf( " Disabling water\n" );
			nowater = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nodetail" ) )
		{
			Sys_Printf( " Ignoring detail brushes\n") ;
			nodetail = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-fulldetail" ) )
		{
			Sys_Printf( " Turning detail brushes into structural brushes\n" );
			fulldetail = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nodetailcollision" ) )
		{
			Sys_Printf( " Disabling collision for detail brushes\n" );
			nodetailcollision = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nofoliage" ) )
		{
			Sys_Printf( " Disabling foliage\n" );
			nofoliage = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nodecals" ) )
		{
			Sys_Printf( " Disabling decals\n" );
			nodecals = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nofog" ) )
		{
			Sys_Printf( " Fog volumes disabled\n" );
			nofog = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-nosubdivide" ) )
		{
			Sys_Printf( " Disabling brush face subdivision\n" );
			nosubdivide = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-noclipmodel" ) )
		{
			Sys_Printf( " Disabling misc_model autoclip feature\n" );
			noclipmodel = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-leaktest" ) )
		{
			Sys_Printf( " Leaktest enabled\n" );
			leaktest = qtrue;
		}
		else if( !strcmp( argv[ i ], "-nocurves" ) )
		{
			Sys_Printf( " Ignoring curved surfaces (patches)\n" );
			noCurveBrushes = qtrue;
		}
		else if( !strcmp( argv[ i ], "-notjunc" ) )
		{
			Sys_Printf( " Disabling T-junction fixing\n" );
			noTJunc = qtrue;
		}
		else if( !strcmp( argv[ i ], "-noclip" ) )
		{
			Sys_Printf( " Disabling face clipping by BSP tree\n" );
			noclip = qtrue;
		}
		else if( !strcmp( argv[ i ], "-fakemap" ) )
		{
			Sys_Printf( " Generating fakemap.map\n" );
			fakemap = qtrue;
		}
		else if( !strcmp( argv[ i ],  "-samplesize" ) )
 		{
			sampleSize = atof( argv[ i + 1 ] );
			if( sampleSize < MIN_LIGHTMAP_SAMPLE_SIZE )
				sampleSize = MIN_LIGHTMAP_SAMPLE_SIZE;
 			i++;
			Sys_Printf( " Lightmap sample size set to %fx%f units\n", sampleSize, sampleSize );
 		}
		else if( !strcmp( argv[ i ], "-lightmapsize" ) )
		{
			lmCustomSize = atoi( argv[ i + 1 ] );

			/* must be a power of 2 and greater than 2 */
			if( ((lmCustomSize - 1) & lmCustomSize) || lmCustomSize < 2 )
			{
				Sys_Warning( "Given lightmap size (%i) must be a power of 2, greater or equal to 2 pixels.", lmCustomSize );
				lmCustomSize = game->lightmapSize;
				
			}
			Sys_Printf( " Default lightmap size set to %d x %d pixels\n", lmCustomSize, lmCustomSize );
			i++;
		}
		else if( !strcmp( argv[ i ], "-maxsurfacelightmapsize" ) )
		{
			lmMaxSurfaceSize = atoi( argv[ i + 1 ] );
			i++;
			Sys_Printf( " Max surface lightmap size set to %d x %d pixels\n", lmMaxSurfaceSize, lmMaxSurfaceSize );
		}
		else if( !strcmp( argv[ i ],  "-entitysaveid") )
		{
			Sys_Printf( " Entity unique savegame identifiers enabled\n" );
			useEntitySaveId = qtrue;
		}
		/* sof2 args */
		else if( !strcmp( argv[ i ], "-rename" ) )
		{
			Sys_Printf( " Appending _bsp suffix to misc_model shaders (SOF2)\n" );
			renameModelShaders = qtrue;
		}
		
		/* ydnar args */
		else if( !strcmp( argv[ i ],  "-ne" ) )
 		{
			normalEpsilon = atof( argv[ i + 1 ] );
 			i++;
			Sys_Printf( " Normal epsilon set to %f\n", normalEpsilon );
 		}
		else if( !strcmp( argv[ i ],  "-de" ) )
 		{
			distanceEpsilon = atof( argv[ i + 1 ] );
 			i++;
			Sys_Printf( " Distance epsilon set to %f\n", distanceEpsilon );
 		}
		else if( !strcmp( argv[ i ],  "-mv" ) )
 		{
			maxLMSurfaceVerts = atoi( argv[ i + 1 ] );
			if( maxLMSurfaceVerts < 3 )
				maxLMSurfaceVerts = 3;
			Sys_Printf( " Maximum lightmapped surface vertex count set to %d\n", maxLMSurfaceVerts );
			if( maxLMSurfaceVerts > maxSurfaceVerts )
			{
				maxSurfaceVerts = maxLMSurfaceVerts;
				Sys_Printf( " Maximum surface vertex count set to %d\n", maxSurfaceVerts );
			}
 			i++;
			Sys_Printf( " Maximum lightmapped surface vertex count set to %d\n", maxLMSurfaceVerts );
 		}
		else if( !strcmp( argv[ i ],  "-mi" ) )
 		{
			maxSurfaceIndexes = atoi( argv[ i + 1 ] );
			if( maxSurfaceIndexes < 3 )
				maxSurfaceIndexes = 3;
 			i++;
			Sys_Printf( " Maximum per-surface index count set to %d\n", maxSurfaceIndexes );
 		}
		else if( !strcmp( argv[ i ], "-np" ) )
		{
			npDegrees = atof( argv[ i + 1 ] );
			if( npDegrees < 0.0f )
				shadeAngleDegrees = 0.0f;
			else if( npDegrees > 0.0f )
				Sys_Printf( " Forcing nonplanar surfaces with a breaking angle of %f degrees\n", npDegrees );
			i++;
		}
		else if( !strcmp( argv[ i ],  "-snap" ) )
 		{
			bevelSnap = atoi( argv[ i + 1 ]);
			if( bevelSnap < 0 )
				bevelSnap = 0;
 			i++;
			if( bevelSnap > 0 )
				Sys_Printf( " Snapping brush bevel planes to %d units\n", bevelSnap );
 		}
		else if( !strcmp( argv[ i ],  "-texrange" ) )
 		{
			texRange = atoi( argv[ i + 1 ]);
			if( texRange < 0 )
				texRange = 0;
 			i++;
			Sys_Printf( " Limiting per-surface texture range to %d texels\n", texRange );
 		}
		else if( !strcmp( argv[ i ], "-nohint" ) )
		{
			Sys_Printf( " Hint brushes disabled\n" );
			noHint = qtrue;
		}
		else if( !strcmp( argv[ i ], "-flat" ) )
		{
			Sys_Printf( " Flatshading enabled\n" );
			flat = qtrue;
		}
		else if( !strcmp( argv[ i ], "-meta" ) )
		{
			Sys_Printf( " Creating meta surfaces from brush faces\n" );
			meta = qtrue;
		}
		else if( !strcmp( argv[ i ], "-patchmeta" ) )
		{
			Sys_Printf( " Creating meta surfaces from patches\n" );
			patchMeta = qtrue;
		}
		else if( !strcmp( argv[ i ], "-flares" ) )
		{
			Sys_Printf( " Flare surfaces enabled\n" );
			emitFlares = qtrue;
		}
		else if( !strcmp( argv[ i ], "-noflares" ) )
		{
			Sys_Printf( " Flare surfaces disabled\n" );
			emitFlares = qfalse;
		}
		else if( !strcmp( argv[ i ], "-skyfix" ) )
		{
			Sys_Printf( " GL_CLAMP sky fix/hack/workaround enabled\n" );
			skyFixHack = qtrue;
		}
		else if( !strcmp( argv[ i ], "-debugsurfaces" ) )
		{
			Sys_Printf( " emitting debug surfaces\n" );
			debugSurfaces = qtrue;
		}
		else if( !strcmp( argv[ i ], "-debuginset" ) )
		{
			Sys_Printf( " Debug surface triangle insetting enabled\n" );
			debugInset = qtrue;
		}
		else if( !strcmp( argv[ i ], "-debugportals" ) )
		{
			Sys_Printf( " Debug portal surfaces enabled\n" );
			debugPortals = qtrue;
		}
		else if( !strcmp( argv[ i ], "-sRGBtex" ) ) 
		{
			texturesRGB = qtrue;
			Sys_Printf( "Default texture colorspace: sRGB\n" );
		}
		else if( !strcmp( argv[ i ], "-nosRGBtex" ) ) 
		{
			texturesRGB = qfalse;
			Sys_Printf( "Default texture colorspace: linear\n" );
		}
		else if( !strcmp( argv[ i ], "-sRGBcolor" ) ) 
		{
			colorsRGB = qtrue;
			Sys_Printf( "Entity _color keys colorspace: sRGB\n" );
		}
		else if( !strcmp( argv[ i ], "-nosRGBcolor" ) ) 
		{
			colorsRGB = qfalse;
			Sys_Printf( "Entity _color keys colorspace: linear\n" );
		}
		else if ( !strcmp( argv[ i ], "-nosRGB" ) ) 
		{
			texturesRGB = qfalse;
			Sys_Printf( "Default texture colorspace: linear\n" );
			colorsRGB = qfalse;
			Sys_Printf( "Entity _color keys colorspace: linear\n" );
		}
		else if (!strcmp(argv[i], "-draw"))
		{
			Sys_Printf("SDL BSP tree viewer enabled\n");
			drawBSP = qtrue;
		}
		else if (!strcmp(argv[i], "-randomize"))
		{
			Sys_Printf("Height randomization enabled\n");
			randomizeBSP = qtrue;
		}
		else if( !strcmp( argv[ i ], "-bsp" ) )
			Sys_Printf( " -bsp argument unnecessary\n" );
		else
			Sys_Warning( "Unknown option \"%s\"", argv[ i ] );
	}

#if 0
	if (bevelSnap <= 2)
	{// UQ1: Snap bevel planes to minimum of 2 Q3 units to reduce waste.
		bevelSnap = 2;
		Sys_Printf(" Forced snapping of brush bevel planes to %d units to reduce plane count\n", bevelSnap);
	}
#endif

	/* set up lmMaxSurfaceSize */
	if (lmMaxSurfaceSize == 0)
		lmMaxSurfaceSize = lmCustomSize;
	
	/* fixme: print more useful usage here */
	if( i != (argc - 1) )
		Error( "usage: wzmap [options] mapfile" );
	
	/* copy source name */
	strcpy( source, ExpandArg( argv[ i ] ) );
	StripExtension( source );

	strcpy(currentMapName, source);

	/* load shaders */
	LoadShaderInfo();
	
	/* UQ1: generateforest */
	//else if( !strcmp( argv[ i ], "-generateforest" ) )
	{
		char filename[1024] = { 0 };
		generateforest = qfalse;
		generatecity = qfalse;

		sprintf(filename, "%s.foliage", source);
		
		if (!FOLIAGE_LoadFoliagePositions( filename ))
		{
			Sys_Printf( "Failed to load foliage file. No forests will be added...\n" );

			char filename2[1024] = { 0 };
			sprintf(filename2, "%s.climate", source);
			FOLIAGE_LoadClimateData(filename2);
		}
		else
		{
			char filename2[1024] = { 0 };

			generateforest = qtrue;
			generatecity = qtrue;

			Sys_PrintHeading ( "--- Procedural geometry generation enabled ---\n" );
			Sys_Printf( "Loaded %i tree points (%i special surfaces) from foliage file.\n", FOLIAGE_NUM_POSITIONS, FOLIAGE_NOT_GROUND_COUNT);

			sprintf(filename2, "%s.climate", source);
			FOLIAGE_LoadClimateData(filename2);
		}
	}

	/* ydnar: set default sample size */
	SetDefaultSampleSize( sampleSize );
	
	/* delete portal, line and surface files */
	sprintf( path, "%s.prt", source );
	remove( path );
	sprintf( path, "%s.lin", source );
	remove( path );
	//%	sprintf( path, "%s.srf", source );	/* ydnar */
	//%	remove( path );
	
	/* expand mapname */
	strcpy( name, ExpandArg( argv[ i ] ) );	
	if( !strcmp( name + strlen( name ) - 4, ".reg" ) )
	{
		/* building as region map */
		mapRegion = qtrue;
		mapRegionBrushes = qfalse; // disable region brushes
		Sys_Printf( "Running region compile\n" );
	}
	else
	{
		/* if we are doing a full map, delete the last saved region map */
		sprintf( path, "%s.reg", source );
		remove( path );
		DefaultExtension( name, ".map" );	/* might be .reg */
	}

	/* load original file from temp spot in case it was renamed by the editor on the way in */
	if( strlen( tempSource ) > 0 )
		LoadMapFile( tempSource, qfalse, qfalse, qfalse, qfalse );
	else
		LoadMapFile( name, qfalse, qfalse, qfalse, qfalse );

	/* check map for errors */
	CheckMapForErrors();

	/* load up decorations */
	LoadDecorations( source );

	/* vortex: preload triangle models */
	LoadTriangleModels();

	/* cull stuff for region */
	RegionScissor();
	
	/* vortex: decorator */
	ProcessDecorations();

	/* process decals */
	ProcessDecals();

	/* ydnar: cloned brush model entities */
	SetCloneModelNumbers();

	/* process world and submodels */
	ProcessModels();
	
	/* set light styles from targetted light entities */
	SetLightStyles();
	
	/* finish and write bsp */
	EndBSPFile();

	if (USE_SECONDARY_BSP)
	{
		GENERATING_SECONDARY_BSP = qtrue;

		{
			int		i, j;

			/* shaders (don't swap the name) */
			numBSPShaders = 0;

			/* planes */
			//free(bspPlanes);
			numBSPPlanes = 0;
			//bspPlanes = (bspPlane_t*)safe_malloc(numBSPPlanes * sizeof(bspPlanes[0]));

			/* nodes */
			numBSPNodes = 0;
			//bspNodes = (bspNode_t*)safe_malloc(numBSPNodes * sizeof(bspNodes[0]));

			/* leafs */
			numBSPLeafs = 0;
			//bspLeafs = (bspLeaf_t*)safe_malloc(numBSPLeafs * sizeof(bspLeafs[0]));

			/* leaffaces */
			numBSPLeafSurfaces = 0;
			//bspLeafSurfaces = (int*)safe_malloc(numBSPLeafSurfaces * sizeof(bspLeafSurfaces[0]));

			/* leafbrushes */
			numBSPLeafBrushes = 0;
			//bspLeafBrushes = (int*)safe_malloc(numBSPLeafBrushes * sizeof(bspLeafBrushes[0]));

			// brushes
			numBSPBrushes = 0;
			//bspBrushes = (bspBrush_t*)safe_malloc(numBSPBrushes * sizeof(bspBrushes[0]));

			// brushsides
			numBSPBrushSides = 0;
			//bspBrushSides = (bspBrushSide_t*)safe_malloc(numBSPBrushSides * sizeof(bspBrushSides[0]));

			// vis
			//((int*)&bspVisBytes)[0] = LittleLong(((int*)&bspVisBytes)[0]);
			//((int*)&bspVisBytes)[1] = LittleLong(((int*)&bspVisBytes)[1]);
			//bspVisBytes = (byte*)realloc(bspVisBytes, numBSPVisBytes * sizeof( byte ));
			numBSPVisBytes = 0;

			/* drawverts (don't swap colors) */
			//free(bspDrawVerts);
			numBSPDrawVerts = 0;
			//bspDrawVerts = (bspDrawVert_t*)safe_malloc(numBSPDrawVerts * sizeof(bspDrawVerts[0]));

			/* drawindexes */
			numBSPDrawIndexes = 0;
			
			/* drawsurfs */
			/* note: rbsp files (and hence q3map2 abstract bsp) have byte lightstyles index arrays, this follows sof2map convention */
			numBSPDrawSurfaces = 0;
			//bspDrawSurfaces = (bspDrawSurface_t*)safe_malloc(numBSPDrawSurfaces * sizeof(bspDrawSurfaces[0]));

			/* fogs */
			numBSPFogs = 0;
		}

		/* process world and submodels */
		ProcessModels();

		/* finish and write bsp */
		EndBSPFile();
	}
	
	/* remove temp map source file if appropriate */
	if( strlen( tempSource ) > 0)
		remove( tempSource );
	
	/* return to sender */
	return 0;
}

