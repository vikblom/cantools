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
    return *in == '/' ? in+1 : in;
}


void set_in_struct(matvar_t *top,
                   int i,
                   char name[],
                   size_t size,
                   enum matio_classes class,
                   enum matio_types type,
                   void *data,
                   int opt)
{
    size_t dim[] = {1, size};
    matvar_t *var = Mat_VarCreate(name, class, type, 2, dim, data, opt);
    Mat_VarSetStructFieldByName(top, name, i, var);
}


/*
 * matWrite - write signals from measurement structure to MAT file
 */
int matWrite(struct hashtable *msg_hash, int count, const char *outFileName)
{
    /* loop over all time series */
    if (hashtable_count(msg_hash) == 0 || count == 0) {
        fprintf(stderr, "error: measurement empty, nothing to write\n");
        return 1;
    }

    mat_t *matfile = Mat_Create(outFileName, NULL);
    if (matfile == NULL) {
        fprintf(stderr, "error: could not create MAT file %s\n", outFileName);
        return 1;
    }

    const char *fieldnames[6] = {"bus", "dbc", "msg", "signal", "time", "data"};
    size_t structdim[2] = {1, count};
    matvar_t* topstruct = Mat_VarCreateStruct("data",
                                              2, structdim,
                                              fieldnames, 6);
    if (topstruct == NULL) {
        fprintf(stderr, "error: could not create MAT struct\n");
        return 1;
    }

    /* Iterator constructor only returns a valid iterator if
     * the hashtable is not empty */
    int i = 0;
    size_t dim[] = {1, 1};
    matvar_t *var;
    struct hashtable_itr *msg_itr = hashtable_iterator(msg_hash);
    do {
        frame_key_t *msg_key = hashtable_iterator_key(msg_itr);
        msg_series_t *msg = hashtable_iterator_value(msg_itr);

        if (!msg->ts_hash || hashtable_count(msg->ts_hash) == 0)
            continue;

        struct hashtable_itr *sig_itr = hashtable_iterator(msg->ts_hash);
        do {
            char *signame = hashtable_iterator_key(sig_itr);
            timeSeries_t *sigdata = hashtable_iterator_value(sig_itr);


            // Channel number
            dim[1] = 1;
            var = Mat_VarCreate(fieldnames[0],
                                MAT_C_UINT8, MAT_T_UINT8,
                                2, dim,
                                &msg_key->bus, 0);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[0], i, var);


            // DBC name
            dim[1] = strlen(msg->dbcname);
            var = Mat_VarCreate(fieldnames[1],
                                MAT_C_CHAR, MAT_T_UTF8,
                                2, dim,
                                msg->dbcname, 0);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[1], i, var);

            // Message name
            dim[1] = strlen(msg->name);
            var = Mat_VarCreate(fieldnames[2],
                                MAT_C_CHAR, MAT_T_UTF8,
                                2, dim,
                                msg->name, MAT_F_DONT_COPY_DATA);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[2], i, var);

            // Signal name
            dim[1] = strlen(signame);
            var = Mat_VarCreate(fieldnames[3],
                                MAT_C_CHAR, MAT_T_UTF8,
                                2, dim,
                                signame, 0);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[3], i, var);

            // Time vector
            dim[1] = msg->n;
            var = Mat_VarCreate(fieldnames[4],
                                MAT_C_DOUBLE, MAT_T_DOUBLE,
                                2, dim,
                                msg->time,
                                MAT_F_DONT_COPY_DATA);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[4], i, var);

            // Data vector
            var = Mat_VarCreate(fieldnames[5],
                                MAT_C_DOUBLE, MAT_T_DOUBLE,
                                2, dim,
                                sigdata,
                                MAT_F_DONT_COPY_DATA);
            Mat_VarSetStructFieldByName(topstruct, fieldnames[5], i, var);

            i++;

        } while (hashtable_iterator_advance(sig_itr));
        free(sig_itr);

    } while (hashtable_iterator_advance(msg_itr));
    free(msg_itr);

    Mat_VarWrite(matfile, topstruct, 0);
    Mat_VarFree(topstruct);
    Mat_Close(matfile);

    return 0;
}
