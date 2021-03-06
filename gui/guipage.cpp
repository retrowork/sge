///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guipage.h"
#include "guielementenum.h"
#include "guielementtools.h"
#include "guieventroutertem.h"
#include "guipagelayout.h"
#include "guistrings.h"
#include "gui/guistyleapi.h"

#include "script/scriptapi.h"

#include "tech/comenumutil.h"
#include "tech/globalobj.h"
#include "tech/point2.inl"

#include <tinyxml.h>

#define BOOST_MEM_FN_ENABLE_STDCALL
#include <boost/mem_fn.hpp>

#include <stack>
#include <queue>

#include "tech/dbgalloc.h" // must be last header

using namespace boost;
using namespace std;

///////////////////////////////////////////////////////////////////////////////

static void CreateElements(const TiXmlNode * pTiXmlNode, IGUIElement * pParent,
                           void (*pfnCallback)(IGUIElement *, IGUIElement *, void *),
                           void * pCallbackData)
{
   if (pTiXmlNode == NULL)
   {
      return;
   }

   UseGlobal(GUIFactory);

   cAutoIPtr<IGUIContainerElement> pContainer;
   if (pParent != NULL)
   {
      pParent->QueryInterface(IID_IGUIContainerElement, (void**)&pContainer);
   }

   for (const TiXmlElement * pXmlElement = pTiXmlNode->FirstChildElement();
      pXmlElement != NULL; pXmlElement = pXmlElement->NextSiblingElement())
   {
      if (pXmlElement->Type() != TiXmlNode::ELEMENT)
      {
         continue;
      }

      cAutoIPtr<IGUIElement> pElement;
      if (pGUIFactory->CreateElement(pXmlElement, pParent, &pElement) == S_OK)
      {
         if (pfnCallback != NULL)
         {
            (*pfnCallback)(pElement, pParent, pCallbackData);
         }
         if (!!pContainer && (pContainer->AddElement(pElement) != S_OK))
         {
            WarnMsg("Error creating child element\n");
         }
         CreateElements(pXmlElement, pElement, pfnCallback, pCallbackData);
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIPageCreateFactoryListener
//

class cGUIPageCreateFactoryListener : public cComObject<IMPLEMENTS(IGUIFactoryListener)>
{
public:
   cGUIPageCreateFactoryListener();
   ~cGUIPageCreateFactoryListener();

   virtual tResult PreCreateElement(const TiXmlElement * pXmlElement, IGUIElement * pParent);
   virtual void OnCreateElement(const TiXmlElement * pXmlElement, IGUIElement * pParent, IGUIElement * pElement);

private:
   list<IGUIStyleElement*> m_styleElements;
};

///////////////////////////////////////

cGUIPageCreateFactoryListener::cGUIPageCreateFactoryListener()
{
}

///////////////////////////////////////

cGUIPageCreateFactoryListener::~cGUIPageCreateFactoryListener()
{
   for_each(m_styleElements.begin(), m_styleElements.end(), mem_fn(&IGUIStyleElement::Release));
   m_styleElements.clear();
}

///////////////////////////////////////

tResult cGUIPageCreateFactoryListener::PreCreateElement(const TiXmlElement * pXmlElement,
                                                        IGUIElement * pParent)
{
   return S_OK;
}

///////////////////////////////////////

void cGUIPageCreateFactoryListener::OnCreateElement(const TiXmlElement * pXmlElement,
                                                    IGUIElement * pParent,
                                                    IGUIElement * pElement)
{
   if (pElement == NULL)
   {
      return;
   }

   cAutoIPtr<IGUIStyleElement> pStyle;
   if (pElement->QueryInterface(IID_IGUIStyleElement, (void**)&pStyle) == S_OK)
   {
      m_styleElements.push_back(CTAddRef(pStyle));
   }
   else
   {
      cAutoIPtr<IGUIStyle> pClassStyle, pInlineStyle;

      if (!m_styleElements.empty())
      {
         // TODO: how to handle multiple style sheets on the same page?
         cAutoIPtr<IGUIStyleSheet> pStyleSheet;
         if (m_styleElements.front()->GetStyleSheet(&pStyleSheet) == S_OK)
         {
            pStyleSheet->GetStyle(pXmlElement->Value(),
               pXmlElement->Attribute(kAttribStyleClass), &pClassStyle);
         }
      }

      {
         const char * pszStyleAttrib = pXmlElement->Attribute(kAttribStyle);
         if (pszStyleAttrib != NULL)
         {
            GUIStyleParseInline(pszStyleAttrib, -1, pClassStyle, &pInlineStyle);
         }
      }

      if (!!pInlineStyle)
      {
         pElement->SetStyle(pInlineStyle);
      }
      else
      {
         pElement->SetStyle(pClassStyle);
      }
   }
}


///////////////////////////////////////////////////////////////////////////////

struct sRenderLoopStackElement
{
   sRenderLoopStackElement(IGUIElement * pElement_, IGUIElementRenderer * pRenderer_, tGUIPoint base_)
    : pElement(pElement_)
    , pRenderer(pRenderer_)
    , base(base_)
   {
   }

   IGUIElement * pElement;
   IGUIElementRenderer * pRenderer;
   tGUIPoint base;
};

typedef stack<sRenderLoopStackElement> tRenderLoopStack;

inline void PushChildElement(tRenderLoopStack * pStack,
                             IGUIElement * pChild,
                             IGUIElementRenderer * pRenderer,
                             const tGUIPoint & position)
{
   cAutoIPtr<IGUIElementRenderer> pChildRenderer;
   if (pChild->GetRenderer(&pChildRenderer) == S_OK)
   {
      pRenderer = pChildRenderer;
   }
   pStack->push(sRenderLoopStackElement(CTAddRef(pChild), CTAddRef(pRenderer), position));
}

template <typename ITERATOR, typename FUNCTOR>
void GUIElementRenderLoop(ITERATOR begin, ITERATOR end, FUNCTOR f)
{
   tRenderLoopStack s;

   for (ITERATOR iter = begin; iter != end; ++iter)
   {
      cAutoIPtr<IGUIElement> pElement(CTAddRef(*iter));
      if (pElement->IsVisible())
      {
         cAutoIPtr<IGUIElementRenderer> pRenderer;
         if (pElement->GetRenderer(&pRenderer) == S_OK)
         {
            s.push(sRenderLoopStackElement(CTAddRef(pElement), CTAddRef(pRenderer), tGUIPoint(0,0)));
         }
      }
   }

   while (!s.empty())
   {
      const sRenderLoopStackElement & t = s.top();
      cAutoIPtr<IGUIElement> pElement(t.pElement);
      cAutoIPtr<IGUIElementRenderer> pRenderer(t.pRenderer);
      tGUIPoint position(pElement->GetPosition() + t.base);
      s.pop();

      tResult result = f(pElement, pRenderer, position);
      if (result == S_FALSE)
      {
         continue;
      }
      else if (FAILED(result))
      {
         while (!s.empty())
         {
            SafeRelease(s.top().pElement);
            SafeRelease(s.top().pRenderer);
            s.pop();
         }
         break;
      }

      cAutoIPtr<IGUIContainerElement> pContainer;
      cAutoIPtr<IGUIElementEnum> pEnum;
      if (pElement->QueryInterface(IID_IGUIContainerElement, (void**)&pContainer) == S_OK
         && pContainer->EnumChildren(&pEnum) == S_OK)
      {
         IGUIElement * pChildren[32];
         ulong count = 0;
         while (SUCCEEDED(pEnum->Next(_countof(pChildren), &pChildren[0], &count)) && (count > 0))
         {
            for (ulong i = 0; i < count; i++)
            {
               if (pChildren[i]->IsVisible())
               {
                  PushChildElement(&s, pChildren[i], pRenderer, position);
               }
               SafeRelease(pChildren[i]);
            }
            count = 0;
         }
      }

      cAutoIPtr<IGUIScrollable> pScrollable;
      if (pElement->QueryInterface(IID_IGUIScrollable, (void**)&pScrollable) == S_OK)
      {
         cAutoIPtr<IGUIScrollBarElement> pVertScrollBar;
         if (pScrollable->GetVerticalScrollBar(&pVertScrollBar) == S_OK)
         {
            PushChildElement(&s, pVertScrollBar, pRenderer, position);
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////

template <typename F>
static tResult GetElementHelper(IGUIElement * pParent, F f, IGUIElement * * ppElement)
{
   if (f(pParent))
   {
      *ppElement = CTAddRef(pParent);
      return S_OK;
   }
   else
   {
      cAutoIPtr<IGUIContainerElement> pContainer;
      cAutoIPtr<IGUIElementEnum> pEnum;
      if (pParent->QueryInterface(IID_IGUIContainerElement, (void**)&pContainer) == S_OK
         && pContainer->EnumChildren(&pEnum) == S_OK)
      {
         IGUIElement * pChildren[32];
         ulong count = 0;

         while (SUCCEEDED((pEnum->Next(_countof(pChildren), &pChildren[0], &count))) && (count > 0))
         {
            for (ulong i = 0; i < count; i++)
            {
               if (GetElementHelper(pChildren[i], f, ppElement) == S_OK)
               {
                  for (; i < count; i++)
                  {
                     SafeRelease(pChildren[i]);
                  }
                  return S_OK;
               }

               SafeRelease(pChildren[i]);
            }

            count = 0;
         }
      }
   }

   return S_FALSE;
}

class cIdMatch
{
public:
   cIdMatch(const tChar * pszId) : m_id(pszId ? pszId : "") {}
   bool operator()(IGUIElement * pElement) { return GUIElementIdMatch(pElement, m_id.c_str()); }
private:
   cStr m_id;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIPage
//

///////////////////////////////////////

cGUIPage::cGUIPage(const tGUIElementList * pElements, cGUINotifyListeners * pNotifyListeners)
 : m_pNotifyListeners(pNotifyListeners)
{
   if (pElements != NULL)
   {
      m_elements.resize(pElements->size());
      copy(pElements->begin(), pElements->end(), m_elements.begin());
      for_each(m_elements.begin(), m_elements.end(), mem_fn(&IGUIElement::AddRef));

      RequestLayout(NULL, kGUILayoutDefault);
   }
}

///////////////////////////////////////

cGUIPage::~cGUIPage()
{
   ClearElements();
   ClearLayoutRequests();
}

///////////////////////////////////////

static void GUIPageCreateElementsCallback(IGUIElement * pElement, IGUIElement * pParent, void * pData)
{
   if (pData == NULL)
   {
      return;
   }

   if (pElement != NULL)
   {
      // Add only top-level elements to the list
      if (pParent == NULL)
      {
         tGUIElementList * pElements = (tGUIElementList*)pData;
         pElements->push_back(CTAddRef(pElement));
      }
   }
}

///////////////////////////////////////

tResult cGUIPage::Create(const TiXmlDocument * pXmlDoc, cGUINotifyListeners * pNotifyListeners, cGUIPage * * ppPage)
{
   if (pXmlDoc == NULL || ppPage == NULL)
   {
      return E_POINTER;
   }

   const TiXmlNode * pTopLevel = pXmlDoc->FirstChildElement();

   if (pTopLevel == NULL)
   {
      return E_FAIL;
   }

   if (strcmp(pTopLevel->Value(), kElementPage) != 0)
   {
      ErrorMsg("Top-level element should be a <page>\n");
      return E_FAIL;
   }

   cAutoIPtr<IGUIFactoryListener> pFL(static_cast<IGUIFactoryListener*>(new cGUIPageCreateFactoryListener));
   if (!pFL)
   {
      return E_OUTOFMEMORY;
   }

   cGUIPage * pPage = new cGUIPage(NULL, pNotifyListeners);
   if (pPage == NULL)
   {
      return E_OUTOFMEMORY;
   }

   UseGlobal(GUIFactory);
   pGUIFactory->AddFactoryListener(pFL);

   CreateElements(pTopLevel, NULL, GUIPageCreateElementsCallback, &pPage->m_elements);

   pGUIFactory->RemoveFactoryListener(pFL);

   if (pPage->m_elements.empty())
   {
      delete pPage;
      return E_FAIL;
   }

   pPage->RequestLayout(NULL, kGUILayoutDefault);

   *ppPage = pPage;
   return S_OK;
}

///////////////////////////////////////

void cGUIPage::Activate()
{
   RunScripts();
}

///////////////////////////////////////

void cGUIPage::Deactivate()
{
}

///////////////////////////////////////

bool cGUIPage::IsModalDialogPage() const
{
   if (!m_elements.empty())
   {
      uint nDialogs = 0;

      tGUIElementList::const_iterator iter = m_elements.begin();
      for (; iter != m_elements.end(); iter++)
      {
         cAutoIPtr<IGUIDialogElement> pDlg;
         if ((*iter)->QueryInterface(IID_IGUIDialogElement, (void**)&pDlg) == S_OK)
         {
            nDialogs++;
            cAutoIPtr<IGUIElement> pOk, pCancel;
            if (GetElementHelper(pDlg, cIdMatch("ok"), &pOk) != S_OK
               || GetElementHelper(pDlg, cIdMatch("cancel"), &pCancel) != S_OK)
            {
               WarnMsg("Dialog box has no \"ok\" nor \"cancel\" button\n");
            }
         }
      }

      WarnMsgIf(nDialogs > 1, "Multiple dialogs defined in one XML file\n");

      return (nDialogs > 0);
   }

   return false;
}

///////////////////////////////////////

static tResult GUIGetElement(const tGUIElementList & elements, const tChar * pszId, IGUIElement * * ppElement)
{
   if (pszId == NULL || ppElement == NULL)
   {
      return E_POINTER;
   }
   tGUIElementList::const_iterator iter = elements.begin(), end = elements.end();
   for (; iter != end; ++iter)
   {
      if (GetElementHelper(*iter, cIdMatch(pszId), ppElement) == S_OK)
      {
         return S_OK;
      }
   }
   return S_FALSE;
}

tResult cGUIPage::GetElement(const tChar * pszId, IGUIElement * * ppElement)
{
   return GUIGetElement(m_elements, pszId, ppElement);
}

///////////////////////////////////////

void cGUIPage::RequestLayout(IGUIElement * pRequester, uint options)
{
   // TODO: Eliminate redundant requests
   m_layoutRequests.push_back(make_pair(CTAddRef(pRequester), options));
}

///////////////////////////////////////

void cGUIPage::UpdateLayout(const tGUIRect & rect)
{
   static const int kUpdateLayoutInsanity = 16;
   int nLayoutUpdates = 0;
   while (!m_layoutRequests.empty() && (nLayoutUpdates < kUpdateLayoutInsanity))
   {
      cAutoIPtr<IGUIElement> pRequester(m_layoutRequests.front().first);
      uint options = m_layoutRequests.front().second;
      m_layoutRequests.pop_front();

      cGUIPageLayout pageLayout(rect, pRequester, options);
      GUIElementRenderLoop(m_elements.rbegin(), m_elements.rend(), pageLayout);

      ++nLayoutUpdates;
   }
}

///////////////////////////////////////

template <typename T, typename RETURN, typename ARG1, typename ARG2, typename ARG3>
class cMemberFunctor
{
protected:
   typedef RETURN (T::*tMethod)(ARG1, ARG2, ARG3);

public:
   cMemberFunctor(T * pT, tMethod pMethod)
      : m_pT(pT), m_pMethod(pMethod) {}

   RETURN operator ()(ARG1 arg1, ARG2 arg2, ARG3 arg3)
   {
      Assert(m_pT != NULL);
      Assert(m_pMethod != NULL);
      return (m_pT->*m_pMethod)(arg1, arg2, arg3);
   }

protected:
   T * m_pT;
   tMethod m_pMethod;
};

tResult cGUIPage::RenderElement(IGUIElement * pElement, IGUIElementRenderer * pRenderer, const tGUIPoint & position)
{
   uint state = kGUIElementRenderStateDefault;

   if (CTIsSameObject(AccessArmed(), pElement))
   {
      state |= kGUIElementRenderStateArmed;
   }

   if (CTIsSameObject(AccessFocus(), pElement))
   {
      state |= kGUIElementRenderStateFocus;
   }

   if (CTIsSameObject(AccessMouseOver(), pElement))
   {
      state |= kGUIElementRenderStateMouseOver;
   }

   if (FAILED(pRenderer->Render(pElement, position, state)))
   {
      ErrorMsg1("A GUI element of type \"%s\" failed to render\n", GUIElementType(pElement).c_str());
   }
   return S_OK;
}

void cGUIPage::Render()
{
   GUIElementRenderLoop(m_elements.rbegin(), m_elements.rend(),
      cMemberFunctor<cGUIPage, tResult, IGUIElement *, IGUIElementRenderer *, const tGUIPoint &>(this, &cGUIPage::RenderElement));
}

///////////////////////////////////////

bool cGUIPage::NotifyListeners(IGUIEvent * pEvent)
{
   if (m_pNotifyListeners != NULL)
   {
      return m_pNotifyListeners->NotifyListeners(pEvent);
   }
   else
   {
      return false;
   }
}

///////////////////////////////////////

tResult cGUIPage::GetHitElement(const tScreenPoint & point, IGUIElement * * ppElement) const
{
   if (ppElement == NULL)
   {
      return E_POINTER;
   }

   list<IGUIElement*> hitElements;
   if (GetHitElements(point, &hitElements) == S_OK)
   {
      Assert(!hitElements.empty());
      *ppElement = CTAddRef(hitElements.front());
      for_each(hitElements.begin(), hitElements.end(), mem_fn(&IGUIElement::Release));
      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

class cCollectHitElements
{
public:
   cCollectHitElements(const tScreenPoint & point, tGUIElementList * pHitElements)
    : m_point(static_cast<float>(point.x), static_cast<float>(point.y))
    , m_pHitElements(pHitElements)
   {
   }

   tResult operator()(IGUIElement * pElement, IGUIElementRenderer * pRenderer, const tGUIPoint & position)
   {
      const tGUISize & size = pElement->GetSize();

      tRectf rect(position.x, position.y, position.x + size.width, position.y + size.height);

      if (rect.PtInside(m_point.x, m_point.y))
      {
         m_pHitElements->push_front(CTAddRef(pElement));
         return S_OK;
      }

      return S_FALSE;
   }

private:
   const tGUIPoint m_point;
   tGUIElementList * m_pHitElements;
};

tResult cGUIPage::GetHitElements(const tScreenPoint & point, tGUIElementList * pElements) const
{
   if (pElements == NULL)
   {
      return E_POINTER;
   }

   if (m_elements.empty())
   {
      return S_FALSE;
   }

   cCollectHitElements collectHitElements(point, pElements);
   GUIElementRenderLoop(m_elements.begin(), m_elements.end(), collectHitElements);

   return pElements->empty() ? S_FALSE : S_OK;
}

///////////////////////////////////////

void cGUIPage::ClearElements()
{
   for_each(m_elements.begin(), m_elements.end(), mem_fn(&IGUIElement::Release));
   m_elements.clear();
}

///////////////////////////////////////

void cGUIPage::ClearLayoutRequests()
{
   tLayoutRequests::iterator iter = m_layoutRequests.begin();
   for (; iter != m_layoutRequests.end(); iter++)
   {
      SafeRelease(iter->first);
   }
   m_layoutRequests.clear();
}

///////////////////////////////////////

static tResult RunScriptHelper(IGUIElement * pElement)
{
   if (pElement == NULL)
   {
      return E_POINTER;
   }
   cAutoIPtr<IGUIScriptElement> pScript;
   if (pElement->QueryInterface(IID_IGUIScriptElement, (void**)&pScript) == S_OK)
   {
      tGUIString script;
      if (pScript->GetScript(&script) == S_OK)
      {
         UseGlobal(ScriptInterpreter);
         if (pScriptInterpreter->ExecString(script.c_str()) != S_OK)
         {
            WarnMsg("An error occured running script element\n");
         }
         else
         {
            return S_OK;
         }
      }
   }
   else
   {
      cAutoIPtr<IGUIContainerElement> pContainer;
      if (pElement->QueryInterface(IID_IGUIContainerElement, (void**)&pContainer) == S_OK)
      {
         cAutoIPtr<IGUIElementEnum> pEnum;
         if (pContainer->EnumChildren(&pEnum) == S_OK)
         {
            ForEach<IGUIElementEnum, IGUIElement>(pEnum, RunScriptHelper);
         }
      }
   }
   return S_FALSE;
}

void cGUIPage::RunScripts()
{
   for_each(BeginElements(), EndElements(), RunScriptHelper);
}


///////////////////////////////////////////////////////////////////////////////
