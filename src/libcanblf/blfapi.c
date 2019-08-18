/*  blfapi.c -- API for BLF access
    Copyright (C) 2016-2017 Andreas Heitmann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "cantools_config.h"

#include <stdlib.h>
#include <string.h>

#include "blfapi.h"
#include "blfparser.c"
#include "blfbuffer.h"

#define BLFMIN(x,y) ((x)<(y)?(x):(y))

/* open a BLF file for reading */
BLFHANDLE
blfCreateFile(FILE *fp)
{
    blfAssertEndian();
    blfAssertStructures();

    BLFHANDLE h = malloc(sizeof(*h));
    if (h == NULL)
        goto fail;

    h->magic = BLHANDLE_MAGIC;
    h->mCANMessageFormat_v1 = 0;
    h->mPeekFlag = 0;

    blfLOGGInit(&h->mLOGG);
    fread(&h->mLOGG, 1, 144, fp); // Overwrite with whats in the file.
    blfStatisticsFromLOGG(&h->mStatistics, &h->mLOGG);

    // TODO: We should peek and check that this is the start.
    blfBufferCreate(&h->mBuffer, fp);
    return h;

fail:
    if(h != NULL) free(h);
    fprintf(stderr,"blfCreateFile() failed\n");
    return NULL;
}

/* close BLFHANDLE */
success_t
blfCloseHandle(BLFHANDLE h)
{
    if (!blfHandleIsInitialized(h))
        return 0;
    blfBufferDestroy(&h->mBuffer);
    free(h);
    return 1;
}

/* retrieve VBLFileStatisticsEx data */
success_t
blfGetFileStatisticsEx(BLFHANDLE h, VBLFileStatisticsEx* pStatistics)
{
    unsigned int nBytes;

    if (!blfHandleIsInitialized(h)) goto fail;
    if (pStatistics == NULL) goto fail;

    nBytes = BLFMIN(h->mStatistics.mStatisticsSize,
                    pStatistics->mStatisticsSize);
    memcpy(pStatistics, &h->mStatistics, nBytes);
    pStatistics->mStatisticsSize = nBytes;
    return 1;

fail:
    return 0;
}

/* skip next object */
success_t
blfSkipObject(BLFHANDLE h, VBLObjectHeaderBase* pBase)
{
    if (!blfHandleIsInitialized(h) || pBase == NULL)
        return 0;
    success_t success = !blfBufferSkip(&h->mBuffer, pBase->mObjectSize);
    if (success) {
        h->mStatistics.mObjectsRead++;
    }
    return success;
}

/* read object header base of next object */
success_t
blfPeekObject(BLFHANDLE h, VBLObjectHeaderBase* pBase)
{
    if (!blfHandleIsInitialized(h) || pBase == NULL)
        return 0;
    return !blfBufferPeek(&h->mBuffer, pBase, sizeof(*pBase));
    // TODO: Sanity check the signature?
}

/* read next object */
success_t
blfReadObject(BLFHANDLE h, VBLObjectHeaderBase *pBase)
{
    success_t success = !blfBufferRead(&h->mBuffer, pBase, pBase->mObjectSize);
    if(success) {
        h->mStatistics.mObjectsRead++;
    }
    return success;
}

/* read next object into size-limited memory */
success_t
blfReadObjectSecure(BLFHANDLE h, VBLObjectHeaderBase* pBase,
                    size_t expectedSize)
{
    if (!blfHandleIsInitialized(h) || pBase == NULL)
        return 0;

    if(pBase->mObjectSize > expectedSize) {
        /* allocate additional memory to read from handle */
        VBLObjectHeaderBase *obj = malloc(pBase->mObjectSize);
        if (!obj) {
            fprintf(stderr, "Allocation of VBLObjectHeaderBase "
                    "with size %u failed.",(uint32_t)pBase->mObjectSize);
            return 0;
        }
        blfVBLObjectHeaderBaseCopy(obj, pBase);
        if(!blfReadObject(h, obj)) {
            free(obj);
            return 0;
        }

        /* copy expected bytes back to pBase */
        memcpy(pBase, obj, expectedSize);
        free(obj);
    } else {
        if(!blfReadObject(h, pBase))
            return 0;

        /* less bytes read than expected? -> clear remaining bytes */
        if (pBase->mObjectSize < expectedSize) {
            blfMemZero((uint8_t *)pBase + pBase->mObjectSize,
                       expectedSize - (size_t)pBase->mObjectSize);
        }
    }
    return 1;
}

/* free object data */
success_t
blfFreeObject(BLFHANDLE h, VBLObjectHeaderBase* pBase)
{
    return 1; // No dynamically sized objects implemented?
}
