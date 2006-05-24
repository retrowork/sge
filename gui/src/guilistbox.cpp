///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guilistbox.h"
#include "guielementbasetem.h"
#include "guielementtools.h"
#include "guistrings.h"

#include "globalobj.h"
#include "multivar.h"

#include <algorithm>
#include <list>
#include <tinyxml.h>

#include "dbgalloc.h" // must be last header


static const uint kInsaneRowCount = 1000;
static const uint kInsaneItemHeight = 500;

extern tResult GUIScrollBarElementCreate(eGUIScrollBarType scrollBarType,
                                         IGUIScrollBarElement * * ppScrollBar);

typedef std::list<IGUIElement *> tGUIElementList;
extern tResult GUIElementEnumCreate(const tGUIElementList & elements,
                                    IGUIElementEnum * * ppEnum);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIListBoxItem
//

////////////////////////////////////////

cGUIListBoxItem::cGUIListBoxItem(const tGUIChar * pszText, uint_ptr data, bool bSelected)
 : m_text(pszText)
 , m_data(data)
 , m_bSelected(bSelected)
{
}

////////////////////////////////////////

cGUIListBoxItem::cGUIListBoxItem(const cGUIListBoxItem & other)
 : m_text(other.m_text)
 , m_data(other.m_data)
 , m_bSelected(other.m_bSelected)
{
}

////////////////////////////////////////

const cGUIListBoxItem & cGUIListBoxItem::operator =(const cGUIListBoxItem & other)
{
   m_text = other.m_text;
   m_data = other.m_data;
   m_bSelected = other.m_bSelected;
   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIListBoxElement
//

////////////////////////////////////////

cGUIListBoxElement::cGUIListBoxElement()
 : m_rowCount(1)
 , m_itemHeight(0)
{
}

////////////////////////////////////////

cGUIListBoxElement::~cGUIListBoxElement()
{
}

///////////////////////////////////////

tResult cGUIListBoxElement::EnumChildren(IGUIElementEnum * * ppChildren)
{
   if (ppChildren == NULL)
   {
      return E_POINTER;
   }
   if (!m_pVScrollBar)
   {
      return S_FALSE;
   }
   tGUIElementList children;
   children.push_back(m_pVScrollBar);
   return GUIElementEnumCreate(children, ppChildren);
}

///////////////////////////////////////

tResult cGUIListBoxElement::SetClientArea(const tGUIRect & clientArea)
{
   tGUIRect modifiedClientArea(clientArea);

   //cAutoIPtr<IGUIElementRenderer> pRenderer;
   //if (GetRenderer(&pRenderer) == S_OK)
   //{
   //   tGUISize vScrollBarSize(0,0);

   //   if (!!m_pVScrollBar)
   //   {
   //      pRenderer->GetPreferredSize(m_pVScrollBar, &vScrollBarSize);
   //   }

   //   if (!!m_pVScrollBar)
   //   {
   //      vScrollBarSize.height = static_cast<tGUISizeType>(clientArea.GetHeight());
   //      m_pVScrollBar->SetPosition(tGUIPoint(clientArea.right - vScrollBarSize.width, static_cast<float>(clientArea.top)));
   //      m_pVScrollBar->SetSize(vScrollBarSize);
   //      modifiedClientArea.right -= FloatToInt(vScrollBarSize.width);
   //   }
   //}

   tResult result = cGUIElementBase<IGUIListBoxElement>::SetClientArea(modifiedClientArea);
   UpdateScrollInfo();
   return result;
}

///////////////////////////////////////

tResult cGUIListBoxElement::ComputeClientArea(IGUIElementRenderer * pRenderer, tGUIRect * pClientArea)
{
   if (pRenderer == NULL || pClientArea == NULL)
   {
      return E_POINTER;
   }

   if (!!m_pVScrollBar && m_pVScrollBar->IsVisible())
   {
      tGUISize scrollBarSize(0,0);
      if (pRenderer->GetPreferredSize(m_pVScrollBar, &scrollBarSize) == S_OK)
      {
         scrollBarSize.height = static_cast<tGUISizeType>(pClientArea->GetHeight());
         m_pVScrollBar->SetPosition(tGUIPoint(pClientArea->right - scrollBarSize.width, static_cast<float>(pClientArea->top)));
         m_pVScrollBar->SetSize(scrollBarSize);
         pClientArea->right -= FloatToInt(scrollBarSize.width);
         return S_OK;
      }
   }

   return S_FALSE;
}

///////////////////////////////////////

tResult cGUIListBoxElement::OnEvent(IGUIEvent * pEvent)
{
   Assert(pEvent != NULL);

   tResult result = S_OK; // allow event processing to continue

   tGUIEventCode eventCode;
   Verify(pEvent->GetEventCode(&eventCode) == S_OK);

   if (eventCode == kGUIEventClick)
   {
      tScreenPoint point;
      Verify(pEvent->GetMousePosition(&point) == S_OK);

      tGUIPoint pos = GUIElementAbsolutePosition(this);

      if (m_itemHeight > 0)
      {
         uint index = FloatToInt(point.y - pos.y) / m_itemHeight;
         Select(index, index);
         Verify(pEvent->SetCancelBubble(true) == S_OK);
      }
   }

   return result;
}

////////////////////////////////////////

tResult cGUIListBoxElement::AddItem(const tChar * pszString, uint_ptr extra)
{
   if (pszString == NULL)
   {
      return E_POINTER;
   }
   m_items.push_back(cGUIListBoxItem(pszString, extra));
   UpdateScrollInfo();
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::RemoveItem(uint index)
{
   if (index >= m_items.size())
   {
      return E_INVALIDARG;
   }
   m_items.erase(m_items.begin() + index);
   UpdateScrollInfo();
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetItemCount(uint * pItemCount) const
{
   if (pItemCount == NULL)
   {
      return E_POINTER;
   }
   *pItemCount = m_items.size();
   return *pItemCount > 0 ? S_OK : S_FALSE;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetItem(uint index, cStr * pString,
                                    uint_ptr * pExtra, bool * pbIsSelected) const
{
   if (index >= m_items.size())
   {
      return E_INVALIDARG;
   }

   if (pString == NULL)
   {
      return E_POINTER;
   }

   *pString = m_items[index].GetText();
   if (pExtra != NULL)
   {
      *pExtra = m_items[index].GetData();
   }
   if (pbIsSelected != NULL)
   {
      *pbIsSelected = m_items[index].IsSelected();
   }
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::Sort()
{
   return E_NOTIMPL;
}

////////////////////////////////////////

tResult cGUIListBoxElement::Clear()
{
   m_items.clear();
   UpdateScrollInfo();
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::FindItem(const tChar * pszString, uint * pIndex) const
{
   if (pszString == NULL || pIndex == NULL)
   {
      return E_POINTER;
   }

   tListBoxItems::const_iterator iter = m_items.begin();
   for (uint index = 0; iter != m_items.end(); iter++, index++)
   {
      if (iter->GetText().compare(pszString) == 0)
      {
         *pIndex = index;
         return S_OK;
      }
   }

   return S_FALSE;
}

////////////////////////////////////////

tResult cGUIListBoxElement::Select(uint startIndex, uint endIndex)
{
   if (startIndex >= m_items.size() || startIndex > endIndex)
   {
      return E_INVALIDARG;
   }

   if (!IsMultiSelect())
   {
      if (startIndex != endIndex)
      {
         WarnMsg2("Invalid selection indices for single-select listbox: %d, %d\n", startIndex, endIndex);
         return E_INVALIDARG;
      }

      tListBoxItems::iterator iter = m_items.begin();
      for (uint index = 0; iter != m_items.end(); iter++, index++)
      {
         if (index == startIndex)
         {
            iter->Select();
         }
         else
         {
            iter->Deselect();
         }
      }
   }
   else
   {
      if (endIndex >= m_items.size())
      {
         endIndex = m_items.size() - 1;
      }

      for (uint i = startIndex; i <= endIndex; i++)
      {
         m_items[i].Select();
      }
   }

   tGUIString onSelChange;
   if (GetOnSelChange(&onSelChange) == S_OK)
   {
      UseGlobal(ScriptInterpreter);
      pScriptInterpreter->ExecString(onSelChange.c_str());
   }

   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::SelectAll()
{
   if (!IsMultiSelect())
   {
      return E_FAIL;
   }

#if _MSC_VER <= 1200
   tListBoxItems::iterator iter = m_items.begin();
   for (; iter != m_items.end(); iter++)
   {
      iter->Select();
   }
#else
   std::for_each(m_items.begin(), m_items.end(), std::mem_fun_ref(&cGUIListBoxItem::Select));
#endif

   tGUIString onSelChange;
   if (GetOnSelChange(&onSelChange) == S_OK)
   {
      UseGlobal(ScriptInterpreter);
      pScriptInterpreter->ExecString(onSelChange.c_str());
   }

   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::Deselect(uint startIndex, uint endIndex)
{
   if (startIndex >= m_items.size())
   {
      return E_INVALIDARG;
   }

   if (endIndex >= m_items.size())
   {
      endIndex = m_items.size() - 1;
   }

   for (uint i = startIndex; i <= endIndex; i++)
   {
      m_items[i].Deselect();
   }

   tGUIString onSelChange;
   if (GetOnSelChange(&onSelChange) == S_OK)
   {
      UseGlobal(ScriptInterpreter);
      pScriptInterpreter->ExecString(onSelChange.c_str());
   }

   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::DeselectAll()
{
#if _MSC_VER <= 1200
   tListBoxItems::iterator iter = m_items.begin();
   for (; iter != m_items.end(); iter++)
   {
      iter->Deselect();
   }
#else
   std::for_each(m_items.begin(), m_items.end(), std::mem_fun_ref(&cGUIListBoxItem::Deselect));
#endif

   tGUIString onSelChange;
   if (GetOnSelChange(&onSelChange) == S_OK)
   {
      UseGlobal(ScriptInterpreter);
      pScriptInterpreter->ExecString(onSelChange.c_str());
   }

   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetSelectedCount(uint * pSelectedCount) const
{
   if (pSelectedCount == NULL)
   {
      return E_POINTER;
   }
#if _MSC_VER <= 1200
   *pSelectedCount = 0;
   tListBoxItems::const_iterator iter = m_items.begin();
   for (; iter != m_items.end(); iter++)
   {
      if (iter->IsSelected())
      {
         *pSelectedCount += 1;
      }
   }
#else
   *pSelectedCount = std::count_if(m_items.begin(), m_items.end(),
                                   std::mem_fun_ref(&cGUIListBoxItem::IsSelected));
#endif
   if (!IsMultiSelect())
   {
      Assert(*pSelectedCount == 1);
   }
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetSelected(uint * pIndices, uint nMaxIndices)
{
   if (pIndices == NULL)
   {
      return E_POINTER;
   }
   if (nMaxIndices == 0)
   {
      return E_INVALIDARG;
   }
   uint nSelected = 0;
   tListBoxItems::const_iterator iter = m_items.begin();
   for (uint index = 0; iter != m_items.end(); iter++, index++)
   {
      if (iter->IsSelected())
      {
         if (nSelected < nMaxIndices)
         {
            pIndices[nSelected++] = index;
         }
         else
         {
            break;
         }
      }
   }
   if (!IsMultiSelect())
   {
      Assert(nSelected == 1);
   }
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetRowCount(uint * pRowCount) const
{
   if (pRowCount == NULL)
   {
      return E_POINTER;
   }
   *pRowCount = m_rowCount;
   return *pRowCount > 0 ? S_OK : S_FALSE;
}

////////////////////////////////////////

tResult cGUIListBoxElement::SetRowCount(uint rowCount)
{
   if (rowCount >= kInsaneRowCount)
   {
      WarnMsg1("Insane row count given to listbox element: %d\n", rowCount);
      return E_INVALIDARG;
   }
   m_rowCount = rowCount;
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetScrollBar(eGUIScrollBarType scrollBarType,
                                         IGUIScrollBarElement * * ppScrollBar)
{
   if (ppScrollBar == NULL)
   {
      return E_POINTER;
   }
   if (scrollBarType == kGUIScrollBarVertical)
   {
      return m_pVScrollBar.GetPointer(ppScrollBar);
   }
   else
   {
      return E_INVALIDARG;
   }
   return E_FAIL;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetItemHeight(uint * pItemHeight) const
{
   if (pItemHeight == NULL)
   {
      return E_POINTER;
   }
   *pItemHeight = m_itemHeight;
   return (m_itemHeight != 0) ? S_OK : S_FALSE;
}

////////////////////////////////////////

tResult cGUIListBoxElement::SetItemHeight(uint itemHeight)
{
   if (itemHeight >= kInsaneItemHeight)
   {
      WarnMsg1("Insane item height given to listbox element: %d\n", itemHeight);
      return E_INVALIDARG;
   }
   m_itemHeight = itemHeight;
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::GetOnSelChange(tGUIString * pOnSelChange) const
{
   if (pOnSelChange == NULL)
   {
      return E_POINTER;
   }
   if (m_onSelChange.empty())
   {
      return S_FALSE;
   }
   *pOnSelChange = m_onSelChange;
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::SetOnSelChange(const tGUIChar * pszOnSelChange)
{
   if (pszOnSelChange == NULL)
   {
      return E_POINTER;
   }
   m_onSelChange = pszOnSelChange;
   return S_OK;
}

////////////////////////////////////////

tResult cGUIListBoxElement::Invoke(const char * pszMethodName,
                                   int argc, const tScriptVar * argv,
                                   int nMaxResults, tScriptVar * pResults)
{
   if (pszMethodName == NULL)
   {
      return E_POINTER;
   }

   if (strcmp(pszMethodName, "AddItem") == 0)
   {
      if (argc == 1 && argv[0].IsString())
      {
         if (AddItem(argv[0], 0) == S_OK)
         {
            return S_OK;
         }
      }
      else if (argc == 2 && argv[0].IsString() && argv[1].IsInt())
      {
         if (AddItem(argv[0], argv[1].ToInt()) == S_OK)
         {
            return S_OK;
         }
      }
      else
      {
         return E_INVALIDARG;
      }
   }
   else if (strcmp(pszMethodName, "RemoveItem") == 0)
   {
      if (argc == 1 && IsNumber(argv[0]))
      {
         int index = argv[0].ToInt();
         if (!AlmostEqual(argv[0].ToDouble(), static_cast<double>(index)))
         {
            ErrorMsg1("Real number with fractional component %f passed as index to ListBox::RemoveItem\n", argv[0].ToDouble());
            return E_INVALIDARG;
         }
         if (RemoveItem(index) == S_OK)
         {
            return S_OK;
         }
      }
      else
      {
         return E_INVALIDARG;
      }
   }
   else if (strcmp(pszMethodName, "Clear") == 0)
   {
      if (argc == 0)
      {
         if (Clear() == S_OK)
         {
            return S_OK;
         }
      }
      else
      {
         return E_INVALIDARG;
      }
   }
   else if (strcmp(pszMethodName, "GetSelected") == 0)
   {
      if (argc != 0)
      {
         return E_INVALIDARG;
      }

      uint nSelected = 0;
      if (GetSelectedCount(&nSelected) == S_OK)
      {
         nSelected = Min(nSelected, static_cast<uint>(nMaxResults));
         uint * pIndices = reinterpret_cast<uint *>(alloca(nSelected * sizeof(uint)));
         if (GetSelected(pIndices, nSelected) == S_OK)
         {
            for (uint i = 0; i < nSelected; i++)
            {
               pResults[i].Assign(static_cast<int>(pIndices[i]));
            }
            return nSelected;
         }
      }
   }
   else if (strcmp(pszMethodName, "GetItem") == 0)
   {
      if (argc != 1 || (!argv[0].IsInt() && !argv[0].IsFloat() && !argv[0].IsDouble()))
      {
         return E_INVALIDARG;
      }

      tGUIString itemText;
      uint_ptr itemData = 0;
      if (GetItem(argv[0].ToInt(), &itemText, &itemData, NULL) == S_OK)
      {
         if (nMaxResults >= 2)
         {
            pResults[0] = itemText.c_str();
            pResults[1] = static_cast<double>(itemData);
            return 2;
         }
         else if (nMaxResults >= 1)
         {
            pResults[0] = itemText.c_str();
            return 1;
         }
      }
   }

   return E_FAIL;
}

////////////////////////////////////////

void cGUIListBoxElement::UpdateScrollInfo()
{
   int contentHeight = m_items.size() * m_itemHeight;

   tGUIRect clientArea;
   if (SUCCEEDED(GetClientArea(&clientArea)))
   {
      if (contentHeight > clientArea.GetHeight())
      {
         if (!m_pVScrollBar)
         {
            GUIScrollBarElementCreate(kGUIScrollBarVertical, &m_pVScrollBar);
            if (!!m_pVScrollBar)
            {
               m_pVScrollBar->SetParent(this);
            }
         }
      }
      else
      {
         if (!!m_pVScrollBar)
         {
            m_pVScrollBar->SetParent(NULL);
            SafeRelease(m_pVScrollBar);
         }
      }
   }

   UseGlobal(GUIContext);
   pGUIContext->RequestLayout(this);
}


///////////////////////////////////////////////////////////////////////////////

tResult GUIListBoxElementCreate(const TiXmlElement * pXmlElement,
                                IGUIElement * pParent, IGUIElement * * ppElement)
{
   if (ppElement == NULL)
   {
      return E_POINTER;
   }

   if (pXmlElement != NULL)
   {
      if (strcmp(pXmlElement->Value(), kElementListBox) == 0)
      {
         cAutoIPtr<IGUIListBoxElement> pListBox = static_cast<IGUIListBoxElement *>(
             new cGUIListBoxElement);
         if (!pListBox)
         {
            return E_OUTOFMEMORY;
         }

         int rows;
         if (pXmlElement->QueryIntAttribute(kAttribRows, &rows) == TIXML_SUCCESS)
         {
            pListBox->SetRowCount(rows);
         }

         const tGUIChar * pszOnSelChange = pXmlElement->Attribute(kAttribOnSelChange);
         if (pszOnSelChange != NULL)
         {
            pListBox->SetOnSelChange(pszOnSelChange);
         }

         for (const TiXmlElement * pXmlChild = pXmlElement->FirstChildElement(); 
            pXmlChild != NULL; pXmlChild = pXmlChild->NextSiblingElement())
         {
            if (pXmlChild->Type() == TiXmlNode::ELEMENT
               && _stricmp(pXmlChild->Value(), "item") == 0)
            {
               const char * pszText = pXmlChild->Attribute(kAttribText);
               if (pszText != NULL)
               {
                  pListBox->AddItem(pszText, 0);
               }
            }
         }

         *ppElement = CTAddRef(pListBox);
         return S_OK;
      }
   }
   else
   {
      *ppElement = static_cast<IGUIListBoxElement *>(new cGUIListBoxElement);
      return (*ppElement != NULL) ? S_OK : E_OUTOFMEMORY;
   }

   return E_FAIL;
}

AUTOREGISTER_GUIELEMENTFACTORYFN(listbox, GUIListBoxElementCreate);

///////////////////////////////////////////////////////////////////////////////
