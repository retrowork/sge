/////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "editorTools.h"
#include "editorTypes.h"
#include "editorCommands.h"
#include "resource.h"

#include "cameraapi.h"
#include "entityapi.h"
#include "terrainapi.h"

#include "globalobj.h"
#include "ray.h"

#include <GL/glew.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

LOG_DEFINE_CHANNEL(EditorTools);
#define LocalMsg(msg)            DebugMsgEx(EditorTools,(msg))
#define LocalMsg1(msg,a)         DebugMsgEx1(EditorTools,(msg),(a))
#define LocalMsg2(msg,a,b)       DebugMsgEx2(EditorTools,(msg),(a),(b))
#define LocalMsg3(msg,a,b,c)     DebugMsgEx3(EditorTools,(msg),(a),(b),(c))
#define LocalMsg4(msg,a,b,c,d)   DebugMsgEx4(EditorTools,(msg),(a),(b),(c),(d))

/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDragTool
//

////////////////////////////////////////

cDragTool::cDragTool()
{
}

////////////////////////////////////////

cDragTool::~cDragTool()
{
}

////////////////////////////////////////

tResult cDragTool::OnKeyDown(const cEditorKeyEvent & keyEvent, IEditorView * pView)
{
   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cDragTool::OnLButtonDown(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (pView != NULL)
   {
      m_pView = CTAddRef(pView);
      UseGlobal(EditorApp);
      pEditorApp->SetToolCapture(this);
      return OnDragStart(mouseEvent, pView);
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cDragTool::OnLButtonUp(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (IsDragging())
   {
      tResult result = OnDragEnd(mouseEvent, pView);
      SafeRelease(m_pView);
      UseGlobal(EditorApp);
      pEditorApp->ReleaseToolCapture();
      return result;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cDragTool::OnMouseMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (IsDragging())
   {
      return OnDragMove(mouseEvent, pView);
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

bool cDragTool::IsDragging()
{
   return (!!m_pView);
}

////////////////////////////////////////

IEditorView * cDragTool::AccessView()
{
   return m_pView;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSelectTool
//

////////////////////////////////////////

cSelectTool::cSelectTool()
 : m_selectType(kST_Ray)
{
}

////////////////////////////////////////

cSelectTool::~cSelectTool()
{
}

////////////////////////////////////////

tResult cSelectTool::OnKeyDown(const cEditorKeyEvent & keyEvent, IEditorView * pView)
{
   return cDragTool::OnKeyDown(keyEvent, pView);
}

////////////////////////////////////////

tResult cSelectTool::GetToolTip(const cEditorMouseEvent & mouseEvent,
                                cStr * pToolTipText, uint_ptr * pToolTipId) const
{
   cAutoIPtr<IEntity> pEntity;
   if (GetRayHitEntity(mouseEvent, &pEntity) == S_OK)
   {
      Sprintf(pToolTipText, "Entity 0x%p", static_cast<void*>(pEntity));
      *pToolTipId = reinterpret_cast<uint_ptr>(static_cast<void*>(pEntity.AccessPointer()));
      return S_OK;
   }

   return S_FALSE;
}

////////////////////////////////////////

tResult cSelectTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   m_selectType = kST_Ray;
   m_dragStartPoint = mouseEvent.GetPoint();
   m_lastMousePoint = mouseEvent.GetPoint();
   return S_EDITOR_TOOL_HANDLED;
}

////////////////////////////////////////

tResult cSelectTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if ((pView != NULL) && CTIsSameObject(pView, AccessView()))
   {
      if (m_selectType == kST_Ray)
      {
         UseGlobal(EntityManager);
         pEntityManager->DeselectAll();
         cAutoIPtr<IEntity> pEntity;
         if (GetRayHitEntity(mouseEvent, &pEntity) == S_OK)
         {
            pEntityManager->Select(pEntity);
         }
      }
      return S_EDITOR_TOOL_HANDLED;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cSelectTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if ((pView != NULL) && CTIsSameObject(pView, AccessView()))
   {
      CPoint delta = mouseEvent.GetPoint() - m_dragStartPoint;
      m_lastMousePoint = mouseEvent.GetPoint();
      return S_EDITOR_TOOL_HANDLED;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cSelectTool::GetRayHitEntity(const cEditorMouseEvent & mouseEvent, IEntity * * ppEntity) const
{
   if (ppEntity == NULL)
   {
      return E_POINTER;
   }

   float ndx, ndy;
   ScreenToNormalizedDeviceCoords(mouseEvent.GetPoint().x, mouseEvent.GetPoint().y, &ndx, &ndy);

   cRay pickRay;
   UseGlobal(Camera);
   if (pCamera->GeneratePickRay(ndx, ndy, &pickRay) == S_OK)
   {
      cAutoIPtr<IEntity> pEntity;
      UseGlobal(EntityManager);
      if (pEntityManager->RayCast(pickRay, &pEntity) == S_OK)
      {
         *ppEntity = CTAddRef(pEntity);
         return S_OK;
      }
   }

   return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMoveCameraTool
//

////////////////////////////////////////

cMoveCameraTool::cMoveCameraTool()
{
}

////////////////////////////////////////

cMoveCameraTool::~cMoveCameraTool()
{
}

////////////////////////////////////////

tResult cMoveCameraTool::OnKeyDown(const cEditorKeyEvent & keyEvent, IEditorView * pView)
{
   static const int kMove = 5;

   switch (keyEvent.GetChar())
   {
      case VK_LEFT:
      {
         MoveCamera(CPoint(-kMove,0));
         break;
      }
      case VK_RIGHT:
      {
         MoveCamera(CPoint(kMove,0));
         break;
      }
      case VK_UP:
      {
         MoveCamera(CPoint(0,-kMove));
         break;
      }
      case VK_DOWN:
      {
         MoveCamera(CPoint(0,kMove));
         break;
      }
   }

   return cDragTool::OnKeyDown(keyEvent, pView);
}

////////////////////////////////////////

tResult cMoveCameraTool::OnMouseWheel(const cEditorMouseWheelEvent & mouseWheelEvent, IEditorView * pView)
{
   static const float kMinElevation = 10;
   static const float kMaxElevation = 500;

   if (pView != NULL)
   {
      UseGlobal(CameraControl);
      if (mouseWheelEvent.GetZDelta() < 0)
      {
         pCameraControl->Lower();
      }
      else
      {
         pCameraControl->Raise();
      }

      //float elevation;
      //if (pView->GetCameraElevation(&elevation) == S_OK)
      //{
      //   elevation += (mouseWheelEvent.GetZDelta() / WHEEL_DELTA);
      //   if (elevation >= kMinElevation && elevation <= kMaxElevation)
      //   {
      //      pView->SetCameraElevation(elevation);
      //   }
      //}
   }

   return cDragTool::OnMouseWheel(mouseWheelEvent, pView);
}

////////////////////////////////////////

static bool GetTerrainLocation(const cRay & ray, tVec3 * pLocation)
{
   HTERRAINQUAD hQuad;
   UseGlobal(TerrainModel);
   if (pTerrainModel->GetQuadFromHitTest(ray, &hQuad) == S_OK)
   {
      tVec3 corners[4];
      if (pTerrainModel->GetQuadCorners(hQuad, corners) == S_OK)
      {
         if (ray.IntersectsTriangle(corners[0], corners[3], corners[2], pLocation)
            || ray.IntersectsTriangle(corners[2], corners[1], corners[0], pLocation))
         {
            return true;
         }
      }
   }
   return false;
}

////////////////////////////////////////

tResult cMoveCameraTool::GetToolTip(const cEditorMouseEvent & mouseEvent,
                                    cStr * pToolTipText, uint_ptr * pToolTipId) const
{
   float ndx, ndy;
   ScreenToNormalizedDeviceCoords(mouseEvent.GetPoint().x, mouseEvent.GetPoint().y, &ndx, &ndy);

   cRay pickRay;
   UseGlobal(Camera);
   if (pCamera->GeneratePickRay(ndx, ndy, &pickRay) == S_OK)
   {
      tVec3 intersect;
      if (GetTerrainLocation(pickRay, &intersect))
      {
         Sprintf(pToolTipText, _T("Hit <%.2f, %.2f, %.2f>"), intersect.x, intersect.y, intersect.z);
         *pToolTipId = static_cast<uint_ptr>(Round(intersect.LengthSqr()));
         return S_OK;
      }
   }

   return S_FALSE;
}

////////////////////////////////////////

tResult cMoveCameraTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   m_lastMousePoint = mouseEvent.GetPoint();
   return S_EDITOR_TOOL_HANDLED;
}

////////////////////////////////////////

tResult cMoveCameraTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if ((pView != NULL) && CTIsSameObject(pView, AccessView()))
   {
      //float camPlaceX, camPlaceZ;
      //if ((pView != NULL) && pView->GetCameraPlacement(&camPlaceX, &camPlaceZ) == S_OK)
      //{
      //   InfoMsg2("Looking at point (%.2f, 0, %.2f)\n", camPlaceX, camPlaceZ);
      //}

      return S_EDITOR_TOOL_HANDLED;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cMoveCameraTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if ((pView != NULL) && CTIsSameObject(pView, AccessView()))
   {
      CPoint delta = mouseEvent.GetPoint() - m_lastMousePoint;
      MoveCamera(delta);
      m_lastMousePoint = mouseEvent.GetPoint();
      return S_EDITOR_TOOL_HANDLED;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

void cMoveCameraTool::MoveCamera(CPoint delta)
{
   UseGlobal(CameraControl);
   if (delta.x < 0)
   {
      pCameraControl->MoveLeft();
   }
   else
   {
      pCameraControl->MoveRight();
   }
   if (delta.y < 0)
   {
      pCameraControl->MoveBack();
   }
   else
   {
      pCameraControl->MoveForward();
   }
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPlaceEntityTool
//

////////////////////////////////////////

cPlaceEntityTool::cPlaceEntityTool(const cStr & model)
 : m_model(model)
{
}

////////////////////////////////////////

cPlaceEntityTool::~cPlaceEntityTool()
{
}

////////////////////////////////////////

tResult cPlaceEntityTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cPlaceEntityTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (m_model.empty())
   {
      return S_EDITOR_TOOL_CONTINUE;
   }

   float ndx, ndy;
   ScreenToNormalizedDeviceCoords(mouseEvent.GetPoint().x, mouseEvent.GetPoint().y, &ndx, &ndy);

   cRay pickRay;
   UseGlobal(Camera);
   if (pCamera->GeneratePickRay(ndx, ndy, &pickRay) == S_OK)
   {
      tVec3 location;
      if (GetTerrainLocation(pickRay, &location))
      {
         cAutoIPtr<IEditorCommand> pCommand(static_cast<IEditorCommand*>(
            new cPlaceEntityCommand(m_model.c_str(), location)));
         if (!pCommand)
         {
            return E_OUTOFMEMORY;
         }

         UseGlobal(EditorApp);
         cAutoIPtr<IEditorModel> pEditorModel;
         if (pEditorApp->GetActiveModel(&pEditorModel) == S_OK)
         {
            pEditorModel->AddCommand(pCommand, true);
         }
      }
   }

   return S_EDITOR_TOOL_HANDLED;
}

////////////////////////////////////////

tResult cPlaceEntityTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   return S_EDITOR_TOOL_CONTINUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainTool
//

////////////////////////////////////////

cTerrainTool::cTerrainTool()
{
}

////////////////////////////////////////

cTerrainTool::~cTerrainTool()
{
}

////////////////////////////////////////

tResult cTerrainTool::Activate()
{
   return S_OK;
}

////////////////////////////////////////

tResult cTerrainTool::Deactivate()
{
   UseGlobal(TerrainRenderer);
   pTerrainRenderer->ClearHighlight();
   return S_OK;
}

////////////////////////////////////////

bool cTerrainTool::GetHitQuad(CPoint point, HTERRAINQUAD * phQuad)
{
   float ndx, ndy;
   ScreenToNormalizedDeviceCoords(point.x, point.y, &ndx, &ndy);

   cRay pickRay;
   UseGlobal(Camera);
   if (pCamera->GeneratePickRay(ndx, ndy, &pickRay) == S_OK)
   {
      UseGlobal(TerrainModel);
      return (pTerrainModel->GetQuadFromHitTest(pickRay, phQuad) == S_OK);
   }

   return false;
}

////////////////////////////////////////

bool cTerrainTool::GetHitVertex(CPoint point, HTERRAINVERTEX * phVertex)
{
   float ndx, ndy;
   ScreenToNormalizedDeviceCoords(point.x, point.y, &ndx, &ndy);

   cRay pickRay;
   UseGlobal(Camera);
   if (pCamera->GeneratePickRay(ndx, ndy, &pickRay) == S_OK)
   {
      UseGlobal(TerrainModel);
      return (pTerrainModel->GetVertexFromHitTest(pickRay, phVertex) == S_OK);
   }

   return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainTileTool
//

////////////////////////////////////////

cTerrainTileTool::cTerrainTileTool(uint tile)
 : m_tile(tile)
{
}

////////////////////////////////////////

cTerrainTileTool::~cTerrainTileTool()
{
}

////////////////////////////////////////

tResult cTerrainTileTool::OnMouseMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (pView != NULL)
   {
      HTERRAINQUAD hQuad = NULL;
      if (GetHitQuad(mouseEvent.GetPoint(), &hQuad))
      {
         UseGlobal(TerrainRenderer);
         pTerrainRenderer->HighlightTerrainQuad(hQuad);
      }
      else
      {
         UseGlobal(TerrainRenderer);
         pTerrainRenderer->ClearHighlight();
      }
   }

   return cTerrainTool::OnMouseMove(mouseEvent, pView);
}

////////////////////////////////////////

tResult cTerrainTileTool::GetToolTip(const cEditorMouseEvent & mouseEvent,
                                     cStr * pToolTipText, uint_ptr * pToolTipId) const
{
   if (pToolTipText == NULL || pToolTipId == NULL)
   {
      return E_POINTER;
   }

   HTERRAINQUAD hQuad = INVALID_HTERRAINQUAD;
   if (GetHitQuad(mouseEvent.GetPoint(), &hQuad))
   {
      UseGlobal(TerrainModel);

      uint tile = ~0;
      pTerrainModel->GetQuadTile(hQuad, &tile);

      int nNeighbors = 0;
      HTERRAINQUAD neighbors[8];
      if (pTerrainModel->GetQuadNeighbors(hQuad, neighbors) == S_OK)
      {
         for (int i = 0; i < _countof(neighbors); i++)
         {
            if (neighbors[i] != INVALID_HTERRAINQUAD)
            {
               nNeighbors += 1;
            }
         }
      }

      Sprintf(pToolTipText, "Tile %d; %d neighbors", tile, nNeighbors);
      *pToolTipId = reinterpret_cast<uint_ptr>(hQuad);
      return S_OK;
   }

   return S_FALSE;
}

////////////////////////////////////////

static void TerrainTileCompositeCommandCB(eEditorCompositeCommandCallback type)
{
   UseGlobal(TerrainRenderer);
   if (type == kPreDo || type == kPreUndo)
   {
      pTerrainRenderer->EnableBlending(false);
   }
   else if (type == kPostDo || type == kPostUndo)
   {
      pTerrainRenderer->EnableBlending(true);
   }
}

////////////////////////////////////////

tResult cTerrainTileTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   UseGlobal(TerrainRenderer);
   pTerrainRenderer->EnableBlending(false);

   m_hitQuads.clear();

   Assert(!m_pCommand);
   if (EditorCompositeCommandCreate(TerrainTileCompositeCommandCB, &m_pCommand) != S_OK)
   {
      ErrorMsg("Error creating composite command\n");
      return E_FAIL;
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cTerrainTileTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   // Do what is done on a move
   tResult result = OnDragMove(mouseEvent, pView);

   UseGlobal(TerrainRenderer);
   pTerrainRenderer->EnableBlending(true);

   Assert(!!m_pCommand);
   UseGlobal(EditorApp);
   cAutoIPtr<IEditorModel> pEditorModel;
   if (pEditorApp->GetActiveModel(&pEditorModel) == S_OK)
   {
      pEditorModel->AddCommand(m_pCommand, false);
   }
   SafeRelease(m_pCommand);

   m_hitQuads.clear();

   return result;
}

////////////////////////////////////////

tResult cTerrainTileTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   Assert(!!m_pCommand);

   HTERRAINQUAD hQuad = NULL;
   if (GetHitQuad(mouseEvent.GetPoint(), &hQuad)
      && m_hitQuads.find(hQuad) == m_hitQuads.end())
   {
      m_hitQuads.insert(hQuad);

      cAutoIPtr<IEditorCommand> pCommand(static_cast<IEditorCommand*>(
         new cTerrainTileCommand(hQuad, m_tile)));

      if (!pCommand)
      {
         return E_OUTOFMEMORY;
      }

      if (pCommand->Do() != S_OK)
      {
         return E_FAIL;
      }

      m_pCommand->Add(pCommand);
   }

   return S_EDITOR_TOOL_HANDLED;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainElevationTool
//

////////////////////////////////////////

cTerrainElevationTool::cTerrainElevationTool()
 : m_hHitVertex(INVALID_HTERRAINVERTEX)
 , m_elevDelta(0)
{
}

////////////////////////////////////////

cTerrainElevationTool::~cTerrainElevationTool()
{
}

////////////////////////////////////////

tResult cTerrainElevationTool::OnMouseMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (pView != NULL)
   {
      if (!IsDragging())
      {
         if (GetHitVertex(mouseEvent.GetPoint(), &m_hHitVertex))
         {
            UseGlobal(TerrainRenderer);
            pTerrainRenderer->HighlightTerrainVertex(m_hHitVertex);
         }
         else
         {
            UseGlobal(TerrainRenderer);
            pTerrainRenderer->ClearHighlight();
         }
      }
   }

   return cTerrainTool::OnMouseMove(mouseEvent, pView);
}

////////////////////////////////////////

tResult cTerrainElevationTool::GetToolTip(const cEditorMouseEvent & mouseEvent,
                                          cStr * pToolTipText, uint_ptr * pToolTipId) const
{
   if (pToolTipText == NULL || pToolTipId == NULL)
   {
      return E_POINTER;
   }

   HTERRAINVERTEX hVert = INVALID_HTERRAINVERTEX;
   if (GetHitVertex(mouseEvent.GetPoint(), &hVert))
   {
      tVec3 pos;
      UseGlobal(TerrainModel);
      if (pTerrainModel->GetVertexPosition(hVert, &pos) == S_OK)
      {
         Sprintf(pToolTipText, "Vertex: <%.2f, %.2f, %.2f>", pos.x, pos.y, pos.z);
         *pToolTipId = reinterpret_cast<uint_ptr>(hVert);
         return S_OK;
      }
   }

   return S_FALSE;
}

////////////////////////////////////////

tResult cTerrainElevationTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   // Should have hit-tested for a vertex in OnMouseMove
   if (m_hHitVertex != INVALID_HTERRAINVERTEX)
   {
      UseGlobal(TerrainRenderer);
      pTerrainRenderer->EnableBlending(false);

      m_lastDragPoint = mouseEvent.GetPoint();
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cTerrainElevationTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   // Do what is done on a move
   tResult result = OnDragMove(mouseEvent, pView);

   UseGlobal(TerrainRenderer);
   pTerrainRenderer->EnableBlending(true);

   if (!!m_pCommand)
   {
      UseGlobal(EditorApp);
      cAutoIPtr<IEditorModel> pEditorModel;
      if (pEditorApp->GetActiveModel(&pEditorModel) == S_OK)
      {
         pEditorModel->AddCommand(m_pCommand, false);
      }
      SafeRelease(m_pCommand);
   }

   return result;
}

////////////////////////////////////////

tResult cTerrainElevationTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (m_hHitVertex != INVALID_HTERRAINVERTEX)
   {
      CPoint delta = mouseEvent.GetPoint() - m_lastDragPoint;
      m_lastDragPoint = mouseEvent.GetPoint();

      m_elevDelta -= static_cast<float>(delta.y);

      if (!!m_pCommand)
      {
         Verify(SUCCEEDED(m_pCommand->Undo()));
         SafeRelease(m_pCommand);
      }

      m_pCommand = static_cast<IEditorCommand*>(
         new cTerrainChangeElevationCommand(m_hHitVertex, m_elevDelta));

      if (!m_pCommand)
      {
         return E_OUTOFMEMORY;
      }

      if (m_pCommand->Do() != S_OK)
      {
         return E_FAIL;
      }
   }

   return S_EDITOR_TOOL_HANDLED;
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTerrainPlateauTool
//

////////////////////////////////////////

cTerrainPlateauTool::cTerrainPlateauTool()
 : m_elevation(0)
{
}

////////////////////////////////////////

cTerrainPlateauTool::~cTerrainPlateauTool()
{
}

////////////////////////////////////////

tResult cTerrainPlateauTool::OnMouseMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   if (pView != NULL)
   {
      HTERRAINVERTEX hHitVertex;
      if (GetHitVertex(mouseEvent.GetPoint(), &hHitVertex))
      {
         UseGlobal(TerrainRenderer);
         pTerrainRenderer->HighlightTerrainVertex(hHitVertex);
      }
      else
      {
         UseGlobal(TerrainRenderer);
         pTerrainRenderer->ClearHighlight();
      }
   }

   return cTerrainTool::OnMouseMove(mouseEvent, pView);
}

////////////////////////////////////////

tResult cTerrainPlateauTool::OnDragStart(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   m_hitVertices.clear();

   HTERRAINVERTEX hHitVertex;
   if (GetHitVertex(mouseEvent.GetPoint(), &hHitVertex))
   {
      tVec3 vertexPos;

      UseGlobal(TerrainModel);
      if (pTerrainModel->GetVertexPosition(hHitVertex, &vertexPos) != S_OK)
      {
         return E_FAIL;
      }

      m_elevation = vertexPos.y;

      Assert(!m_pCommand);
      if (EditorCompositeCommandCreate(TerrainTileCompositeCommandCB, &m_pCommand) != S_OK)
      {
         ErrorMsg("Error creating composite command\n");
         return E_FAIL;
      }
   }

   return S_EDITOR_TOOL_CONTINUE;
}

////////////////////////////////////////

tResult cTerrainPlateauTool::OnDragEnd(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   // Do what is done on a move
   tResult result = OnDragMove(mouseEvent, pView);

   UseGlobal(TerrainRenderer);
   pTerrainRenderer->EnableBlending(true);

   Assert(!!m_pCommand);
   UseGlobal(EditorApp);
   cAutoIPtr<IEditorModel> pEditorModel;
   if (pEditorApp->GetActiveModel(&pEditorModel) == S_OK)
   {
      pEditorModel->AddCommand(m_pCommand, false);
   }
   SafeRelease(m_pCommand);

   m_hitVertices.clear();

   return result;
}

////////////////////////////////////////

tResult cTerrainPlateauTool::OnDragMove(const cEditorMouseEvent & mouseEvent, IEditorView * pView)
{
   HTERRAINVERTEX hHitVertex;
   if (GetHitVertex(mouseEvent.GetPoint(), &hHitVertex)
      && m_hitVertices.find(hHitVertex) == m_hitVertices.end())
   {
      m_hitVertices.insert(hHitVertex);

      cAutoIPtr<IEditorCommand> pCommand(static_cast<IEditorCommand*>(
         new cTerrainSetElevationCommand(hHitVertex, m_elevation, IDS_TERRAIN_PLATEAU_COMMAND_LABEL)));

      if (!pCommand)
      {
         return E_OUTOFMEMORY;
      }

      if (pCommand->Do() != S_OK)
      {
         return E_FAIL;
      }

      m_pCommand->Add(pCommand);
   }

   return S_EDITOR_TOOL_HANDLED;
}


/////////////////////////////////////////////////////////////////////////////
