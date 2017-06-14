#ifndef LIBDSD_EXPORT_H_INCLUDED
#define LIBDSD_EXPORT_H_INCLUDED

#if defined(DSS_NO_DLL)
#define DSS_API

#elif defined(_MSC_VER)
#ifdef DSS_API_EXPORTS
#define DSS_API __declspec(dllexport)
#else
#define DSS_API __declspec(dllimport)
#endif

#elif defined(DSS_USE_VISIBILITY_ATTR)
#define DSS_API __attribute__((visibility("default")))

#else
#define DSS_API

#endif

#define DSS_API_VERSION_CURRENT 0
#define DSS_API_VERSION_REVISION 0
#define DSS_API_VERSION_AGE 0

#endif