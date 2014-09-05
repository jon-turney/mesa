/*
 * Copyright © 2014 Jon TURNEY
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
  Wrapper functions for calling WGL extension functions

  XXX: should handle wglGetProcAddress returning NULL better
 */

#include "wgl.h"

const char *wglGetExtensionsStringARB(HDC hdc_)
{
   PFNWGLGETEXTENSIONSSTRINGARBPROC proc;
   proc = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
   return proc(hdc_);
}

HGLRC wglCreateContextAttribsARB(HDC hdc_, HGLRC hShareContext_,
                                     const int *attribList_)
{
   PFNWGLCREATECONTEXTATTRIBSARBPROC proc;
   proc = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
   return proc(hdc_, hShareContext_, attribList_);
}

BOOL wglMakeContextCurrentARB(HDC hDrawDC_, HDC hReadDC_, HGLRC hglrc_)
{
   PFNWGLMAKECONTEXTCURRENTARBPROC proc;
   proc = (PFNWGLMAKECONTEXTCURRENTARBPROC)wglGetProcAddress("wglMakeContextCurrentARB");
   return proc(hDrawDC_, hReadDC_, hglrc_ );
}

HPBUFFERARB wglCreatePbufferARB(HDC hDC_, int iPixelFormat_, int iWidth_,
                                int iHeight_, const int *piAttribList_)
{
   PFNWGLCREATEPBUFFERARBPROC proc;
   proc = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
   return proc (hDC_, iPixelFormat_, iWidth_, iHeight_, piAttribList_ );
}

HDC wglGetPbufferDCARB(HPBUFFERARB hPbuffer_)
{
   PFNWGLGETPBUFFERDCARBPROC proc;
   proc = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
   return proc(hPbuffer_ );
}

int wglReleasePbufferDCARB(HPBUFFERARB hPbuffer_, HDC hDC_)
{
   PFNWGLRELEASEPBUFFERDCARBPROC proc;
   proc = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
   return proc(hPbuffer_, hDC_ );
}

BOOL wglDestroyPbufferARB(HPBUFFERARB hPbuffer_)
{
   PFNWGLDESTROYPBUFFERARBPROC proc;
   proc = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
   return proc(hPbuffer_);
}
