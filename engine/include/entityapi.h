///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_ENTITYAPI_H
#define INCLUDED_ENTITYAPI_H

#include "enginedll.h"
#include "comtools.h"

#ifdef _MSC_VER
#pragma once
#endif

F_DECLARE_INTERFACE(IEntityManager);

F_DECLARE_INTERFACE(IRenderDevice);

class cTerrainLocatorHack
{
public:
   virtual void Locate(float nx, float nz, float * px, float * py, float * pz) = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IEntityManager
//

interface IEntityManager : IUnknown
{
   virtual void SetRenderDeviceHack(IRenderDevice *) = 0;
   virtual void SetTerrainLocatorHack(cTerrainLocatorHack *) = 0;

   virtual tResult SpawnEntity(const char * pszMesh, float x, float z) = 0;
};

////////////////////////////////////////

ENGINE_API void EntityManagerCreate();

///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_ENTITYAPI_H
