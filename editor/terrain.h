/////////////////////////////////////////////////////////////////////////////
// $Id$

#if !defined(INCLUDED_TERRAIN_H)
#define INCLUDED_TERRAIN_H

#include "comtools.h"
#include "vec2.h"
#include "vec3.h"
#include "str.h"

#include <vector>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

F_DECLARE_INTERFACE(IEditorTileSet);
F_DECLARE_INTERFACE(IMaterial);
F_DECLARE_INTERFACE(IRenderDevice);
F_DECLARE_INTERFACE(IVertexDeclaration);
F_DECLARE_INTERFACE(IReader);
F_DECLARE_INTERFACE(IWriter);
class cHeightMap;

class cTerrain;
class cTerrainTile;
class cTerrainChunk;

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

const int kDefaultStepSize = 32;
const int kTilesPerChunk = 32;

class cTerrain
{
public:
   cTerrain();
   ~cTerrain();

   tResult Read(IReader * pReader);
   tResult Write(IWriter * pWriter);

   bool Create(uint xDim, uint zDim, int stepSize, IEditorTileSet * pTileSet,
      uint defaultTile, cHeightMap * pHeightMap);

   void GetDimensions(uint * pxd, uint * pzd) const;
   void GetExtents(uint * px, uint * pz) const;

   void GetTileIndices(float x, float z, uint * pix, uint * piz);

   const sTerrainVertex * GetVertexPointer() const;
   size_t GetVertexCount() const;

   tResult Render(IRenderDevice * pRenderDevice);

   cTerrainTile * GetTile(uint ix, uint iz);

protected:
   void InitializeVertices(uint xDim, uint zDim, int stepSize, cHeightMap * pHeightMap);
   bool CreateTerrainChunks();

private:
   uint m_xDim, m_zDim;
   uint m_tileSize; // dimensions of a single tile in terrain space

   uint m_xChunks, m_zChunks; // # of terrain chunks in the x, z directions

   std::vector<sTerrainVertex> m_vertices;

   cStr m_tileSetName;
   cAutoIPtr<IEditorTileSet> m_pTileSet;

   cAutoIPtr<IMaterial> m_pMaterial;

   std::vector<cTerrainTile> m_tiles;
};


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainTile
//

class cTerrainTile
{
public:
   cTerrainTile();
   ~cTerrainTile();

   void SetVertices(const sTerrainVertex * pVertices);
   const sTerrainVertex * GetVertices() const;

   void SetTile(uint tile);
   uint GetTile() const;

private:
   sTerrainVertex m_vertices[4]; // one quad
   uint m_tile;
};

///////////////////////////////////////

inline const sTerrainVertex * cTerrainTile::GetVertices() const
{
   return m_vertices;
}

///////////////////////////////////////

inline void cTerrainTile::SetTile(uint tile)
{
   m_tile = tile;
}

///////////////////////////////////////

inline uint cTerrainTile::GetTile() const
{
   return m_tile;
}


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
