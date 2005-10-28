///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_GUIEVENTROUTER_H
#define INCLUDED_GUIEVENTROUTER_H

#include "guiapi.h"

#include "connptimpl.h"

#include <list>

#ifdef _MSC_VER
#pragma once
#endif

struct sInputEvent;

typedef std::list<IGUIElement *> tGUIElementList;

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cGUIEventRouter
//
// Template base class that implements the IGUIEventRouter methods. Mainly
// for testability. A derived class that includes the event routing parts but
// excludes the connection to the real input system, etc. will be used by unit 
// tests.
//
// The template parameter INTRFC is a derivative of IGUIEventRouter
// (e.g., IGUIContext, which inherits from IGUIEventRouter)
//
/// @class cGUIEventRouter
/// @brief Template base class that implements the IGUIEventRouter methods.

template <typename INTRFC>
class cGUIEventRouter : public cConnectionPoint<INTRFC, IGUIEventListener>
{
public:
   cGUIEventRouter();
   ~cGUIEventRouter();

   ////////////////////////////////////
   // IGUIEventRouter methods

   virtual tResult AddEventListener(IGUIEventListener * pListener);
   virtual tResult RemoveEventListener(IGUIEventListener * pListener);

   virtual tResult GetFocus(IGUIElement * * ppElement);
   virtual tResult SetFocus(IGUIElement * pElement);

protected:
   ////////////////////////////////////

   tResult GetMouseOver(IGUIElement * * ppElement);
   tResult SetMouseOver(IGUIElement * pElement);

   tResult GetDrag(IGUIElement * * ppElement);
   tResult SetDrag(IGUIElement * pElement);

   IGUIElement * AccessFocus();
   IGUIElement * AccessMouseOver();
   IGUIElement * AccessDrag();

   uint GetElementCount() const;

   tResult GetElement(const tChar * pszId, IGUIElement * * ppElement);
   tResult AddElement(IGUIElement * pElement);
   tResult RemoveElement(IGUIElement * pElement);
   tResult HasElement(IGUIElement * pElement) const;
   void RemoveAllElements();

   template <typename F>
   tResult ForEachElement(F f)
   {
      tResult result = E_FAIL;
      tGUIElementList::iterator iter;
      for (iter = m_elements.begin(); iter != m_elements.end(); iter++)
      {
         result = f(*iter);
         // S_OK means success (good)
         // S_FALSE means not visible (good)
         // otherwise, error (bad)
         if (FAILED(result))
         {
            break;
         }
      }
      return result;
   }

   tGUIElementList::iterator BeginElements();
   tGUIElementList::iterator EndElements();

   tResult GetHitElements(const tGUIPoint & point, std::list<IGUIElement*> * pElements) const;
   tResult GetHitElement(const tGUIPoint & point, IGUIElement * * ppElement) const;

   bool DoEvent(IGUIEvent * pEvent);
   bool BubbleEvent(IGUIEvent * pEvent);
   bool BubbleEvent(IGUIElement * pStartElement, IGUIEvent * pEvent);

   tResult GetActiveModalDialog(IGUIDialogElement * * ppModalDialog);

   void DoDragDrop(const sInputEvent * pInputEvent, tGUIEventCode eventCode, IGUIElement * pMouseOver, IGUIElement * pDrag);
   void DoMouseEnterExit(const sInputEvent * pInputEvent, IGUIElement * pMouseOver, IGUIElement * pRestrictTo);

   bool HandleInputEvent(const sInputEvent * pInputEvent);

private:
   tGUIElementList m_elements;

   cAutoIPtr<IGUIElement> m_pFocus, m_pMouseOver, m_pDrag;
};

///////////////////////////////////////

template <typename INTRFC>
inline IGUIElement * cGUIEventRouter<INTRFC>::AccessFocus()
{
   return m_pFocus;
}

///////////////////////////////////////

template <typename INTRFC>
inline IGUIElement * cGUIEventRouter<INTRFC>::AccessMouseOver()
{
   return m_pMouseOver;
}

///////////////////////////////////////

template <typename INTRFC>
inline IGUIElement * cGUIEventRouter<INTRFC>::AccessDrag()
{
   return m_pDrag;
}

///////////////////////////////////////

template <typename INTRFC>
inline uint cGUIEventRouter<INTRFC>::GetElementCount() const
{
   return m_elements.size();
}

///////////////////////////////////////

template <typename INTRFC>
inline tGUIElementList::iterator cGUIEventRouter<INTRFC>::BeginElements()
{
   return m_elements.begin();
}

///////////////////////////////////////

template <typename INTRFC>
inline tGUIElementList::iterator cGUIEventRouter<INTRFC>::EndElements()
{
   return m_elements.end();
}

///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_GUIEVENTROUTER_H
