#ifndef LIBDSDCALLBACKS_H_INCLUDED
#define LIBDSDCALLBACKS_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*DSDIOCallback_Read) (void *ptr, size_t size, size_t nmemb, void* client_data);

typedef size_t (*DSDIOCallback_Write) (const void *ptr, size_t size, size_t nmemb, void* client_data);

typedef int (*DSDIOCallback_Seek) (uint64_t offset, int whence, void* client_data);

typedef uint64_t (*DSDIOCallback_Tell) (void* client_data);

typedef int (*DSDIOCallback_Eof) (void* client_data);

typedef DSDIOLengthStatus (*DSDIOCallback_Length)(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);

typedef struct {
    DSDIOCallback_Read read;
    DSDIOCallback_Write write;
    DSDIOCallback_Seek seek;
    DSDIOCallback_Tell tell;
    DSDIOCallback_Eof eof;
    DSDIOCallback_Length length; 
} DSDIOCallbacks;

#ifdef __cplusplus
}
#endif

#endif