/*  busAssignment.c -- assign bus id to DBC file
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

#include <stdio.h>
#include <string.h>
#include "busassignment.h"
#include "messagehash.h"
#include "dbcmodel.h"

struct busAssignmentEntry_s {
    int bus;
    char *filename;
    char *basename;
    messageHash_t *messageHash;
};

struct busAssignment_s {
    int n;
    struct busAssignmentEntry_s *list; /* array of n busAssigmentEntry_t's */
};

extern int verbose_flag;


char *basename(char *path)
{
    int i = 0;
    int first = 0;
    int stop = 0;

    while (path[i++]) {
        if (path[i] == '/' || path[i] == '\\') {
            first = i+1;
        } else if (path[i] == '.') {
            stop = i;
        }
    }
    if (stop == 0)
        stop = i-1; // index of \0

    int len = stop - first;
    char *out = malloc(len+1);
    for (i = 0; i < len; i++) {
        out[i] = path[first+i];
    }
    out[len] = '\0';

    return out;
}


busAssignment_t *busAssignment_create(void)
{
    CREATE(busAssignment_t, busAssignment);

    busAssignment->n = 0;
    busAssignment->list = NULL;
    return busAssignment;
}

void busAssignment_associate(busAssignment_t *busAssignment,
                             int bus, char *filename)
{
    busAssignment->n++;
    busAssignment->list = (busAssignmentEntry_t *)
        realloc(busAssignment->list,
                busAssignment->n * sizeof(*(busAssignment->list)));
    busAssignment->list[busAssignment->n-1].bus = bus;
    busAssignment->list[busAssignment->n-1].filename = strdup(filename);
    busAssignment->list[busAssignment->n-1].basename = basename(filename);
    busAssignment->list[busAssignment->n-1].messageHash = NULL;
}

int busAssignment_parseDBC(busAssignment_t *busAssignment)
{
    int i;
    int ret = 0;

    for(i = 0; i < busAssignment->n; i++) {
        dbc_t *dbc;

        if(verbose_flag) {
            fprintf(stderr, "Parsing DBC file %s\n", busAssignment->list[i].filename);
        }
        if(NULL != (dbc = dbc_read_file(busAssignment->list[i].filename))) {
            messageHash_t *messageHash = messageHash_create(dbc->message_list);

            // busAssignment->list[i].databaseName = NULL;
            busAssignment->list[i].messageHash = messageHash;
            if(messageHash == NULL) {
                fprintf(stderr,
                        "busAssignment_parseDBC(): error parsing DBC file %s\n",
                        busAssignment->list[i].filename);
                ret = 1;
                break;
            }
            dbc_free(dbc);
        } else {
            fprintf(stderr, "busAssignment_parseDBC(): error opening DBC file %s\n",
                    busAssignment->list[i].filename);
            ret = 1;
            break;
        }
    }
    return ret;
}

void busAssignment_free(busAssignment_t *busAssignment)
{
    int i;

    if(busAssignment != NULL) {
        for(i = 0; i < busAssignment->n; i++) {
            busAssignmentEntry_t *entry = &(busAssignment->list[i]);
            free(entry->filename);
            free(entry->basename);
            messageHash_free(entry->messageHash);
        }
        if(busAssignment->list != NULL) free(busAssignment->list);
    }
    free(busAssignment);
}

/**
 * @brief      Find message spec. in bus library.
 *
 * @details    Finds the first match of ID in the parsed bus libraries.
 *             Only buses matching argument bus as used.
 *             bus=-1 means match all buses
 *             Writes the basename of the used dbc into basename_used.
 *             Returns NULL if no match was found.
 */
message_t *get_msg_spec(busAssignment_t *bus_lib,
                        int id,
                        int bus,
                        char **basename_used)
{
    message_t *match = NULL;
    for (int i = 0; i < bus_lib->n ; i++) {
        busAssignmentEntry_t entry = bus_lib->list[i];
        if (entry.bus == bus) {
            if ((match = hashtable_search(entry.messageHash, &id))) {
                *basename_used = entry.basename;
                return match;
            }
        }
    }

    return NULL;
}
