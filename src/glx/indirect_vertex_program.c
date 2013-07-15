/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>
#include <GL/gl.h>
#include "indirect.h"
#include "glxclient.h"
#include "indirect_vertex_array.h"
#include <GL/glxproto.h>

#if !defined(__GNUC__)
#  define __builtin_expect(x, y) x
#endif

static void
get_parameter(unsigned opcode, unsigned size, GLenum target, GLuint index,
              void *params)
{
   struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   const GLuint cmdlen = 12;

   if (__builtin_expect(dpy != NULL, 1)) {
      GLubyte const *pc = __glXSetupVendorRequest(gc,
                                                  X_GLXVendorPrivateWithReply,
                                                  opcode, cmdlen);

      *((GLenum *) (pc + 0)) = target;
      *((GLuint *) (pc + 4)) = index;
      *((GLuint *) (pc + 8)) = 0;

      (void) __glXReadReply(dpy, size, params, GL_FALSE);
      UnlockDisplay(dpy);
      SyncHandle();
   }
   return;
}


void
__indirect_glGetProgramEnvParameterfvARB(GLenum target, GLuint index,
                                         GLfloat * params)
{
   get_parameter(1296, 4, target, index, params);
}


void
__indirect_glGetProgramEnvParameterdvARB(GLenum target, GLuint index,
                                         GLdouble * params)
{
   get_parameter(1297, 8, target, index, params);
}


void
__indirect_glGetProgramLocalParameterfvARB(GLenum target, GLuint index,
                                           GLfloat * params)
{
   get_parameter(1305, 4, target, index, params);
}


void
__indirect_glGetProgramLocalParameterdvARB(GLenum target, GLuint index,
                                           GLdouble * params)
{
   get_parameter(1306, 8, target, index, params);
}
