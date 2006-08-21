////////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_RENDERFONTFTGL_H
#define INCLUDED_RENDERFONTFTGL_H

#include "renderapi.h"

#ifdef _MSC_VER
#pragma once
#endif

class FTGLTextureFont;

////////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRenderFontFtgl
//

class cRenderFontFtgl : public cComObject<IMPLEMENTS(IRenderFont)>
{
   cRenderFontFtgl(const cRenderFontFtgl &);
   const cRenderFontFtgl & operator =(const cRenderFontFtgl &);

   cRenderFontFtgl(FTGLTextureFont * pFtglFont);

public:
   virtual ~cRenderFontFtgl();

   static tResult Create(const tChar * pszFont, int fontPointSize, IRenderFont * * ppFont);

   virtual tResult MeasureText(const tChar * pszText, int textLength, int * pWidth, int * pHeight) const;

   virtual tResult RenderText(const tChar * pszText, int textLength, int x, int y) const;

private:
   FTGLTextureFont * m_pFont;
};

////////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_RENDERFONTFTGL_H
