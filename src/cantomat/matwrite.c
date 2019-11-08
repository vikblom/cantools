/*  matWrite.c -- write out MAT files
    Copyright (C) 2007-2017 Andreas Heitmann

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

#if WITH_DMALLOC
#include <dmalloc.h>
#endif

#include <stdio.h>
#include <string.h>
#include <matio.h>
#include "measurement.h"
#include "hashtable_itr.h"


// Basically a custom version of strchr for periods.
const char *nextfield(const char *in, char *out)
{
    while (*in && *in != '/')
        *out++ = *in++;
    *out = '\0';
    return *in == '.' ? in+1 : in;
}


/*
 * matWrite - write signals from measurement structure to MAT file
 */
int matWrite(measurement_t *measurement, const char *outFileName)
{
    /* loop over all time series */
    struct hashtable *timeSeriesHash = measurement->timeSeriesHash;
    if (hashtable_count(timeSeriesHash) == 0) {
        fprintf(stderr, "error: measurement empty, nothing to write\n");
        return 1;
    }

    mat_t *matfile = Mat_Create(outFileName, NULL);
    if (matfile == NULL) {
        fprintf(stderr, "error: could not create MAT file %s\n", outFileName);
        return 1;
    }

    const char *fieldnames[5] = {"dbc", "msg", "signal", "time", "data"};
    size_t structdim[2] = {1, hashtable_count(timeSeriesHash)};
    matvar_t* topstruct = Mat_VarCreateStruct("data",
                                              2, structdim,
                                              fieldnames, 5);
    if (topstruct == NULL) {
        fprintf(stderr, "error: could not create MAT struct\n");
        return 1;
    }


    /* Iterator constructor only returns a valid iterator if
     * the hashtable is not empty */
    int i = 0;
    struct hashtable_itr *itr = hashtable_iterator(timeSeriesHash);
    do {
        char BUFFER[128];
        const char *signal_key = hashtable_iterator_key(itr);
        timeSeries_t *timeSeries = hashtable_iterator_value(itr);

        // Signal name
        signal_key = nextfield(signal_key, BUFFER);
        size_t sigdim[2] = {1, strlen(BUFFER)};
        matvar_t *sigvar = Mat_VarCreate(fieldnames[0],
                                         MAT_C_CHAR, MAT_T_UTF8,
                                         2, sigdim,
                                         BUFFER, 0);
        Mat_VarSetStructFieldByName(topstruct, fieldnames[0], i, sigvar);

        // Message name
        signal_key = nextfield(signal_key, BUFFER);
        size_t msgdim[2] = {1, strlen(BUFFER)};
        matvar_t *msgvar = Mat_VarCreate(fieldnames[1],
                                         MAT_C_CHAR, MAT_T_UTF8,
                                         2, msgdim,
                                         BUFFER, 0);
        Mat_VarSetStructFieldByName(topstruct, fieldnames[1], i, msgvar);

        // Database name
        signal_key = nextfield(signal_key, BUFFER);
        size_t dbcdim[2] = {1, strlen(BUFFER)};
        matvar_t *dbcvar = Mat_VarCreate(fieldnames[2],
                                         MAT_C_CHAR, MAT_T_UTF8,
                                         2, dbcdim,
                                         BUFFER, 0);
        Mat_VarSetStructFieldByName(topstruct, fieldnames[2], i, dbcvar);

        size_t dim[2] = {timeSeries->n, 1};
        // Time vector
        matvar_t *timevar = Mat_VarCreate(signal_key,
                                          MAT_C_DOUBLE, MAT_T_DOUBLE,
                                          2, dim,
                                          timeSeries->time,
                                          MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(topstruct, fieldnames[3], i, timevar);

        // Data vector
        matvar_t *datavar = Mat_VarCreate(signal_key,
                                          MAT_C_DOUBLE, MAT_T_DOUBLE,
                                          2, dim,
                                          timeSeries->value,
                                          MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(topstruct, fieldnames[4], i, datavar);

        i++;
    } while (hashtable_iterator_advance(itr));

    free(itr);

    Mat_VarWrite(matfile, topstruct, 0);
    Mat_VarFree(topstruct);
    Mat_Close(matfile);

    return 0;
}
