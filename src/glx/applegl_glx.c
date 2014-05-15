/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@bitplanet.net)
 */

#include <stdbool.h>
#include <dlfcn.h>

#include "glxclient.h"
#include "apple/apple_glx_context.h"
#include "apple/apple_glx.h"
#include "apple/apple_cgl.h"
#include "glx_error.h"
#include "dri_common.h"

struct appledri_display
{
   __GLXDRIdisplay base;
};

struct appledri_screen
{
   struct glx_screen base;

   __GLXDRIscreen vtable;
};

struct appledri_drawable
{
   __GLXDRIdrawable base;
};

static void
applegl_destroy_context(struct glx_context *gc)
{
   apple_glx_destroy_context(&gc->driContext, gc->psc->dpy);
}

static int
applegl_bind_context(struct glx_context *gc, struct glx_context *old,
		     GLXDrawable draw, GLXDrawable read)
{
   struct appledri_drawable *pdraw, *pread;
   Display *dpy = gc->psc->dpy;

   pdraw = (struct appledri_drawable *) driFetchDrawable(gc, draw);
   pread = (struct appledri_drawable *) driFetchDrawable(gc, read);

   if (pdraw == NULL || pread == NULL)
      return GLXBadDrawable;

   bool error = apple_glx_make_current_context(dpy,
					       (old && old != &dummyContext) ? old->driContext : NULL,
					       gc ? gc->driContext : NULL, draw);

   apple_glx_diagnostic("%s: error %s\n", __func__, error ? "YES" : "NO");
   if (error)
      return 1; /* GLXBadContext is the same as Success (0) */

   apple_glapi_set_dispatch();

   return Success;
}

static void
applegl_unbind_context(struct glx_context *gc, struct glx_context *new)
{
   Display *dpy;
   bool error;

   /* If we don't have a context, then we have nothing to unbind */
   if (!gc)
      return;

   /* If we have a new context, keep this one around and remove it during bind. */
   if (new)
      return;

   dpy = gc->psc->dpy;

   error = apple_glx_make_current_context(dpy,
					  (gc != &dummyContext) ? gc->driContext : NULL,
					  NULL, None);

   apple_glx_diagnostic("%s: error %s\n", __func__, error ? "YES" : "NO");
}

static void
applegl_wait_gl(struct glx_context *gc)
{
   glFinish();
}

static void
applegl_wait_x(struct glx_context *gc)
{
   Display *dpy = gc->psc->dpy;
   apple_glx_waitx(dpy, gc->driContext);
}

static void *
applegl_get_proc_address(const char *symbol)
{
   return dlsym(apple_cgl_get_dl_handle(), symbol);
}

static const struct glx_context_vtable applegl_context_vtable = {
   .destroy             = applegl_destroy_context,
   .bind                = applegl_bind_context,
   .unbind              = applegl_unbind_context,
   .wait_gl             = applegl_wait_gl,
   .wait_x              = applegl_wait_x,
   .use_x_font          = DRI_glXUseXFont,
   .bind_tex_image      = NULL,
   .release_tex_image   = NULL,
   .get_proc_address    = applegl_get_proc_address,
};

struct glx_context *
applegl_create_context(struct glx_screen *psc,
		       struct glx_config *config,
		       struct glx_context *shareList, int renderType)
{
   struct glx_context *gc;
   int errorcode;
   bool x11error;
   Display *dpy = psc->dpy;
   int screen = psc->scr;

   /* TODO: Integrate this with apple_glx_create_context and make
    * struct apple_glx_context inherit from struct glx_context. */

   gc = calloc(1, sizeof(*gc));
   if (gc == NULL)
      return NULL;

   if (!glx_context_init(gc, psc, config)) {
      free(gc);
      return NULL;
   }

   gc->vtable = &applegl_context_vtable;
   gc->driContext = NULL;

   /* TODO: darwin: Integrate with above to do indirect */
   if(apple_glx_create_context(&gc->driContext, dpy, screen, config, 
			       shareList ? shareList->driContext : NULL,
			       &errorcode, &x11error)) {
      __glXSendError(dpy, errorcode, 0, X_GLXCreateContext, x11error);
      gc->vtable->destroy(gc);
      return NULL;
   }

   gc->currentContextTag = -1;
   gc->config = config;
   gc->isDirect = GL_TRUE;
   gc->xid = 1; /* Just something not None, so we know when to destroy
		 * it in MakeContextCurrent. */

   return gc;
}

static struct glx_context *
applegl_create_context_attribs(struct glx_screen *psc,
                               struct glx_config *config,
                               struct glx_context *shareList,
                               unsigned num_attribs,
                               const uint32_t *attribs,
                               unsigned *error)
{
   return applegl_create_context(psc, config, shareList, 0);
}

static int64_t
applegl_swap_buffers(__GLXDRIdrawable * pdraw, int64_t unused1, int64_t unused2,
                     int64_t unused3, Bool flush)
{
   struct appledri_screen *psc = (struct appledri_screen *) pdraw->psc;
   Display *dpy = psc->base.dpy;

   if (flush) {
      glFlush();
   }

   struct glx_context * gc = __glXGetCurrentContext();
   if (gc && apple_glx_is_current_drawable(dpy, gc->driContext, pdraw->drawable)) {
      apple_glx_swap_buffers(gc->driContext);
   } else {
      __glXSendError(dpy, GLXBadCurrentWindow, 0, X_GLXSwapBuffers, false);
   }

   return 0;
}

static void
applegl_destroy_drawable(__GLXDRIdrawable * pdraw)
{
   struct appledri_screen *psc = (struct appledri_screen *) pdraw->psc;
   struct appledri_drawable *pdp = (struct appledri_drawable *) pdraw;

   free(pdraw);
}

static __GLXDRIdrawable *
applegl_create_drawable(struct glx_screen *base, XID xDrawable,
                        GLXDrawable drawable, struct glx_config *modes)
{
   struct appledri_screen *psc = (struct appledri_screen *) base;
   struct appledri_drawable *pdp;

   pdp = calloc(1, sizeof(*pdp));
   if (!pdp)
      return NULL;

 #if 0
   {
      XWindowAttributes xwattr;
      XVisualInfo *visinfo;

      XGetWindowAttributes(dpy, win, &xwattr);

      visinfo = glXGetVisualFromFBConfig(dpy, config);

      if (NULL == visinfo) {
         __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateWindow, false);
         return None;
      }

      if (visinfo->visualid != XVisualIDFromVisual(xwattr.visual)) {
         __glXSendError(dpy, BadMatch, 0, X_GLXCreateWindow, true);
         return None;

         free(visinfo);
      }
#endif

   pdp->base.xDrawable = xDrawable;
   pdp->base.drawable = drawable;
   pdp->base.psc = &psc->base;

   pdp->base.destroyDrawable = applegl_destroy_drawable;

   return &pdp->base;
}

static const struct glx_screen_vtable applegl_screen_vtable = {
   .create_context         = applegl_create_context,
   .create_context_attribs = applegl_create_context_attribs,
   .query_renderer_integer = NULL,
   .query_renderer_string  = NULL,
};

_X_HIDDEN struct glx_screen *
applegl_create_screen(int screen, struct glx_display * priv)
{
   struct appledri_screen *psc;
   __GLXDRIscreen *psp;

   psc = calloc(1, sizeof *psc);
   if (psc == NULL)
      return NULL;

   glx_screen_init(&psc->base, screen, priv);
   psc->base.vtable = &applegl_screen_vtable;

   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->createDrawable = applegl_create_drawable;
   psp->swapBuffers = applegl_swap_buffers;

   return &psc->base;
}

static void
applegl_destroy_display(__GLXDRIdisplay * dpy)
{
   free(dpy);
}

_X_HIDDEN __GLXDRIdisplay *
applegl_create_display(struct glx_display *glx_dpy)
{
   struct appledri_display *pdpyp;

   if (apple_init_glx(glx_dpy->dpy))
      return NULL;

   pdpyp = malloc(sizeof *pdpyp);
   if (pdpyp == NULL)
      return NULL;

   pdpyp->base.destroyDisplay = applegl_destroy_display;
   pdpyp->base.createScreen = applegl_create_screen;
   return &pdpyp->base;

}
