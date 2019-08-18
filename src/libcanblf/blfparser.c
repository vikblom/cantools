/*  blfparser.c --  parse BLF files
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <zlib.h>
#include <assert.h>

#include "blfparser.h"
#include "blfapi.h"

/* clear memory */
void
blfMemZero(uint8_t *mem, const size_t n)
{
    memset((char *)mem, 0, n);
}

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
void
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
  blfMemZero((uint8_t *)(s->mReserved),
            sizeof(s->mReserved));
}

/* initialize LOGG structure */
void
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
void
blfHeaderBaseDump(VBLObjectHeaderBase *b)
{
/*
  printf("header.base.mSignature = %c%c%c%c\n",
  ((uint8_t *)&b->mSignature)[0],
  ((uint8_t *)&b->mSignature)[1],
  ((uint8_t *)&b->mSignature)[2],
  ((uint8_t *)&b->mSignature)[3]);
  printf("header.base.mHeaderSize    = %d\n",b->mHeaderSize);
  printf("header.base.mHeaderVersion = %d\n",b->mHeaderVersion);
*/
}

/* fill VBLFileStatisticsEx structure from LOGG structure */
void
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

/* decoding on big endian machines requires byte swapping - not yet implemented */
void
blfAssertEndian(void)
{
#ifdef WORDS_BIGENDIAN
    perror("BLF decoding currently unsupported on big endian machines.");
    exit(EXIT_FAILURE);
#endif
}

/* diagnose structure sizes to ensure that we can fread into them */
void
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
success_t
blfHandleIsInitialized(BLFHANDLE h)
{
    return (h!=NULL) && (h->magic == BLHANDLE_MAGIC);
}

/* copy VBLObjectHeaderBase */
void
blfVBLObjectHeaderBaseCopy(VBLObjectHeaderBase *const dest,
                           const VBLObjectHeaderBase *const source)
{
    *dest = *source;
}
