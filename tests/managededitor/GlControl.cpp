///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "GlControl.h"

#include <GL/glew.h>
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGlControl
//

cGlControl::cGlControl()
 : m_hDc(NULL)
 , m_hGlrc(NULL)
{
}

void cGlControl::SwapBuffers()
{
   ::SwapBuffers(m_hDc);
}

void cGlControl::OnHandleCreated(System::EventArgs ^ e)
{
   System::Windows::Forms::UserControl::OnHandleCreated(e);

   HWND hWnd = reinterpret_cast<HWND>(Handle.ToPointer());

   m_hDc = GetDC(hWnd);

   int bpp = GetDeviceCaps(m_hDc, BITSPIXEL);

   PIXELFORMATDESCRIPTOR pfd;
   memset(&pfd, 0, sizeof(pfd));
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = bpp;
   pfd.cDepthBits = bpp;
   pfd.cStencilBits = bpp;
   pfd.dwLayerMask = PFD_MAIN_PLANE;

   int pixelFormat = ChoosePixelFormat(m_hDc, &pfd);
   while ((pixelFormat == 0) && (pfd.cStencilBits > 0))
   {
      pfd.cStencilBits /= 2;
      pixelFormat = ChoosePixelFormat(m_hDc, &pfd);
   }

   SetPixelFormat(m_hDc, pixelFormat, &pfd);
   m_hGlrc = wglCreateContext(m_hDc);
   wglMakeCurrent(m_hDc, m_hGlrc);

   glewInit();
}

void cGlControl::OnHandleDestroyed(System::EventArgs ^ e)
{
   System::Windows::Forms::UserControl::OnHandleDestroyed(e);

   HWND hWnd = reinterpret_cast<HWND>(Handle.ToPointer());

   if (m_hDc != NULL)
   {
      wglMakeCurrent(m_hDc, NULL);
      ReleaseDC(hWnd, m_hDc);
      m_hDc = NULL;
   }

   if (m_hGlrc != NULL)
   {
      wglDeleteContext(m_hGlrc);
      m_hGlrc = NULL;
   }
}

///////////////////////////////////////////////////////////////////////////////
