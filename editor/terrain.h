/////////////////////////////////////////////////////////////////////////////
// $Id$

#if !defined(INCLUDED_TERRAIN_H)
#define INCLUDED_TERRAIN_H

#include "comtools.h"
#include "vec2.h"
#include "vec3.h"

#include <vector>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

F_DECLARE_INTERFACE(IEditorTileSet);
F_DECLARE_INTERFACE(IMaterial);
F_DECLARE_INTERFACE(IRenderDevice);
F_DECLARE_INTERFACE(IVertexDeclaration);
class cHeightMap;

/////////////////////////////////////////////////////////////////////////////

struct sTerrainVertex
{
   tVec2 uv1;
   tVec3 rgb;
   tVec3 pos;
};

tResult TerrainVertexDeclarationCreate(IRenderDevice * pRenderDevice,
                                       IVertexDeclaration * * ppVertexDecl);

/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrain
//
// Simple terrain data class. For rendering tile-based terrain with artist-
// generated transition tiles. Not a LOD terrain renderer implementing the
// ROAM algorithm or anything like that.

const int kDefaultStepSize = 16;
const int kTilesPerChunk = 32;

class cTerrain
{
public:
   cTerrain();
   ~cTerrain();

   bool Create(uint xDim, uint zDim, int stepSize, IEditorTileSet * pTileSet,
      uint defaultTile, cHeightMap * pHeightMap);

   void GetDimensions(uint * pxd, uint * pzd) const;
   void GetExtents(uint * px, uint * pz) const;

   const sTerrainVertex * GetVertexPointer() const;
   size_t GetVertexCount() const;

   void Render(IRenderDevice * pRenderDevice);

protected:
   void InitializeVertices(uint xDim, uint zDim, int stepSize, cHeightMap * pHeightMap);
   bool CreateTerrainChunks();

private:
   uint m_xDim, m_zDim;
   uint m_tileSize;

   uint m_xChunks, m_zChunks; // # of terrain chunks in the x, z directions

   std::vector<sTerrainVertex> m_vertices;

   cAutoIPtr<IMaterial> m_pMaterial;
};


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainChunk
//

class cTerrainChunk
{
public:
   cTerrainChunk();
   ~cTerrainChunk();

private:
   cAutoIPtr<IMaterial> m_pMaterial;
};


/////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_TERRAIN_H
