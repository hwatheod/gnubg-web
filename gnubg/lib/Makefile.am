##
## All targets have been declared "noinst" for inclusion with gnubg.  As
## a standalone library, it would be better for them to be installed.
##


AM_CPPFLAGS = -DUSE_GTK_UNDEF -I$(srcdir)/.. @GLIB_CFLAGS@

LIBADD = @GLIB_LIBS@

noinst_LTLIBRARIES = libevent.la libsimd.la

libsimd_la_SOURCES = neuralnetsse.c inputs.c ../output.c
libsimd_la_CFLAGS = $(AM_CFLAGS) $(SIMD_CFLAGS)

libevent_la_SOURCES = list.c neuralnet.c mt19937ar.c isaac.c md5.c simd.h mm_malloc.h cache.c \
		      cache.h list.h neuralnet.h mt19937ar.h isaac.h isaacs.h md5.h simd.h mm_malloc.h $(srcdir)/../eval.h gnubg-types.h sigmoid.h
libevent_la_LIBADD = libsimd.la

noinst_HEADERS = cache.h list.h neuralnet.h mt19937ar.h isaac.h isaacs.h md5.h simd.h mm_malloc.h $(srcdir)/../eval.h $(srcdir)/../output.h 

