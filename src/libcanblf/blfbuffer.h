#ifndef INCLUDE_BLFBUFFER_H
#define INCLUDE_BLFBUFFER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct BlfBuffer {
    FILE *source;
    unsigned char *buffer;
    size_t capacity;
    size_t position;
    size_t size;
};


int blfBufferCreate(struct BlfBuffer *buf, FILE *file);
void blfBufferDestroy(struct BlfBuffer *buf);
int blfBufferRead(struct BlfBuffer *buf, void *dest, size_t n);
int blfBufferPeek(struct BlfBuffer *buf, void *dest, size_t n);


#ifdef __cplusplus
}
#endif

#endif // INCLUDE_BLFBUFFER_H
