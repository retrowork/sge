/////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "ToolPaletteBar.h"

#include "editorCtrlBars.h"

#include "globalobj.h"

#include "resource.h"       // main symbols

#include "dbgalloc.h" // must be last header

static const uint kButtonSize = 32;
static const CRect buttonMargins(5,5,5,5);

/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cButtonPanel
//

////////////////////////////////////////

cButtonPanel::cButtonPanel()
 : m_margins(buttonMargins),
   m_pTool(new cTerrainTileTool)
{
}

////////////////////////////////////////

cButtonPanel::~cButtonPanel()
{
   Assert(m_buttons.empty());
}

////////////////////////////////////////

void cButtonPanel::AddButton(CButton * pButton)
{
   Assert(pButton != NULL);
   m_buttons.push_back(pButton);
}

////////////////////////////////////////

void cButtonPanel::Clear()
{
   tButtons::iterator iter;
   for (iter = m_buttons.begin(); iter != m_buttons.end(); iter++)
   {
      (*iter)->DestroyWindow();
      delete *iter;
   }
   m_buttons.clear();
}

////////////////////////////////////////

void cButtonPanel::Reposition(LPCRECT pRect, BOOL bRepaint)
{
   CRect buttonRect;
   buttonRect.left = pRect->left + m_margins.left;
   buttonRect.top = pRect->top + m_margins.top;
   buttonRect.right = buttonRect.left + kButtonSize;
   buttonRect.bottom = buttonRect.top + kButtonSize;

   tButtons::iterator iter;
   for (iter = m_buttons.begin(); iter != m_buttons.end(); iter++)
   {
      if (((*iter) != NULL) && IsWindow((*iter)->m_hWnd))
      {
         (*iter)->MoveWindow(buttonRect, bRepaint);

         buttonRect.OffsetRect(buttonRect.Width(), 0);

         if (buttonRect.left > (pRect->right - m_margins.right)
            || buttonRect.right > (pRect->right - m_margins.right))
         {
            buttonRect.left = pRect->left + m_margins.left;
            buttonRect.top += kButtonSize;;
            buttonRect.right = buttonRect.left + kButtonSize;
            buttonRect.bottom = buttonRect.top + kButtonSize;
         }
      }
   }
}

////////////////////////////////////////

void cButtonPanel::HandleClick(uint buttonId)
{
   Assert(!!m_pTool);

   UseGlobal(EditorApp);
   pEditorApp->SetActiveTool(m_pTool);

   m_pTool->SetTile(buttonId - m_buttons[0]->GetDlgCtrlID());

   CButton * pClickedButton = NULL;

   tButtons::iterator iter;
   for (iter = m_buttons.begin(); iter != m_buttons.end(); iter++)
   {
      if (((*iter) != NULL) && IsWindow((*iter)->m_hWnd) && (*iter)->GetDlgCtrlID() != buttonId)
      {
         (*iter)->SendMessage(BM_SETSTATE, FALSE);
      }

      if ((*iter)->GetDlgCtrlID() == buttonId)
      {
         pClickedButton = *iter;
      }
   }

   Assert(pClickedButton != NULL);
   pClickedButton->SendMessage(BM_SETSTATE, TRUE);
}

////////////////////////////////////////

void cButtonPanel::SetMargins(LPCRECT pMargins)
{
   if (pMargins != NULL)
   {
      m_margins = *pMargins;
   }
}


/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cToolPaletteBar
//

////////////////////////////////////////

AUTO_REGISTER_CONTROLBAR(IDS_TOOL_PALETTE_BAR_TITLE, cToolPaletteBar::Factory, kCBP_Right);

////////////////////////////////////////

tResult cToolPaletteBar::Factory(cDockingWindow * * ppDockingWindow)
{
   if (ppDockingWindow == NULL)
   {
      return E_POINTER;
   }
   cToolPaletteBar * pToolPaletteBar = new cToolPaletteBar;
   if (pToolPaletteBar == NULL)
   {
      return E_OUTOFMEMORY;
   }
   *ppDockingWindow = static_cast<cDockingWindow *>(pToolPaletteBar);
   return S_OK;
}

////////////////////////////////////////

cToolPaletteBar::cToolPaletteBar()
{
}

////////////////////////////////////////

cToolPaletteBar::~cToolPaletteBar()
{
}

////////////////////////////////////////

void cToolPaletteBar::OnDefaultTileSetChange(IEditorTileSet * pTileSet)
{
   ClearButtons();

   if (pTileSet != NULL)
   {
      uint nTiles = 0;
      if (pTileSet->GetTileCount(&nTiles) == S_OK)
      {
         for (uint i = 0; i < nTiles; i++)
         {
            cAutoIPtr<IEditorTile> pTile;
            if (pTileSet->GetTile(i, &pTile) == S_OK)
            {
               cStr tileName;
               HBITMAP hBitmap = NULL;
               if (pTile->GetName(&tileName) == S_OK
                  && pTile->GetBitmap(kButtonSize, false, &hBitmap) == S_OK)
               {
                  CButton * pButton = new CButton();
                  if (pButton != NULL)
                  {
                     if (pButton->Create(m_hWnd, rcDefault, "", WS_CHILD | WS_VISIBLE | BS_BITMAP, 0, kButtonIdFirst + i, this))
                     {
                        pButton->SendMessage(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);

                        m_tooltip.AddTool(pButton->m_hWnd, tileName.c_str());

                        m_buttonPanel.AddButton(pButton);
                     }
                     else
                     {
                        delete pButton;
                     }
                  }
               }
            }
         }
      }

      RepositionButtons();
   }
}

////////////////////////////////////////

void cToolPaletteBar::ClearButtons()
{
   m_buttonPanel.Clear();
}

////////////////////////////////////////

void cToolPaletteBar::RepositionButtons(BOOL bRepaint)
{
   CRect rect;
   GetClientRect(rect);
   m_buttonPanel.Reposition(rect, bRepaint);
}

////////////////////////////////////////

LRESULT cToolPaletteBar::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
   if (!m_tooltip.Create(m_hWnd))
   {
      ErrorMsg1("Unable to create tooltip control (error %d)\n", GetLastError());
      return -1;
   }

   UseGlobal(EditorTileManager);
   pEditorTileManager->Connect(this);

   CMessageLoop * pMessageLoop = _Module.GetMessageLoop();
   pMessageLoop->AddMessageFilter(this);

   return 0;
}

////////////////////////////////////////

LRESULT cToolPaletteBar::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
   CMessageLoop * pMessageLoop = _Module.GetMessageLoop();
   pMessageLoop->RemoveMessageFilter(this);

   UseGlobal(EditorTileManager);
   pEditorTileManager->Disconnect(this);
   ClearButtons();
   return 0;
}

////////////////////////////////////////

LRESULT cToolPaletteBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
   RepositionButtons();
   return 0;
}

////////////////////////////////////////

LRESULT cToolPaletteBar::OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled)
{
   if (wNotifyCode == BN_CLICKED)
   {
      m_buttonPanel.HandleClick(wID);
   }
   return 0;
}

////////////////////////////////////////

BOOL cToolPaletteBar::PreTranslateMessage(MSG* pMsg) 
{
   if (m_tooltip.IsWindow())
   {
      if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)
      {
         m_tooltip.RelayEvent(pMsg);
      }
   }

   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
