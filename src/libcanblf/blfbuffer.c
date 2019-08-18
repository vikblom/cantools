#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#include "blfbuffer.h"
#include "blfapi.h"


static int fpeek(FILE* fp, void *dest, size_t bytes)
{
    long before = ftell(fp);
    size_t ret = fread(dest, bytes, 1, fp);
    fseek(fp, before, SEEK_SET);
    return ret;
}


static void assertLobjNext(FILE *fp)
{
    char tmp[4];
    fpeek(fp, &tmp, 4);
    assert(tmp[0] == 'L' || tmp[1] == 'O' || tmp[2] == 'B' || tmp[3] == 'J');
}


static int skipToPayload(FILE *source, size_t *zipd_size, size_t *unzipd_size)
{
    VBLObjectHeaderBaseLOGG log;
    if (!fpeek(source, &log, sizeof(log))) {
        return 0;
    }
    assert(log.base.mObjectType == BL_OBJ_TYPE_LOG_CONTAINER);

    fseek(source, sizeof(VBLObjectHeaderBaseLOGG), SEEK_CUR);
    *zipd_size = log.base.mObjectSize - sizeof(VBLObjectHeaderBaseLOGG);
    *unzipd_size = log.deflatebuffersize;
    return 1;
}


static void blfBufferPrint(struct BlfBuffer *buf)
{
    printf("Position: %zu\t\tSize: %zu\t\tCapacity: %zu\n",
           buf->position, buf->size, buf->capacity);
}


// Manipulates buffer so that n_incoming bytes can be appended.
static int blfBufferRealloc(struct BlfBuffer *buf, size_t n_incoming)
{
    size_t n_free = buf->capacity - (buf->position + buf->size);
    if (n_incoming <= n_free) {
        return 0;
    }

    if (buf->position) {
        size_t i;
        for (i = 0; i < buf->size; ++i) {
            buf->buffer[i] = buf->buffer[buf->position + i];
        }
        buf->position = 0;

        if (n_incoming <= buf->capacity - buf->size) {
            return 0;
        }
    }

    buf->buffer = realloc(buf->buffer, buf->size + n_incoming);
    if (!buf->buffer) {
        fprintf(stderr, "Reallocating buffer failed.\n");
        return 0;
    }
    buf->capacity = buf->size + n_incoming;
    return 1;
}


// Unzip data into buffer
// Fails if buffer is not big enough for inflated data.
static size_t blfBufferUnzip(struct BlfBuffer *buf,
                             unsigned char *zip_data, size_t zipd_size)
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


// Unzip the next LOBJ into buffer
static int blfBufferRefill(struct BlfBuffer *buf)
{
    // Check whats coming
    size_t zipd_size, unzipd_size;
    if (!skipToPayload(buf->source, &zipd_size, &unzipd_size)) {
        fprintf(stderr, "Next item is not a container. Cannot add more data.\n");
        return 1;
    }

    // Actually read the compressed data into memory
    unsigned char *zipd_data = malloc(zipd_size);
    if (!zipd_data) {
        fprintf(stderr, "Allocating zipdata failed.\n");
        return 1;
    }
    fread(zipd_data, 1, zipd_size, buf->source);

    // Append unzipd data
    blfBufferRealloc(buf, unzipd_size);
    size_t added = blfBufferUnzip(buf, zipd_data, zipd_size);
    assert(added == unzipd_size); // Just checking...

    // Cleanup
    free(zipd_data);
    fseek(buf->source, zipd_size % 4, SEEK_CUR);
    return 0;
}


int blfBufferPeek(struct BlfBuffer *buf, void *dest, size_t n)
{
    if (buf->size < n && blfBufferRefill(buf)) {
        fprintf(stderr, "Refill failed\n");
        return 1;
    }

    memcpy(dest, buf->buffer + buf->position, n);
    return 0;
}

int blfBufferSkip(struct BlfBuffer *buf, size_t n)
{
    n += n % 4; // Include alignment padding
    if (buf->size < n && blfBufferRefill(buf)) {
        fprintf(stderr, "Refill failed\n");
        return 1;
    }
    buf->position += n;
    buf->size -= n;
    return 0;
}


int blfBufferRead(struct BlfBuffer *buf, void *dest, size_t n)
{
    if (blfBufferPeek(buf, dest, n)) {
        return 1;
    }
    blfBufferSkip(buf, n);
    return 0;
}


int blfBufferCreate(struct BlfBuffer *buf, FILE *file)
{
    if (!file) {
        fprintf(stderr, "Cannot buffer NULL\n");
        return 0;
    }
    // TODO: If we dont start at an object, seek until we get one?
    assertLobjNext(file);

    buf->source = file;
    buf->buffer = malloc(1024);
    if (!buf->buffer) {
        fprintf(stderr, "Allocating BlfBuffer failed.\n");
    }
    buf->capacity = 1024;
    buf->position = 0;
    buf->size = 0;
    return 1;
}


void blfBufferDestroy(struct BlfBuffer *buf)
{
    free(buf->buffer);
    return;
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

    struct BlfBuffer buf;
    blfBufferCreate(&buf, fp);

    size_t count = 0;
    VBLObjectHeaderBase base;
    while (1) {
        if (blfBufferPeek(&buf, &base, sizeof(base)))
            break;
        if (blfBufferSkip(&buf, base.mObjectSize))
            break;
        count++;
    }

    printf("Done! Read %ld objects\n", count);
    blfBufferPrint(&buf);

    blfBufferDestroy(&buf);

    fclose(fp);
    return 0;
}
