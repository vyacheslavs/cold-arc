//
// Created by developer on 9/8/23.
//

#ifndef COLD_ARC_GTK_FILESTREAM_H
#define COLD_ARC_GTK_FILESTREAM_H

#include <stdint.h>

#define LIBISOFS_WITHOUT_LIBBURN yes
#include "libisofs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stream_callback_type {
    void* opaque;
    void (*on_open)(IsoStream*, void*);
    void (*on_close)(IsoStream*, void*);
    void (*on_read)(IsoStream*, void*);
    uint64_t offset;
    uint64_t total;
} stream_callback;

IsoStream* file_stream(const char* path, stream_callback* cb);

stream_callback* file_stream_get_callback(const IsoStream* stream);
const char* file_stream_get_filepath(const IsoStream* stream);
int file_stream_get_progress(const IsoStream* stream);

#ifdef __cplusplus
}
#endif
#endif //COLD_ARC_GTK_FILESTREAM_H
