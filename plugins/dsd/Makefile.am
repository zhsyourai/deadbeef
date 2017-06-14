if HAVE_DSD
pkglib_LTLIBRARIES = dsd.la
dsd_la_SOURCES = dsd.c
dsd_la_LDFLAGS = -module -avoid-version -export-symbols-regex

dsd_la_LIBADD = $(LDADD) $(dsd_LIBS)
AM_CFLAGS = $(CFLAGS) $(dsd_CFLAGS) -std=c99
endif
