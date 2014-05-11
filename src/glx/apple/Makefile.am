noinst_LTLIBRARIES = libappleglx.la

libappleglx_la_SOURCES = \
	apple_cgl.c \
	apple_glapi.c \
	apple_glx.c \
	apple_glx_context.c \
	apple_glx_drawable.c \
	apple_glx_log.c \
	apple_glx_pbuffer.c \
	apple_glx_pixmap.c \
	apple_glx_surface.c \
	apple_visual.c \
	apple_xgl_api_read.c \
	apple_xgl_api_stereo.c \
	apple_xgl_api_viewport.c \
	appledri.c \
	glx_empty.c \
	glxreply.c

libappleglx_la_LDFLAGS += -lXplugin -framework ApplicationServices -framework CoreFoundation
