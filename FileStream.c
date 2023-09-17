//
// Created by developer on 9/8/23.
//

#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "FileStream.h"

#include <stdio.h>

#ifndef MIN
#   define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define ISO_FSTREAM_FS_ID          6
ino_t file_stream_serial_id = (ino_t)1;

typedef struct FileStreamPrivateType {
    stream_callback* cb;
    const char* path;
    int fd;
    void* mmap_base;
    size_t mmap_size;
    ssize_t offset;
    ino_t ino_id;
} FileStreamPrivate;

static int file_stream_open(IsoStream *stream);
static int file_stream_close(IsoStream *stream);
static off_t file_stream_get_size(IsoStream *stream);
static int file_stream_read(IsoStream *stream, void *buf, size_t count);
static int file_stream_is_repeatable(IsoStream *stream);
static void file_stream_get_id(IsoStream *stream, unsigned int *fs_id, dev_t *dev_id, ino_t *ino_id);
void file_stream_free(IsoStream *stream);
static int file_stream_update_size(IsoStream *stream);
static IsoStream* file_stream_get_input_stream(IsoStream *stream, int flag);
static int file_stream_clone_stream(IsoStream *old_stream, IsoStream **new_stream, int flag);

static int file_stream_open(IsoStream *stream) {
    if (stream == NULL) {
        return ISO_NULL_POINTER;
    }
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    data->fd = open(data->path, O_RDONLY);
    if (data->fd < 0)
        return ISO_FILE_DOESNT_EXIST;
    data->mmap_base = mmap(0, data->mmap_size, PROT_READ, MAP_PRIVATE, data->fd, 0);
    if (data->mmap_base == MAP_FAILED)
        goto error;

    data->offset = 0;
    (void) madvise(data->mmap_base, data->mmap_size, MADV_SEQUENTIAL);

    if (data->cb && data->cb->on_open)
        data->cb->on_open(stream, data->cb->opaque);
    return ISO_SUCCESS;
error:
    if (data->fd >= 0) {
        close(data->fd);
        data->fd = -1;
    }
    data->mmap_base = NULL;
    return ISO_FILE_NOT_OPENNED;
}

static int file_stream_close(IsoStream *stream) {
    if (stream == NULL) {
        return ISO_NULL_POINTER;
    }
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;

    if (data->offset == -1) {
        return ISO_FILE_NOT_OPENED;
    }

    (void) madvise(data->mmap_base, data->mmap_size, MADV_DONTNEED);
    (void) munmap(data->mmap_base, data->mmap_size);
    close(data->fd);

    data->fd = -1;
    data->mmap_base = NULL;
    data->offset = -1;
    if (data->cb && data->cb->on_close)
        data->cb->on_close(stream, data->cb->opaque);
    return ISO_SUCCESS;
}

static off_t file_stream_get_size(IsoStream *stream) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    return (off_t) data->mmap_size;
}

static int file_stream_read(IsoStream *stream, void *buf, size_t count) {
    if (stream == NULL || buf == NULL) {
        return ISO_NULL_POINTER;
    }
    if (count == 0) {
        return ISO_WRONG_ARG_VALUE;
    }
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;

    if (data->offset == -1) {
        return ISO_FILE_NOT_OPENED;
    }
    if (data->offset >= (ssize_t) data->mmap_size) {
        return 0; /* EOF */
    }
    size_t len = MIN(count, data->mmap_size - data->offset);
    memcpy(buf, data->mmap_base + data->offset, len);
    data->offset += (ssize_t)len;
    if (data->cb && data->cb->on_read)
        data->cb->on_read(stream, data->cb->opaque);
    return (int)len;
}

static int file_stream_is_repeatable(IsoStream *stream) {
    return 1;
}

static void file_stream_get_id(IsoStream *stream, unsigned int *fs_id, dev_t *dev_id, ino_t *ino_id) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    *fs_id = ISO_FSTREAM_FS_ID;
    *dev_id = 0;
    *ino_id = data->ino_id;
}

void file_stream_free(IsoStream *stream) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    free((void*)data->path);
    free(data);
}

static int file_stream_update_size(IsoStream *stream) {
    return ISO_SUCCESS;
}

static IsoStream* file_stream_get_input_stream(IsoStream *stream, int flag) {
    return NULL;
}

static int file_stream_clone_stream(IsoStream *old_stream, IsoStream **new_stream, int flag) {
    if (flag)
        return ISO_STREAM_NO_CLONE; /* unknown option required */
    *new_stream = NULL;
    IsoStream *stream = calloc(1, sizeof(IsoStream));
    if (stream == NULL)
        return ISO_OUT_OF_MEM;
    stream->refcount = 1;
    stream->class = old_stream->class;
    FileStreamPrivate* new_data = calloc(1, sizeof(FileStreamPrivate));
    if (!new_data) {
        free(stream);
        return ISO_OUT_OF_MEM;
    }
    FileStreamPrivate* data = (FileStreamPrivate* )old_stream->data;
    new_data->fd = -1;
    new_data->mmap_base = NULL;
    new_data->cb = data->cb;
    new_data->mmap_size = data->mmap_size;
    new_data->offset = -1;
    new_data->ino_id = file_stream_serial_id++;
    new_data->path = strdup(data->path);
    if (!new_data->path) {
        free(new_data);
        free(stream);
        return ISO_OUT_OF_MEM;
    }
    stream->data = new_data;
    *new_stream = stream;
    return ISO_SUCCESS;
}
static
IsoStreamIface file_stream_class = {
    4, /* version */
    "fs ",
    file_stream_open,
    file_stream_close,
    file_stream_get_size,
    file_stream_read,
    file_stream_is_repeatable,
    file_stream_get_id,
    file_stream_free,
    file_stream_update_size,
    file_stream_get_input_stream,
    NULL,
    file_stream_clone_stream
};

IsoStream* file_stream(const char* path, stream_callback* cb) {
    if (!path)
        return NULL;

    IsoStream* str = (IsoStream*) malloc (sizeof(IsoStream));
    if (!str)
        return NULL;

    FileStreamPrivate* data = (FileStreamPrivate*) malloc(sizeof(FileStreamPrivate));
    if (!data) {
        free(str);
        return NULL;
    }

    data->path = strdup(path);
    if (!data->path) {
        free(data);
        free(str);
        return NULL;
    }
    struct stat st;
    if (stat(data->path, &st) < 0) {
        free(data);
        free(str);
        return NULL;
    }
    data->mmap_size = st.st_size;

    data->fd = -1;
    data->mmap_base = NULL;
    data->cb = cb;
    data->offset = -1;
    data->ino_id = file_stream_serial_id++;

    str->refcount = 1;
    str->data = data;
    str->class = &file_stream_class;

    return str;
}
const char* file_stream_get_filepath(const IsoStream* stream) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    return data->path;
}

stream_callback* file_stream_get_callback(const IsoStream* stream) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;
    return data->cb;
}
int file_stream_get_progress(const IsoStream* stream) {
    FileStreamPrivate* data = (FileStreamPrivate*) stream->data;

    if (data->offset<0)
        return 100;

    if (data->mmap_size == 0)
        return 100;

    return (int)(100 * data->offset / data->mmap_size);
}
