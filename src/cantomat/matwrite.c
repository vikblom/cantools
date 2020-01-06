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


// TODO: Use a "spec" struct instead. Also include function handle?
const char *FIELD_NAMES[6] = {"bus", "dbc", "msg", "signal", "time", "data"};
const enum matio_classes FIELD_CLASSES[] = {MAT_C_UINT8,
                                            MAT_C_CHAR,
                                            MAT_C_CHAR,
                                            MAT_C_CHAR,
                                            MAT_C_DOUBLE,
                                            MAT_C_DOUBLE};
const enum matio_types FIELD_TYPES[] = {MAT_T_UINT8,
                                        MAT_T_UTF8,
                                        MAT_T_UTF8,
                                        MAT_T_UTF8,
                                        MAT_T_DOUBLE,
                                        MAT_T_DOUBLE};
const int FIELD_OPTS[] = {0, 0, 0, 0, MAT_F_DONT_COPY_DATA, MAT_F_DONT_COPY_DATA};




void set_in_struct_array(matvar_t *struct_array,
                         int nth_field,
                         int array_index,
                         size_t size,
                         void *data)
{
    size_t dim[] = {1, size};
    matvar_t *var = Mat_VarCreate(FIELD_NAMES[nth_field],
                                  FIELD_CLASSES[nth_field],
                                  FIELD_TYPES[nth_field],
                                  2, dim, data,
                                  FIELD_OPTS[nth_field]);
    Mat_VarSetStructFieldByName(struct_array,
                                FIELD_NAMES[nth_field],
                                array_index,
                                var);
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
            set_in_struct_array(topstruct, 0, i, 1, &msg_key->bus);

            // DBC name
            set_in_struct_array(topstruct, 1, i,
                                strlen(msg->dbcname),
                                msg->dbcname);

            // Message name
            set_in_struct_array(topstruct, 2, i,
                                strlen(msg->name),
                                msg->name);

            // Signal name
            set_in_struct_array(topstruct, 3, i,
                                strlen(signame),
                                signame);

            // Time vector
            set_in_struct_array(topstruct, 4, i,
                                msg->n,
                                msg->time);

            // Data vector
            set_in_struct_array(topstruct, 5, i,
                                msg->n,
                                sigdata);

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
