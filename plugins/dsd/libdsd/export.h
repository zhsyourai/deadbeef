#ifndef LIBDSD_EXPORT_H_INCLUDED
#define LIBDSD_EXPORT_H_INCLUDED

#if defined(DSD_NO_DLL)
#define DSD_API

#elif defined(_MSC_VER)
#ifdef DSD_API_EXPORTS
#define DSD_API __declspec(dllexport)
#else
#define DSD_API __declspec(dllimport)
#endif

#elif defined(DSD_USE_VISIBILITY_ATTR)
#define DSD_API __attribute__((visibility("default")))

#else
#define DSD_API

#endif

#define DSD_API_VERSION_CURRENT 0
#define DSD_API_VERSION_REVISION 0
#define DSD_API_VERSION_AGE 0

#endif