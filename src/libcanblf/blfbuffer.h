#ifndef INCLUDE_BLFBUFFER_H
#define INCLUDE_BLFBUFFER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FILE *source;
    unsigned char *buffer;
    size_t capacity;
    size_t position;
    size_t size;
} BlfBuffer;


int blfBufferCreate(BlfBuffer *buf, FILE *file);
void blfBufferDestroy(BlfBuffer *buf);
int blfBufferRead(BlfBuffer *buf, void *dest, size_t n);
int blfBufferPeek(BlfBuffer *buf, void *dest, size_t n);
int blfBufferSkip(BlfBuffer *buf, size_t n);


#ifdef __cplusplus
}
#endif

#endif // INCLUDE_BLFBUFFER_H
