///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_MS3DMESH_H
#define INCLUDED_MS3DMESH_H

#include "skeleton.h"

#include "readwriteapi.h"
#include "matrix4.h"
#include "vec3.h"

#include <vector>

#ifdef _MSC_VER
#pragma once
#endif

F_DECLARE_INTERFACE(IKeyFrameInterpolator);
F_DECLARE_INTERFACE(IMaterial);
F_DECLARE_INTERFACE(IResourceManager);
F_DECLARE_INTERFACE(IRenderDevice);

typedef struct _CGprogram * CGprogram;
typedef struct _CGparameter * CGparameter;

const int kMaxBoneName = 32;

///////////////////////////////////////////////////////////////////////////////
// MS3D file format structs

#pragma pack(push,1)

typedef struct
{
   char    id[10];                  // always "MS3D000000"
   int     version;                 // 4
} ms3d_header_t;

typedef struct
{
   byte    flags;                   // SELECTED | SELECTED2 | HIDDEN
   float   vertex[3];               //
   char    boneId;                  // -1 = no bone
   byte    referenceCount;
} ms3d_vertex_t;

typedef struct
{
   uint16    flags;                 // SELECTED | SELECTED2 | HIDDEN
   uint16    vertexIndices[3];      //
   float   vertexNormals[3][3];     //
   float   s[3];                    //
   float   t[3];                    //
   byte    smoothingGroup;          // 1 - 32
   byte    groupIndex;              //
} ms3d_triangle_t;

typedef struct
{
   char            name[32];        //
   float           ambient[4];      //
   float           diffuse[4];      //
   float           specular[4];     //
   float           emissive[4];     //
   float           shininess;       // 0.0f - 128.0f
   float           transparency;    // 0.0f - 1.0f
   char            mode;            // 0, 1, 2 is unused now
   char            texture[128];    // texture.bmp
   char            alphamap[128];   // alpha.bmp
} ms3d_material_t;

typedef struct
{
   float           time;            // time in seconds
   float           rotation[3];     // x, y, z angles
} ms3d_keyframe_rot_t;

typedef struct
{
   float           time;            // time in seconds
   float           position[3];     // local position
} ms3d_keyframe_pos_t;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMs3dGroup
//

class cMs3dGroup
{
   friend class cReadWriteOps<cMs3dGroup>;

public:
   cMs3dGroup();

   const char * GetName() const { return name; }

   int GetMaterialIndex() const
   {
      return materialIndex;
   }

   const std::vector<uint16> & GetTriangleIndices() const
   {
      return triangleIndices;
   }

private:
   char name[32];
   std::vector<uint16> triangleIndices;
   char materialIndex;
};


//////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMs3dMesh
//

class cMs3dMesh
{
   cMs3dMesh(const cMs3dMesh &); // private, un-implemented
   const cMs3dMesh & operator =(const cMs3dMesh &); // private, un-implemented

public:
   cMs3dMesh();
   ~cMs3dMesh();

   void GetAABB(tVec3 * pMaxs, tVec3 * pMins) const;

   inline void Render() const
   {
      (this->*m_pfnRender)();
   }

   tResult Read(IReader * pReader, IRenderDevice * pRenderDevice, IResourceManager * pResourceManager);

   void Reset();

   void SetFrame(float percent);

   // using software or vertex program rendering?
   bool IsRenderingSoftware() const;

   int GetMaterialCount() const { return m_materials.size(); }
   IMaterial * AccessMaterial(int index) { return m_materials[index]; }

   int GetGroupCount() const { return m_groups.size(); }
   const cMs3dGroup & GetGroup(int index) const { return m_groups[index]; }

   const cSkeleton * GetSkeleton() const { return &m_skeleton; }

private:
   typedef void (cMs3dMesh:: * tRenderMethod)() const;

   tRenderMethod m_pfnRender;

   void RenderSoftware() const;
   void RenderVertexProgram() const;

   typedef std::vector<ms3d_vertex_t> tVertices;
   typedef std::vector<ms3d_triangle_t> tTriangles;
   typedef std::vector<cMs3dGroup> tGroups;
   typedef std::vector<IMaterial *> tMaterials;

   tVertices m_vertices;
   tTriangles m_triangles;
   tGroups m_groups;
   tMaterials m_materials;

   mutable tVec3 m_maxs, m_mins;
   mutable bool m_bCalculatedAABB;

   cSkeleton m_skeleton;

   tMatrices m_boneMatrices;

   CGprogram m_program;
   CGparameter m_modelViewProjParam;
};

inline bool cMs3dMesh::IsRenderingSoftware() const
{
   return m_pfnRender == RenderSoftware;
}

///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_MS3DMESH_H
