/*
 Copyright (c) 2009 Apple Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization.
*/

/* Must be before OpenGL.framework is included.  Remove once fixed:
 * <rdar://problem/7872773>
 */
#include <GL/gl.h>
#include <GL/glext.h>
#define __gltypes_h_ 1

/* Must be first for:
 * <rdar://problem/6953344>
 */
#include "apple_glx_context.h"
#include "apple_glx_drawable.h"

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "apple_glx.h"
#include "glxconfig.h"
#include "apple_cgl.h"

/* mesa defines in glew.h, Apple in glext.h.
 * Due to namespace nightmares, just do it here.
 */
#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT 0x84F5
#endif

static bool pbuffer_make_current(struct apple_glx_context *ac,
                                 struct apple_glx_drawable *d);

static void pbuffer_destroy(Display * dpy, struct apple_glx_drawable *d);

static struct apple_glx_drawable_callbacks callbacks = {
   .type = APPLE_GLX_DRAWABLE_PBUFFER,
   .make_current = pbuffer_make_current,
   .destroy = pbuffer_destroy
};


/* Return true if an error occurred. */
bool
pbuffer_make_current(struct apple_glx_context *ac,
                     struct apple_glx_drawable *d)
{
   struct apple_glx_pbuffer *pbuf = &d->types.pbuffer;
   CGLError cglerr;

   assert(APPLE_GLX_DRAWABLE_PBUFFER == d->type);

   cglerr = apple_cgl.set_pbuffer(ac->context_obj, pbuf->buffer_obj, 0, 0, 0);

   if (kCGLNoError != cglerr) {
      fprintf(stderr, "set_pbuffer: %s\n", apple_cgl.error_string(cglerr));
      return true;
   }

   if (!ac->made_current) {
      apple_glapi_oglfw_viewport_scissor(0, 0, pbuf->width, pbuf->height);
      ac->made_current = true;
   }

   apple_glx_diagnostic("made pbuffer drawable 0x%lx current\n", d->drawable);

   return false;
}

void
pbuffer_destroy(Display * dpy, struct apple_glx_drawable *d)
{
   struct apple_glx_pbuffer *pbuf = &d->types.pbuffer;

   assert(APPLE_GLX_DRAWABLE_PBUFFER == d->type);

   apple_glx_diagnostic("destroying pbuffer for drawable 0x%lx\n",
                        d->drawable);

   apple_cgl.destroy_pbuffer(pbuf->buffer_obj);
   XFreePixmap(dpy, pbuf->xid);
}

void
apple_glx_pbuffer_destroy(Display * dpy, GLXPbuffer pbuf)
{
   if (!apple_glx_drawable_destroy_by_type(dpy, pbuf,
                                           APPLE_GLX_DRAWABLE_PBUFFER))
      __glXSendError(dpy, GLXBadPbuffer, pbuf, X_GLXDestroyPbuffer, false);
}

GLXDrawable
apple_glx_pbuffer_create(Display * dpy, struct glx_config *config,
                         unsigned int width, unsigned int height,
                         const int *attrib_list, GLboolean size_in_attribs)
{
   struct apple_glx_drawable *d;
   struct apple_glx_pbuffer *pbuf = NULL;
   CGLError err;
   Window root;
   int screen;
   Pixmap xid;
   struct glx_config *modes = (struct glx_config *) config;

   root = DefaultRootWindow(dpy);
   screen = DefaultScreen(dpy);

   /*
    * This pixmap is only used for a persistent XID.
    * The XC-MISC extension cleans up XIDs and reuses them transparently,
    * so we need to retain a server-side reference.
    */
   xid = XCreatePixmap(dpy, root, (unsigned int) 1,
                       (unsigned int) 1, DefaultDepth(dpy, screen));

   if (None == xid) {
      *errorcode = BadAlloc;
      return true;
   }

   if (apple_glx_drawable_create(dpy, screen, xid, &d, &callbacks)) {
      *errorcode = BadAlloc;
      return true;
   }

   /* The lock is held in d from create onward. */
   pbuf = &d->types.pbuffer;

   pbuf->xid = xid;
   pbuf->width = width;
   pbuf->height = height;

   err = apple_cgl.create_pbuffer(width, height, GL_TEXTURE_RECTANGLE_EXT,
                                  (modes->alphaBits > 0) ? GL_RGBA : GL_RGB,
                                  0, &pbuf->buffer_obj);

   if (kCGLNoError != err) {
      d->unlock(d);
      d->destroy(d);
      *errorcode = BadMatch;
      return true;
   }

   pbuf->fbconfigID = modes->fbconfigID;

   pbuf->event_mask = 0;

   d->unlock(d);

   return pbuf->xid;

 err:
   /*
    * apple_glx_pbuffer_create only sets the errorcode to core X11
    * errors.
    */
   __glXSendError(dpy, errorcode, 0, X_GLXCreatePbuffer, true);
   return NULL;
}



/* Return true if an error occurred. */
static bool
get_max_size(int *widthresult, int *heightresult)
{
   CGLContextObj oldcontext;
   GLint ar[2];

   oldcontext = apple_cgl.get_current_context();

   if (!oldcontext) {
      /* 
       * There is no current context, so we need to make one in order
       * to call glGetInteger.
       */
      CGLPixelFormatObj pfobj;
      CGLError err;
      CGLPixelFormatAttribute attr[10];
      int c = 0;
      GLint vsref = 0;
      CGLContextObj newcontext;

      attr[c++] = kCGLPFAColorSize;
      attr[c++] = 32;
      attr[c++] = 0;

      err = apple_cgl.choose_pixel_format(attr, &pfobj, &vsref);
      if (kCGLNoError != err) {
         if (getenv("LIBGL_DIAGNOSTIC")) {
            printf("choose_pixel_format error in %s: %s\n", __func__,
                   apple_cgl.error_string(err));
         }

         return true;
      }


      err = apple_cgl.create_context(pfobj, NULL, &newcontext);

      if (kCGLNoError != err) {
         if (getenv("LIBGL_DIAGNOSTIC")) {
            printf("create_context error in %s: %s\n", __func__,
                   apple_cgl.error_string(err));
         }

         apple_cgl.destroy_pixel_format(pfobj);

         return true;
      }

      err = apple_cgl.set_current_context(newcontext);

      if (kCGLNoError != err) {
         printf("set_current_context error in %s: %s\n", __func__,
                apple_cgl.error_string(err));
         return true;
      }


      glGetIntegerv(GL_MAX_VIEWPORT_DIMS, ar);

      apple_cgl.set_current_context(oldcontext);
      apple_cgl.destroy_context(newcontext);
      apple_cgl.destroy_pixel_format(pfobj);
   }
   else {
      /* We have a valid context. */

      glGetIntegerv(GL_MAX_VIEWPORT_DIMS, ar);
   }

   *widthresult = ar[0];
   *heightresult = ar[1];

   return false;
}

bool
apple_glx_pbuffer_query(GLXPbuffer p, int attr, unsigned int *value)
{
   bool result = false;
   struct apple_glx_drawable *d;
   struct apple_glx_pbuffer *pbuf;

   d = apple_glx_drawable_find_by_type(p, APPLE_GLX_DRAWABLE_PBUFFER,
                                       APPLE_GLX_DRAWABLE_LOCK);

   if (d) {
      pbuf = &d->types.pbuffer;

      switch (attr) {
      case GLX_WIDTH:
         *value = pbuf->width;
         result = true;
         break;

      case GLX_HEIGHT:
         *value = pbuf->height;
         result = true;
         break;

      case GLX_PRESERVED_CONTENTS:
         *value = true;
         result = true;
         break;

      case GLX_LARGEST_PBUFFER:{
            int width, height;
            if (get_max_size(&width, &height)) {
               fprintf(stderr, "internal error: "
                       "unable to find the largest pbuffer!\n");
            }
            else {
               *value = width;
               result = true;
            }
         }
         break;

      case GLX_FBCONFIG_ID:
         *value = pbuf->fbconfigID;
         result = true;
         break;

      case GLX_EVENT_MASK_SGIX:
         result = apple_glx_pbuffer_get_event_mask(drawable, value);
         break;
      }

      d->unlock(d);
   }

   return result;
}

int
apple_get_drawable_attribute(Display * dpy, GLXDrawable drawable,
                             int attribute, unsigned int *value)
{
   Window root;
   int x, y;
   unsigned int width, height, bd, depth;

   if (apple_glx_pixmap_query(drawable, attribute, value))
      return 0;                   /*done */

   if (apple_glx_pbuffer_query(drawable, attribute, value))
      return 0;                   /*done */

   /*
    * The OpenGL spec states that we should report GLXBadDrawable if
    * the drawable is invalid, however doing so would require that we
    * use XSetErrorHandler(), which is known to not be thread safe.
    * If we use a round-trip call to validate the drawable, there could
    * be a race, so instead we just opt in favor of letting the
    * XGetGeometry request fail with a GetGeometry request X error
    * rather than GLXBadDrawable, in what is hoped to be a rare
    * case of an invalid drawable.  In practice most and possibly all
    * X11 apps using GLX shouldn't notice a difference.
    */
   if (XGetGeometry
       (dpy, drawable, &root, &x, &y, &width, &height, &bd, &depth)) {
      switch (attribute) {
      case GLX_WIDTH:
         *value = width;
         break;

      case GLX_HEIGHT:
         *value = height;
         break;

      case GLX_EVENT_MASK_SGIX:
         *value = 0;
      }
   }

   return 0;
}

void
apple_change_drawable_attribute(Display * dpy, GLXDrawable drawable,
                                const CARD32 * attribs, size_t num_attribs)
{
   for (i = 0; i < num_attribs; i++) {
      switch(attribs[i * 2]) {
      case GLX_EVENT_MASK:
         apple_glx_pbuffer_set_event_mask(drawable, value);
         break;
      }
   }
}

bool
apple_glx_pbuffer_set_event_mask(GLXDrawable drawable, unsigned long mask)
{
   struct apple_glx_drawable *d;
   bool result = false;

   d = apple_glx_drawable_find_by_type(drawable, APPLE_GLX_DRAWABLE_PBUFFER,
                                       APPLE_GLX_DRAWABLE_LOCK);

   if (d) {
      d->types.pbuffer.event_mask = mask;
      result = true;
      d->unlock(d);
   }

   return result;
}

bool
apple_glx_pbuffer_get_event_mask(GLXDrawable drawable, unsigned long *mask)
{
   struct apple_glx_drawable *d;
   bool result = false;

   d = apple_glx_drawable_find_by_type(drawable, APPLE_GLX_DRAWABLE_PBUFFER,
                                       APPLE_GLX_DRAWABLE_LOCK);
   if (d) {
      *mask = d->types.pbuffer.event_mask;
      result = true;
      d->unlock(d);
   }

   return result;
}
