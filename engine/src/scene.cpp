///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "scene.h"

#include "render.h"

#include <algorithm>

#include "dbgalloc.h" // must be last header

F_DECLARE_INTERFACE(IInput);

extern ISceneEntityEnum * SceneEntityEnumCreate(const tSceneEntityList & entities);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScene
//

BEGIN_CONSTRAINTS()
   AFTER_GUID(IID_IInput)
END_CONSTRAINTS()

///////////////////////////////////////

cScene::cScene()
 : cGlobalObject<IMPLEMENTS(IScene)>("Scene", CONSTRAINTS())
{
}

///////////////////////////////////////

cScene::~cScene()
{
}

///////////////////////////////////////

tResult cScene::Init()
{
   UseGlobal(Input);
   pInput->Connect(&m_inputListener);
   return S_OK;
}

///////////////////////////////////////

tResult cScene::Term()
{
   UseGlobal(Input);
   pInput->Disconnect(&m_inputListener);

   Clear();

   return S_OK;
}

///////////////////////////////////////

tResult cScene::AddEntity(eSceneLayer layer, ISceneEntity * pEntity)
{
   return m_layers[layer].AddEntity(pEntity);
}

///////////////////////////////////////

tResult cScene::RemoveEntity(eSceneLayer layer, ISceneEntity * pEntity)
{
   return m_layers[layer].RemoveEntity(pEntity);
}

///////////////////////////////////////

tResult cScene::SetCamera(eSceneLayer layer, ISceneCamera * pCamera)
{
   return m_layers[layer].SetCamera(pCamera);
}

///////////////////////////////////////

tResult cScene::GetCamera(eSceneLayer layer, ISceneCamera * * ppCamera)
{
   return m_layers[layer].GetCamera(ppCamera);
}

///////////////////////////////////////

void cScene::Clear(eSceneLayer layer)
{
   m_layers[layer].Clear();
}

///////////////////////////////////////

void cScene::Clear()
{
   for (int i = 0; i < _countof(m_layers); i++)
   {
      m_layers[i].Clear();
   }
}

///////////////////////////////////////

tResult cScene::AddInputListener(eSceneLayer layer, IInputListener * pListener)
{
   return m_layers[layer].AddInputListener(pListener);
}

///////////////////////////////////////

tResult cScene::RemoveInputListener(eSceneLayer layer, IInputListener * pListener)
{
   return m_layers[layer].RemoveInputListener(pListener);
}

///////////////////////////////////////

tResult cScene::Render(IRenderDevice * pRenderDevice)
{
   for (int i = 0; i < _countof(m_layers); i++)
   {
      cAutoIPtr<ISceneCamera> pCamera;
      if (m_layers[i].GetCamera(&pCamera) == S_OK)
      {
         pRenderDevice->SetProjectionMatrix(pCamera->GetProjectionMatrix());
         pRenderDevice->SetViewMatrix(pCamera->GetViewMatrix());
      }
      tResult result = m_layers[i].Render(pRenderDevice);
      if (result != S_OK)
         return result;
   }
   return S_OK;
}

///////////////////////////////////////

tResult cScene::Query(const cRay & ray, tSceneEntityList * pEntities)
{
   Assert(pEntities != NULL);
   for (int i = 0; i < _countof(m_layers); i++)
   {
      m_layers[i].Query(ray, pEntities);
   }
   return pEntities->empty() ? S_FALSE : S_OK;
}

///////////////////////////////////////

tResult cScene::Query(const cRay & ray, ISceneEntityEnum * * ppEnum)
{
   tSceneEntityList entities;
   for (int i = 0; i < _countof(m_layers); i++)
   {
      m_layers[i].Query(ray, &entities);
   }

   tResult result = S_FALSE;

   if (!entities.empty())
   {
      *ppEnum = SceneEntityEnumCreate(entities);
      result = S_OK;
   }

   std::for_each(entities.begin(), entities.end(), CTInterfaceMethodRef(&::IUnknown::Release));

   return result;
}

///////////////////////////////////////

#define GetOuter(Class, Member) ((Class *)((byte *)this - offsetof(Class, Member)))

bool cScene::cInputListener::OnMouseEvent(int x, int y, uint mouseState, double time)
{
   cScene * pScene = GetOuter(cScene, m_inputListener);

   for (int i = _countof(pScene->m_layers) - 1; i >= 0; i--)
   {
      if (pScene->m_layers[i].HandleMouseEvent(x, y, mouseState, time))
      {
         return true;
      }
   }

   return false;
}

///////////////////////////////////////

bool cScene::cInputListener::OnKeyEvent(long key, bool down, double time)
{
   cScene * pScene = GetOuter(cScene, m_inputListener);

   for (int i = _countof(pScene->m_layers) - 1; i >= 0; i--)
   {
      if (pScene->m_layers[i].HandleKeyEvent(key, down, time))
      {
         return true;
      }
   }

   return false;
}

///////////////////////////////////////

void SceneCreate()
{
   cAutoIPtr<IScene>(new cScene);
}

///////////////////////////////////////////////////////////////////////////////
