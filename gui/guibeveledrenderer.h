///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_GUIBEVELEDRENDERER_H
#define INCLUDED_GUIBEVELEDRENDERER_H

#include "gui/guiapi.h"
#include "gui/guielementapi.h"

#ifdef _MSC_VER
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIBeveledRenderer
//

enum eBeveledColor
{
   kBC_Shadow,
   kBC_Highlight,
   kBC_Face,
   kBC_Text,

   kBC_NumColors
};

class cGUIBeveledRenderer : public cComObject<IMPLEMENTS(IGUIElementRenderer)>
{
public:
   cGUIBeveledRenderer(uint bevel, const tGUIColor colorScheme[kBC_NumColors]);
   ~cGUIBeveledRenderer();

   virtual tResult Render(IGUIElement * pElement, const tGUIPoint & position, uint state);
   virtual tResult GetPreferredSize(IGUIElement * pElement, const tGUISize & parentSize, tGUISize * pSize);
   virtual tResult AllocateBorderSpace(IGUIElement * pElement, tGUIRect * pRect);

private:
   uint GetBevel() const { return m_bevel; }
   const tGUIColor & GetColor(eBeveledColor color) const { return m_colorScheme[color]; }

   tResult ButtonRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize ButtonPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   tResult LabelRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize LabelPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   tResult ListBoxRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize ListBoxPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;
   uint ListBoxPreferredItemHeight(IRenderFont * pFont) const;

   tResult ScrollBarRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize ScrollBarPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   tResult TextEditRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize TextEditPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   tResult TitleBarRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize TitleBarPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   tResult ContainerRender(IGUIElement * pElement, IGUIStyle * pStyle, IRenderFont * pFont, const tGUIRect & rect, uint state);
   tGUISize ContainerPreferredSize(IGUIElement * pElement, IRenderFont * pFont, const tGUISize & parentSize) const;

   typedef tResult (cGUIBeveledRenderer::*tRenderMethod)(IGUIElement *, IGUIStyle *, IRenderFont *, const tGUIRect &, uint state);
   typedef tGUISize (cGUIBeveledRenderer::*tPreferredSizeMethod)(IGUIElement *, IRenderFont *, const tGUISize &) const;

   struct sMethodTableEntry
   {
      const IID * pIID;
      tRenderMethod pfnRender;
      tPreferredSizeMethod pfnPreferredSize;
   };

   static const sMethodTableEntry gm_methodTable[];

   uint m_bevel;
   tGUIColor m_colorScheme[kBC_NumColors];
};

///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_GUIBEVELEDRENDERER_H
