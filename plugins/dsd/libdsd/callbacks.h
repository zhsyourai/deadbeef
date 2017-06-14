#ifndef LIBDSD_CALLBACKS_H_INCLUDED
#define LIBDSD_CALLBACKS_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DSD_IOHandle;

typedef size_t (*DSD_IOCallback_Read) (void *ptr, size_t size, size_t nmemb, DSD_IOHandle handle);

typedef size_t (*DSD_IOCallback_Write) (const void *ptr, size_t size, size_t nmemb, DSD_IOHandle handle);

typedef int (*DSD_IOCallback_Seek) (DSD_IOHandle handle, DSD_int64 offset, int whence);

typedef DSD_int64 (*DSD_IOCallback_Tell) (DSD_IOHandle handle);

typedef int (*DSD_IOCallback_Eof) (DSD_IOHandle handle);

typedef int (*DSD_IOCallback_Close) (DSD_IOHandle handle);

typedef struct {
    DSD_IOCallback_Read read;
    DSD_IOCallback_Write write;
    DSD_IOCallback_Seek seek;
    DSD_IOCallback_Tell tell;
    DSD_IOCallback_Eof eof;
    DSD_IOCallback_Close close;
} DSD_IOCallbacks;

#ifdef __cplusplus
}
#endif

#endif