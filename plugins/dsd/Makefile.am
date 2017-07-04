if HAVE_DSD
pkglib_LTLIBRARIES = dsd.la
dsd_la_SOURCES = dsd.c\
    libdsd/include/callbacks.h\
    libdsd/include/decoder.h\
    libdsd/include/export.h\
    libdsd/include/libdsd.h\
    libdsd/include/metadata.h\
    libdsd/include/typedefs.h\
    libdsd/src/decoder.c\
    libdsd/src/output.c\
    libdsd/src/include/dsdiff_format.h\
    libdsd/src/include/dsf_format.h\
    libdsd/src/include/libdsd_endian.h\
    libdsd/src/include/output.h\
    libdsd/libdstdec/ccp_calc.c\
    libdsd/libdstdec/ccp_calc.h\
    libdsd/libdstdec/conststr.h\
    libdsd/libdstdec/dst_ac.c\
    libdsd/libdstdec/dst_ac.h\
    libdsd/libdstdec/dst_data.c\
    libdsd/libdstdec/dst_data.h\
    libdsd/libdstdec/dst_fram.c\
    libdsd/libdstdec/dst_fram.h\
    libdsd/libdstdec/dst_init.c\
    libdsd/libdstdec/dst_init.h\
    libdsd/libdstdec/types.h\
    libdsd/libdstdec/unpack_dst.c\
    libdsd/libdstdec/unpack_dst.h


dsd_la_LDFLAGS = -module -avoid-version

dsd_la_LIBADD = $(LDADD)

AM_CFLAGS = $(CFLAGS) $(DSD_CFLAGS) -std=c99
endif
