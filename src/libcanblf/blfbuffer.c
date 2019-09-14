#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#include "blfapi.h"
#include "blfbuffer.h"

static int fpeek(FILE* fp, void *dest, size_t bytes)
{
    long before = ftell(fp);
    if (before == -1)
        return 0;

    size_t ret = fread(dest, 1, bytes, fp);

    if (fseek(fp, before, SEEK_SET) != 0)
        return 0;

    return ret == bytes;
}


static int isLobjNext(FILE *fp)
{
    char tmp[4];
    fpeek(fp, &tmp, 4);
    return tmp[0] == 'L' || tmp[1] == 'O' || tmp[2] == 'B' || tmp[3] == 'J';
}


// Moves source past a log container header.
static int readLogHead(FILE *source, VBLObjectHeaderBaseLOGG *logp)
{
    if (!fpeek(source, logp, sizeof(*logp))) {
        return 0;
    }
    if (logp->base.mObjectType != BL_OBJ_TYPE_LOG_CONTAINER) {
        fprintf(stderr, "Next item is not a container. Cannot add more data.\n");
        return 0;
    }
    return fseek(source, sizeof(*logp), SEEK_CUR) == 0;
}


// Increase buffer capacity to fit at least current size + n_incoming
static int blfBufferRealloc(BlfBuffer *buf, size_t n_incoming)
{
    size_t n_free = buf->capacity - (buf->position + buf->size);
    if (n_incoming <= n_free) {
        return 1;
    }

    // Shift down to start
    if (buf->position) {
        size_t i;
        for (i = 0; i < buf->size; ++i) {
            buf->buffer[i] = buf->buffer[buf->position + i];
        }
        buf->position = 0;
    }

    if (n_incoming <= buf->capacity - buf->size) {
        return 1;
    }

    buf->buffer = realloc(buf->buffer, buf->size + n_incoming);
    if (!buf->buffer) {
        fprintf(stderr, "Reallocating buffer failed.\n");
        return 0;
    }
    buf->capacity = buf->size + n_incoming;
    return 1;
}


// Unzip data into buffer, increases buffer size.
// The buffer must have capacity for all the new data.
static size_t blfBufferUnzip(BlfBuffer *buf,
                             unsigned char *zip_data,
                             size_t zipd_size)
{
    z_stream stream;
    stream.state = Z_NULL;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.total_out = 0;

    stream.next_in = zip_data;
    stream.avail_in = zipd_size;

    stream.next_out = buf->buffer + (buf->position + buf->size);
    stream.avail_out = buf->capacity - (buf->position + buf->size);

    if (inflateInit(&stream) != Z_OK) {
        fprintf(stderr, "Zlib init failed.\n");
        return 0;
    }
    if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Zlib could not complete inflate.\n");
        inflateEnd(&stream);
        return 0;
    }

    inflateEnd(&stream);
    buf->size += stream.total_out;
    return stream.total_out;
}


// Adds more data to buffer from source file, increasing size and maybe capacity.
static int blfBufferRefill(BlfBuffer *buf)
{
    // Check whats coming and make sure there is room for it
    VBLObjectHeaderBaseLOGG log;
    if (!readLogHead(buf->source, &log)) {
        //fprintf(stderr, "readLogHead failed.\n");
        return 0;
    }
    size_t raw_size = log.base.mObjectSize - sizeof(VBLObjectHeaderBaseLOGG);
    size_t new_size = log.deflatebuffersize;
    if (!blfBufferRealloc(buf, new_size)) {
        fprintf(stderr, "Buffer realloc failed.\n");
        return 0;
    }


    unsigned char *data;
    if (log.compressedflag == 2) {
        data = malloc(raw_size);
        if (!data) {
            fprintf(stderr, "Allocating zipdata failed.\n");
            return 0;
        }
    } else {
        data = buf->buffer + (buf->position + buf->size);
    }

    size_t read = fread(data, 1, raw_size, buf->source);
    if (read < raw_size) {
        fprintf(stderr, "Reading from file failed.\n");
        return 0;
    }

    if (log.compressedflag == 2) {
        size_t added = blfBufferUnzip(buf, data, raw_size);
        free(data);
        assert(added == new_size); // Just checking...
    } else {
        buf->size += raw_size;
    }

    // Cleanup
    return fseek(buf->source, raw_size % 4, SEEK_CUR) == 0;
}


int blfBufferPeek(BlfBuffer *buf, void *dest, size_t n)
{
    while (buf->size < n) {
        if (!blfBufferRefill(buf)) {
            return 0;
        }
    }
    memcpy(dest, buf->buffer + buf->position, n);
    return 1;
}

int blfBufferSkip(BlfBuffer *buf, size_t n)
{
    n += n % 4; // Include alignment padding
    while (buf->size < n) {
        if (!blfBufferRefill(buf)) {
            fprintf(stderr, "Refill failed\n");
            return 0;
        }
    }
    buf->position += n;
    buf->size -= n;
    return 1;
}


int blfBufferRead(BlfBuffer *buf, void *dest, size_t n)
{
    if (!blfBufferPeek(buf, dest, n))
        return 0;
    blfBufferSkip(buf, n);
    return 1;
}


int blfBufferCreate(BlfBuffer *buf, FILE *file)
{
    if (!file) {
        fprintf(stderr, "Cannot buffer NULL\n");
        return 0;
    }
    // TODO: If we dont start at an object, seek until we get one?
    if (!isLobjNext(file))
        return 0;

    buf->source = file;
    buf->buffer = malloc(1024);
    if (!buf->buffer) {
        fprintf(stderr, "Allocating BlfBuffer failed.\n");
        return 0;
    }
    buf->capacity = 1024;
    buf->position = 0;
    buf->size = 0;
    return 1;
}


void blfBufferDestroy(BlfBuffer *buf)
{
    free(buf->buffer);
    return;
}


#ifdef BLF_BUFFER_MAIN
// For debug purposes
static void blfBufferDump(BlfBuffer *buf)
{
    printf("Position: %zu\t\tSize: %zu\t\tCapacity: %zu\n",
           buf->position, buf->size, buf->capacity);
}


// Lists how many objects of each kind (id < 256) there are in the file.
// Useful for sanity checking after changes.
static void summarizeBlf(FILE *fp)
{
    BlfBuffer buf;
    blfBufferCreate(&buf, fp);

    size_t total = 0;
    size_t counts[256] = {0};
    VBLObjectHeaderBase base;
    while (blfBufferPeek(&buf, &base, sizeof(base))) {
        if (!blfBufferSkip(&buf, base.mObjectSize))
            break;
        //printf("Object id %d\n", base.mObjectType);
        counts[base.mObjectType < 256 ? base.mObjectType : 0]++;
        total++;
    }

    int i;
    for (i = 0; i < 256; ++i) {
        if (counts[i])
            printf("%6d %zu\n", i, counts[i]);
    }
    printf("Total: %zu\n", total);
    blfBufferDestroy(&buf);
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: blfbuf file.blf\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }
    // Skip the header
    char tmp[144];
    fread(tmp, 1, 144, fp);

    summarizeBlf(fp);

    fclose(fp);
    return 0;
}
#endif // BLF_BUFFER_MAIN
