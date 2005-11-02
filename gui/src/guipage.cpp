///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guipage.h"
#include "guielementtools.h"

#include <queue>

#include "dbgalloc.h" // must be last header

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIPage
//

///////////////////////////////////////

cGUIPage::cGUIPage(const tGUIElementList * pElements)
 : m_bUpdateLayout(pElements != NULL)
{
   if (pElements != NULL)
   {
      m_elements.resize(pElements->size());
      std::copy(pElements->begin(), pElements->end(), m_elements.begin());
      std::for_each(m_elements.begin(), m_elements.end(), CTInterfaceMethod(&IGUIElement::AddRef));
   }
}

///////////////////////////////////////

cGUIPage::~cGUIPage()
{
   Clear();
}

///////////////////////////////////////

void cGUIPage::Clear()
{
   std::for_each(m_elements.begin(), m_elements.end(), CTInterfaceMethod(&IGUIElement::Release));
   m_elements.clear();
}

///////////////////////////////////////

tResult cGUIPage::AddElement(IGUIElement * pElement)
{
   if (pElement == NULL)
   {
      return E_POINTER;
   }

   if (HasElement(pElement) == S_OK)
   {
      return S_FALSE;
   }

   m_elements.push_back(CTAddRef(pElement));
   m_bUpdateLayout = true;
   return S_OK;
}

///////////////////////////////////////

tResult cGUIPage::RemoveElement(IGUIElement * pElement)
{
   if (pElement == NULL)
   {
      return E_POINTER;
   }

   tGUIElementList::iterator f = std::find_if(m_elements.begin(), m_elements.end(), cSameAs(pElement));
   if (f != m_elements.end())
   {
      (*f)->Release();
      m_elements.erase(f);
      m_bUpdateLayout = true;
      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

size_t cGUIPage::CountElements() const
{
   return m_elements.size();
}

///////////////////////////////////////

tResult cGUIPage::HasElement(IGUIElement * pElement) const
{
   if (pElement == NULL)
   {
      return E_POINTER;
   }

   tGUIElementList::const_iterator f = std::find_if(m_elements.begin(), m_elements.end(), cSameAs(pElement));
   return (f != m_elements.end()) ? S_OK : S_FALSE;
}

///////////////////////////////////////

tResult cGUIPage::GetElement(const tChar * pszId, IGUIElement * * ppElement)
{
   if (pszId == NULL || ppElement == NULL)
   {
      return E_POINTER;
   }

   // TODO: construct a map to do this
   // TODO: recursively search all children?
   tGUIElementList::iterator iter;
   for (iter = m_elements.begin(); iter != m_elements.end(); iter++)
   {
      if (GUIElementIdMatch(*iter, pszId))
      {
         *ppElement = CTAddRef(*iter);
         return S_OK;
      }
   }

   return S_FALSE;
}

///////////////////////////////////////

tResult cGUIPage::GetActiveModalDialog(IGUIDialogElement * * ppDialog)
{
   if (ppDialog == NULL)
   {
      return E_POINTER;
   }

   if (m_elements.empty())
   {
      return S_FALSE;
   }

   return m_elements.back()->QueryInterface(IID_IGUIDialogElement, (void**)ppDialog);
}

///////////////////////////////////////

void cGUIPage::UpdateLayout(const tGUIRect & rect)
{
   if (m_bUpdateLayout)
   {
      std::for_each(BeginElements(), EndElements(), cSizeAndPlaceElement(rect));
      m_bUpdateLayout = false;
   }
}

///////////////////////////////////////

void cGUIPage::Render(IGUIRenderDevice * pRenderDevice)
{
   if (pRenderDevice != NULL)
   {
      std::for_each(BeginElements(), EndElements(), cRenderElement(pRenderDevice));
   }
}

///////////////////////////////////////

typedef std::pair<IGUIElement*, uint> tQueueEntry;

bool operator >(const tQueueEntry & lhs, const tQueueEntry & rhs)
{
   return lhs.second > rhs.second;
}

tResult cGUIPage::GetHitElements(const tGUIPoint & point, tGUIElementList * pElements) const
{
   if (pElements == NULL)
   {
      return E_POINTER;
   }

   if (m_elements.empty())
   {
      return S_FALSE;
   }

   std::priority_queue<tQueueEntry, std::vector<tQueueEntry>, std::greater<tQueueEntry> > q;

   tGUIElementList::const_iterator iter = BeginElements();
   for (; iter != EndElements(); iter++)
   {
      uint zorder = 0;
      cAutoIPtr<IGUIDialogElement> pTempDlg;
      if ((*iter)->QueryInterface(IID_IGUIDialogElement, (void**)&pTempDlg) == S_OK)
      {
         zorder = 1000; // modal dialogs should have a higher z order
      }
      q.push(tQueueEntry(CTAddRef(*iter), zorder));
   }

   while (!q.empty())
   {
      cAutoIPtr<IGUIElement> pElement(q.top().first);
      uint zorder = q.top().second;
      q.pop();

      tGUIPoint pos(GUIElementAbsolutePosition(pElement));
      tGUIPoint relative(point - pos);

      if (pElement->Contains(relative))
      {
         cAutoIPtr<IGUIElementEnum> pEnum;
         if (pElement->EnumChildren(&pEnum) == S_OK)
         {
            IGUIElement * pChildren[32];
            ulong count = 0;
            while (SUCCEEDED(pEnum->Next(_countof(pChildren), &pChildren[0], &count)) && (count > 0))
            {
               for (ulong i = 0; i < count; i++)
               {
                  q.push(tQueueEntry(pChildren[i], zorder+1));
               }
               count = 0;
            }
         }

         pElements->push_front(CTAddRef(pElement));
      }
   }

   return pElements->empty() ? S_FALSE : S_OK;
}


///////////////////////////////////////////////////////////////////////////////
