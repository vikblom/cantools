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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "blfapi.h"
#include "blfbuffer.h"

#define BLHANDLE_MAGIC 0x01234567
#define BLFMIN(x,y) ((x)<(y)?(x):(y))


/* initialize SYSTEMTIME structure */
static void
blfSystemTimeInit(SYSTEMTIME *const s)
{
    s->wYear = 0;
    s->wMonth = 0;
    s->wDayOfWeek = 0;
    s->wDay = 0;
    s->wHour = 0;
    s->wMinute = 0;
    s->wSecond = 0;
    s->wMilliseconds = 0;
}


/* initialize VBLFileStatisticsEx structure */
static void
blfStatisticsInit(VBLFileStatisticsEx *const s)
{
    s->mStatisticsSize = 0x88u;
    s->mApplicationID = 0;
    s->mApplicationMajor = 0;
    s->mApplicationMinor = 0;
    s->mApplicationBuild = 0;
    s->mFileSize = 0;
    s->mUncompressedFileSize = 0;
    s->mObjectCount = 0;
    s->mObjectsRead = 0;

    blfSystemTimeInit(&s->mMeasurementStartTime);
    blfSystemTimeInit(&s->mLastObjectTime);

    assert(sizeof(s->mReserved) == 0x48);
    memset(s->mReserved, 0, sizeof(s->mReserved));
}


/* initialize LOGG structure */
static void
blfLOGGInit(LOGG l)
{
    l->mSignature = 0;
    l->mHeaderSize = 0;
    l->mCRC = 0;
    l->appID = 0;
    l->dwCompression = 0;
    l->appMajor = 0;
    l->appMinor = 0;
    l->fileSize = 0;
    l->uncompressedFileSize = 0;
    l->objectCount = 0;
    l->appBuild = 0;

    blfSystemTimeInit(&l->mMeasurementStartTime);
    blfSystemTimeInit(&l->mMeasurementEndTime);
}

/* debug: dump VBLObjectHeaderBase structure */
/*
static void
blfHeaderBaseDump(VBLObjectHeaderBase *b)
{
  printf("header.base.mSignature = %c%c%c%c\n",
  ((uint8_t *)&b->mSignature)[0],
  ((uint8_t *)&b->mSignature)[1],
  ((uint8_t *)&b->mSignature)[2],
  ((uint8_t *)&b->mSignature)[3]);
  printf("header.base.mHeaderSize    = %d\n",b->mHeaderSize);
  printf("header.base.mHeaderVersion = %d\n",b->mHeaderVersion);
}
*/


/* fill VBLFileStatisticsEx structure from LOGG structure */
static void
blfStatisticsFromLOGG(VBLFileStatisticsEx *const s, const LOGG_t *const l)
{
    s->mApplicationID        = l->appID;
    s->mApplicationMajor     = l->appMajor;
    s->mApplicationMinor     = l->appMinor;
    s->mFileSize             = l->fileSize;
    s->mUncompressedFileSize = l->uncompressedFileSize;
    s->mObjectCount          = l->objectCount;
    s->mApplicationBuild     = l->appBuild;
    s->mMeasurementStartTime = l->mMeasurementStartTime;
    s->mLastObjectTime       = l->mMeasurementEndTime;
}


/* diagnose structure sizes to ensure that we can fread into them */
static void
blfAssertStructures(void)
{
    assert(sizeof(VBLObjectHeaderBaseLOGG) == 32);
    assert(sizeof(LOGG_t) == 144);
    assert(sizeof(VBLObjectHeaderBase) == 16);
    assert(sizeof(VBLObjectHeader) == 32);
    assert(sizeof(VBLObjectHeaderBaseLOGG) == 32);
    assert(sizeof(VBLCANMessage) == 48);
    assert(sizeof(VBLFileStatisticsEx) == 136);
}


/* check, if BLFHANDLE is initialized */
static success_t
blfHandleIsInitialized(BLFHANDLE h)
{
    return (h!=NULL) && (h->magic == BLHANDLE_MAGIC);
}


/* copy VBLObjectHeaderBase */
static void
blfVBLObjectHeaderBaseCopy(VBLObjectHeaderBase *const dest,
                           const VBLObjectHeaderBase *const source)
{
    *dest = *source;
}


/* open a BLF file for reading */
BLFHANDLE
blfCreateFile(FILE *fp)
{
    blfAssertStructures();

    BLFHANDLE h = malloc(sizeof(*h));
    if (!h)
        goto fail;

    h->magic = BLHANDLE_MAGIC;
    h->mCANMessageFormat_v1 = 0;
    h->mPeekFlag = 0;

    blfLOGGInit(&h->mLOGG);
    blfStatisticsInit(&h->mStatistics);
    fread(&h->mLOGG, 1, 144, fp); // Overwrite with whats in the file.
    blfStatisticsFromLOGG(&h->mStatistics, &h->mLOGG);

    // TODO: We should peek and check that this is the start.
    if (!blfBufferCreate(&h->mBuffer, fp))
        goto fail;

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
    success_t success = blfBufferSkip(&h->mBuffer, pBase->mObjectSize);
    if (success)
        h->mStatistics.mObjectsRead++;
    return success;
}


/* read object header base of next object */
success_t
blfPeekObject(BLFHANDLE h, VBLObjectHeaderBase* pBase)
{
    if (!blfHandleIsInitialized(h) || pBase == NULL)
        return 0;
    return blfBufferPeek(&h->mBuffer, pBase, sizeof(*pBase));
}


/* read next object */
success_t
blfReadObject(BLFHANDLE h, VBLObjectHeaderBase *pBase)
{
    success_t success = blfBufferRead(&h->mBuffer, pBase, pBase->mObjectSize);
    if(success)
        h->mStatistics.mObjectsRead++;
    return success;
}


/* read next object into size-limited memory */
success_t
blfReadObjectSecure(BLFHANDLE h,
                    VBLObjectHeaderBase* pBase,
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
        *obj = *pBase;

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
            memset(pBase + pBase->mObjectSize, 0,
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
