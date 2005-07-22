///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guifontd3d.h"
#include "sys.h"

#ifndef _WIN32
#error ("This file is for Windows compilation only")
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if HAVE_DIRECTX
#include <d3dx9.h>
#endif

#include <cstring>

#include "dbgalloc.h" // must be last header

#if HAVE_DIRECTX
#ifdef _DEBUG
#pragma comment(lib, "d3dx9d")
#else
#pragma comment(lib, "d3dx9")
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGUIFontD3D
//

///////////////////////////////////////

cGUIFontD3D::cGUIFontD3D(ID3DXFont * pD3dxFont)
#if HAVE_DIRECTX
 : m_pD3dxFont(CTAddRef(pD3dxFont))
#endif
{
}

///////////////////////////////////////

cGUIFontD3D::~cGUIFontD3D()
{
}

///////////////////////////////////////

tResult cGUIFontD3D::RenderText(const char * pszText, int textLength, tRect * pRect,
                                uint flags, const cColor & color) const
{
#if HAVE_DIRECTX
   if (!!m_pD3dxFont)
   {
      RECT rect;
      rect.left = pRect->left;
      rect.top = pRect->top;
      rect.right = pRect->right;
      rect.bottom = pRect->bottom;

      DWORD format = 0;
      if (flags & kRT_Center) format |= DT_CENTER;
      if (flags & kRT_VCenter) format |= DT_VCENTER;
      if (flags & kRT_NoClip) format |= DT_NOCLIP;
      if (flags & kRT_CalcRect) format |= DT_CALCRECT;
      if (flags & kRT_SingleLine) format |= DT_SINGLELINE;
      if (flags & kRT_Bottom) format |= DT_BOTTOM;

      m_pD3dxFont->DrawText(NULL, pszText, textLength, &rect, format, color.GetARGB());

      if (flags & kRT_CalcRect)
      {
         pRect->left = rect.left;
         pRect->top = rect.top;
         pRect->right = rect.right;
         pRect->bottom = rect.bottom;
      }

      return S_OK;
   }
#endif // HAVE_DIRECTX

   return E_FAIL;
}

///////////////////////////////////////

tResult cGUIFontD3D::RenderText(const wchar_t * pszText, int textLength, tRect * pRect,
                                uint flags, const cColor & color) const
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////

tResult GUIFontCreateD3D(const cGUIFontDesc & fontDesc, IGUIFont * * ppFont)
{
#if HAVE_DIRECTX
   if (ppFont == NULL)
   {
      return E_POINTER;
   }

   HDC hScreenDC = GetDC(NULL);
   if (hScreenDC == NULL)
   {
      return E_FAIL;
   }

   int height = -MulDiv(fontDesc.GetPointSize(), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);

   ReleaseDC(NULL, hScreenDC), hScreenDC = NULL;

   cAutoIPtr<IDirect3DDevice9> pD3DDevice9;
   if (SysGetDirect3DDevice9(&pD3DDevice9) == S_OK)
   {
      cAutoIPtr<ID3DXFont> pD3DXFont;
      if (D3DXCreateFont(pD3DDevice9, height, 0, fontDesc.GetBold() ? FW_EXTRABOLD : FW_NORMAL,
         4, fontDesc.GetItalic(), DEFAULT_CHARSET, OUT_TT_PRECIS, PROOF_QUALITY,
         DEFAULT_PITCH | FF_DONTCARE, fontDesc.GetFace(), &pD3DXFont) == S_OK)
      {
         cAutoIPtr<cGUIFontD3D> pFont(new cGUIFontD3D(pD3DXFont));
         if (!pFont)
         {
            return E_OUTOFMEMORY;
         }

         *ppFont = CTAddRef(pFont);
         return S_OK;
      }
   }
#endif // HAVE_DIRECTX

   return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
