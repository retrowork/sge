///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guicontext.h"
#include "guievent.h"
#include "guielementtools.h"
#include "guieventroutertem.h"

#include "keys.h"
#include "resourceapi.h"
#include "readwriteapi.h"

#include <tinyxml.h>
#include <GL/glew.h>

#ifdef HAVE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>
#endif

#include "dbgalloc.h" // must be last header

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIContext
//

///////////////////////////////////////

BEGIN_CONSTRAINTS()
   AFTER_GUID(IID_IInput)
END_CONSTRAINTS()

///////////////////////////////////////

cGUIContext::cGUIContext()
 : tBaseClass("GUIContext", CONSTRAINTS()),
   m_inputListener(this),
   m_bNeedLayout(false)
#ifdef GUI_DEBUG
   , m_bShowDebugInfo(false)
   , m_debugInfoPlacement(0,0)
   , m_debugInfoTextColor(tGUIColor::White)
   , m_lastMousePos(0,0)
#endif
{
}

///////////////////////////////////////

cGUIContext::~cGUIContext()
{
}

///////////////////////////////////////

tResult cGUIContext::Init()
{
   UseGlobal(Input);
   pInput->SetGUIInputListener(&m_inputListener);
   GUILayoutManagerRegisterBuiltInTypes();
   return S_OK;
}

///////////////////////////////////////

tResult cGUIContext::Term()
{
   UseGlobal(Input);
   pInput->SetGUIInputListener(NULL);
   RemoveAllElements();
   return S_OK;
}

///////////////////////////////////////

tResult cGUIContext::AddElement(IGUIElement * pElement)
{
#ifndef NDEBUG
   {
      // Check that all sink interface pointers are valid
      tSinksIterator iter = BeginSinks();
      tSinksIterator end = EndSinks();
      for (; iter != end; iter++)
      {
         (*iter)->AddRef();
         (*iter)->Release();
      }
   }
#endif
   return tBaseClass::AddElement(pElement);
}

///////////////////////////////////////

tResult cGUIContext::RemoveElement(IGUIElement * pElement)
{
   tResult result = tBaseClass::RemoveElement(pElement);
#ifndef NDEBUG
   {
      // Check that all sink interface pointers are valid
      tSinksIterator iter = BeginSinks();
      tSinksIterator end = EndSinks();
      for (; iter != end; iter++)
      {
         (*iter)->AddRef();
         (*iter)->Release();
      }
   }
#endif
   return result;
}

///////////////////////////////////////

tResult cGUIContext::HasElement(IGUIElement * pElement) const
{
   return tBaseClass::HasElement(pElement);
}

///////////////////////////////////////

tResult cGUIContext::LoadFromResource(const char * psz)
{
   tResKey rk(psz, kRC_TiXml);
   TiXmlDocument * pTiXmlDoc = NULL;
   UseGlobal(ResourceManager);
   if (pResourceManager->Load(rk, (void**)&pTiXmlDoc) == S_OK)
   {
      uint nCreated = LoadFromTiXmlDoc(pTiXmlDoc);
      pResourceManager->Unload(rk);
      return (nCreated > 0) ? S_OK : S_FALSE;
   }
   return E_FAIL;
}

///////////////////////////////////////

tResult cGUIContext::LoadFromString(const char * psz)
{
   TiXmlDocument doc;
   doc.Parse(psz);

   if (doc.Error())
   {
      DebugMsg1("TiXml parse error: %s\n", doc.ErrorDesc());
      return E_FAIL;
   }

   uint nCreated = LoadFromTiXmlDoc(&doc);
   return (nCreated > 0) ? S_OK : S_FALSE;
}

///////////////////////////////////////

uint cGUIContext::LoadFromTiXmlDoc(TiXmlDocument * pTiXmlDoc)
{
   Assert(pTiXmlDoc != NULL);

   UseGlobal(GUIFactory);

   uint nElementsCreated = 0;
   TiXmlElement * pXmlElement;
   for (pXmlElement = pTiXmlDoc->FirstChildElement(); pXmlElement != NULL; pXmlElement = pXmlElement->NextSiblingElement())
   {
      if (pXmlElement->Type() == TiXmlNode::ELEMENT)
      {
         cAutoIPtr<IGUIElement> pGUIElement;
         if (pGUIFactory->CreateElement(pXmlElement->Value(), pXmlElement, &pGUIElement) == S_OK)
         {
            if (AddElement(pGUIElement) == S_OK)
            {
               nElementsCreated++;
            }
         }
      }
   }

   m_bNeedLayout = (nElementsCreated > 0);

   return nElementsCreated;
}

///////////////////////////////////////

void cGUIContext::ClearGUI()
{
   RemoveAllElements();
   m_bNeedLayout = true;
}

///////////////////////////////////////

class cRenderElement
{
public:
   cRenderElement();
   tResult operator()(IGUIElement * pGUIElement);
};

cRenderElement::cRenderElement()
{
}

tResult cRenderElement::operator()(IGUIElement * pGUIElement)
{
   if (pGUIElement == NULL)
   {
      return E_POINTER;
   }

   if (!pGUIElement->IsVisible())
   {
      return S_FALSE;
   }

   cAutoIPtr<IGUIElementRenderer> pRenderer;
   if (pGUIElement->GetRenderer(&pRenderer) == S_OK)
   {
      if (pRenderer->Render(pGUIElement) == S_OK)
      {
         return S_OK;
      }
   }

   return E_FAIL;
}

///////////////////////////////////////

tResult cGUIContext::RenderGUI()
{
   if (m_bNeedLayout)
   {
      m_bNeedLayout = false;

      int viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);

      ForEachElement(cSizeAndPlaceElement(tGUIRect(0,0,viewport[2],viewport[3])));
   }

   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
   glDisable(GL_DEPTH_TEST);
   ForEachElement(cRenderElement());
#ifdef GUI_DEBUG
   RenderDebugInfo();
#endif
   glPopAttrib();

   return S_OK;
}

///////////////////////////////////////

tResult cGUIContext::ShowDebugInfo(const tGUIPoint & placement, const tGUIColor & textColor)
{
#ifdef GUI_DEBUG
   if (!m_bShowDebugInfo)
   {
      m_bShowDebugInfo = true;
      m_debugInfoPlacement = placement;
      m_debugInfoTextColor = textColor;
      return S_OK;
   }
#endif
   return S_FALSE;
}

///////////////////////////////////////

tResult cGUIContext::HideDebugInfo()
{
#ifdef GUI_DEBUG
   if (m_bShowDebugInfo)
   {
      m_bShowDebugInfo = false;
      return S_OK;
   }
#endif
   return S_FALSE;
}

///////////////////////////////////////

#ifdef GUI_DEBUG
static void DescribeElement(IGUIElement * pElement, char * psz, uint maxLength)
{
   Assert(pElement != NULL);
   cAutoIPtr<IGUIButtonElement> pButton;
   cAutoIPtr<IGUIPanelElement> pPanel;
   cAutoIPtr<IGUILabelElement> pLabel;
   cAutoIPtr<IGUIDialogElement> pDialog;
   const char * pszId = pElement->GetId();
   if (pElement->QueryInterface(IID_IGUIButtonElement, (void**)&pButton) == S_OK)
   {
      snprintf(psz, maxLength, "Button \"%s\"", pButton->GetText());
   }
   else if (pElement->QueryInterface(IID_IGUIPanelElement, (void**)&pPanel) == S_OK)
   {
      snprintf(psz, maxLength, "Panel '%s'", strlen(pszId) > 0 ? pszId : "<no id>");
   }
   else if (pElement->QueryInterface(IID_IGUILabelElement, (void**)&pLabel) == S_OK)
   {
      tGUIString text;
      if (pLabel->GetText(&text) == S_OK)
      {
         snprintf(psz, maxLength, "Label \"%s\"", text.c_str());
      }
      else
      {
         snprintf(psz, maxLength, "Label '%s'", strlen(pszId) > 0 ? pszId : "<no id>");
      }
   }
   else if (pElement->QueryInterface(IID_IGUIDialogElement, (void**)&pDialog) == S_OK)
   {
      tGUIString title;
      if (pDialog->GetTitle(&title) == S_OK)
      {
         snprintf(psz, maxLength, "Dialog \"%s\"", title.c_str());
      }
      else
      {
         snprintf(psz, maxLength, "Dialog '%s'", strlen(pszId) > 0 ? pszId : "<no id>");
      }
   }
   else
   {
      snprintf(psz, maxLength, "Element '%s'", strlen(pszId) > 0 ? pszId : "<no id>");
   }
}
#endif

///////////////////////////////////////

#ifdef GUI_DEBUG
template <typename CTYPE, size_t SIZE>
class cTextBuffer
{
public:
   cTextBuffer() : m_length(0)
   {
      m_buffer[0] = 0;
   }

   const CTYPE * GetBuffer() const
   {
      return m_buffer;
   }

   CTYPE * NextPointer()
   {
      m_length = strlen(m_buffer);
      if (m_length > 0)
      {
         strncat(m_buffer, "\n", _countof(m_buffer) - m_length);
         m_buffer[_countof(m_buffer) - 1] = 0;
         m_length = strlen(m_buffer);
      }
      return m_buffer + m_length;
   }

   uint MaxLength() const
   {
      return SIZE - m_length;
   }

   void NullTerminate()
   {
      m_buffer[SIZE - 1] = 0;
   }

private:
   uint m_length;
   CTYPE m_buffer[SIZE];
};

void cGUIContext::RenderDebugInfo()
{
   if (!m_bShowDebugInfo)
   {
      return;
   }

   cAutoIPtr<IGUIFont> pFont;
   if (GUIFontGetDefault(&pFont) == S_OK)
   {
      cTextBuffer<char, 200> text;

      snprintf(text.NextPointer(), text.MaxLength(), "Mouse: (%d, %d)", 
         Round(m_lastMousePos.x), Round(m_lastMousePos.y));

      cAutoIPtr<IGUIElement> pHitElement;
      if (GetHitElement(m_lastMousePos, &pHitElement) == S_OK)
      {
         DescribeElement(pHitElement, text.NextPointer(), text.MaxLength());

         tGUIPoint pos = pHitElement->GetPosition();
         snprintf(text.NextPointer(), text.MaxLength(), "Position: (%d, %d)", 
            Round(pos.x), Round(pos.y));

         tGUIPoint absPos = GUIElementAbsolutePosition(pHitElement);
         snprintf(text.NextPointer(), text.MaxLength(), "Absolute Position: (%d, %d)",
            Round(absPos.x), Round(absPos.y));

         tGUIPoint relPoint(m_lastMousePos - absPos);
         Assert(pHitElement->Contains(relPoint));
         snprintf(text.NextPointer(), text.MaxLength(), "Mouse (relative): (%d, %d)",
            Round(relPoint.x), Round(relPoint.y));

         text.NullTerminate();
      }

      tGUIRect rect(Round(m_debugInfoPlacement.x), Round(m_debugInfoPlacement.y), 0, 0);
      pFont->RenderText(text.GetBuffer(), -1, &rect, kRT_NoClip, m_debugInfoTextColor);
   }
}
#endif

///////////////////////////////////////

#ifdef GUI_DEBUG
bool cGUIContext::HandleInputEvent(const sInputEvent * pEvent)
{
   Assert(pEvent != NULL);

   if (pEvent->key == kMouseMove)
   {
      m_lastMousePos = pEvent->point;
   }

   return cGUIEventRouter<IGUIContext>::HandleInputEvent(pEvent);
}
#endif

///////////////////////////////////////

cGUIContext::cInputListener::cInputListener(cGUIContext * pOuter)
 : m_pOuter(pOuter)
{
}

///////////////////////////////////////

bool cGUIContext::cInputListener::OnInputEvent(const sInputEvent * pEvent)
{
   Assert(m_pOuter != NULL);
   return m_pOuter->HandleInputEvent(pEvent);
}

///////////////////////////////////////

void GUIContextCreate()
{
   cAutoIPtr<IGUIContext>(new cGUIContext);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CPPUNIT

///////////////////////////////////////////////////////////////////////////////
//
// CLASS cGUITestableEventRouter
//

class cGUITestableEventRouter : public cComObject<cGUIEventRouter<IGUIEventRouter>, &IID_IGUIEventRouter>
{
public:
   cGUITestableEventRouter();

   void FireInputEvent(long key, bool down, tVec2 point);

private:
   ulong m_time;
};

///////////////////////////////////////

cGUITestableEventRouter::cGUITestableEventRouter()
 : m_time(0)
{
}

///////////////////////////////////////

void cGUITestableEventRouter::FireInputEvent(long key, bool down, tVec2 point)
{
   sInputEvent event;
   event.key = key;
   event.down = down;
   event.point = point;
   event.time = m_time++;
   HandleInputEvent(&event);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS cGUITestEventListener
//

class cGUITestEventListener : public cComObject<IMPLEMENTS(IGUIEventListener)>
{
public:
   virtual tResult OnEvent(IGUIEvent * pEvent);
};

///////////////////////////////////////

tResult cGUITestEventListener::OnEvent(IGUIEvent * pEvent)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

class cGUIEventRouterTests : public CppUnit::TestCase
{
   CPPUNIT_TEST_SUITE(cGUIEventRouterTests);
   CPPUNIT_TEST_SUITE_END();

private:
   virtual void setUp();
   virtual void tearDown();

   cAutoIPtr<cGUITestableEventRouter> m_pEventRouter;
   cAutoIPtr<cGUITestEventListener> m_pEventListener;
};

///////////////////////////////////////

CPPUNIT_TEST_SUITE_REGISTRATION(cGUIEventRouterTests);

///////////////////////////////////////

void cGUIEventRouterTests::setUp()
{
   CPPUNIT_ASSERT(!m_pEventRouter);
   m_pEventRouter = new cGUITestableEventRouter;

   CPPUNIT_ASSERT(!m_pEventListener);
   m_pEventListener = new cGUITestEventListener;
   m_pEventRouter->AddEventListener(m_pEventListener);
}

///////////////////////////////////////

void cGUIEventRouterTests::tearDown()
{
   if (m_pEventRouter && m_pEventListener)
   {
      m_pEventRouter->RemoveEventListener(m_pEventListener);
   }

   SafeRelease(m_pEventRouter);
   SafeRelease(m_pEventListener);
}

///////////////////////////////////////////////////////////////////////////////

#endif // HAVE_CPPUNIT

///////////////////////////////////////////////////////////////////////////////
