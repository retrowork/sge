///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "renderapi.h"
#include "scenecamera.h"

#include "dbgalloc.h" // must be last header

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSceneCamera
//

///////////////////////////////////////

ISceneCamera * SceneCameraCreate()
{
   return static_cast<ISceneCamera *>(new cSceneCamera);
}

///////////////////////////////////////

cSceneCamera::cSceneCamera()
 : m_projectionType(kPT_Orthographic),
   m_projection(tMatrix4::GetIdentity()),
   m_view(tMatrix4::GetIdentity())
{
}

///////////////////////////////////////

cSceneCamera::~cSceneCamera()
{
}

///////////////////////////////////////

void cSceneCamera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
   m_projectionType = kPT_Perspective;
   MatrixPerspective(fov, aspect, znear, zfar, &m_projection);
   UpdateCompositeMatrices();
}

///////////////////////////////////////

void cSceneCamera::SetOrtho(float left, float right, float bottom, float top, float znear, float zfar)
{
   m_projectionType = kPT_Orthographic;
   MatrixOrtho(left, right, bottom, top, znear, zfar, &m_projection);
   UpdateCompositeMatrices();
}

///////////////////////////////////////

void cSceneCamera::UpdateCompositeMatrices()
{
   GetProjectionMatrix().Multiply(GetViewMatrix(), &m_viewProjection);
   MatrixInvert(GetViewProjectionMatrix().m, m_viewProjectionInverse.m);
   m_frustum.ExtractPlanes(GetViewProjectionMatrix());
}

///////////////////////////////////////////////////////////////////////////////
