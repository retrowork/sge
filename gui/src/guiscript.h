///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_GUISCRIPT_H
#define INCLUDED_GUISCRIPT_H

#include "guielementbase.h"

#ifdef _MSC_VER
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIScriptElement
//

class cGUIScriptElement : public cComObject<cGUIElementBase<IGUIScriptElement>, &IID_IGUIScriptElement>
{
public:
   cGUIScriptElement();
   ~cGUIScriptElement();

   virtual tResult GetScript(tGUIString * pScript);
   virtual tResult SetScript(const tChar * pszScript);

private:
   tGUIString m_script;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIScriptElementFactory
//

class cGUIScriptElementFactory : public cComObject<IMPLEMENTS(IGUIElementFactory)>
{
public:
   virtual tResult CreateElement(const TiXmlElement * pXmlElement, IGUIElement * * ppElement);
};

///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_GUISCRIPT_H
