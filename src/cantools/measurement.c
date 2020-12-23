/*  measurement.c -- process CAN trace file
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
#include <assert.h>
#include "measurement.h"
#include "busassignment.h"
#include "messagehash.h"
#include "hashtable.h"
#include "hashtable_itr.h"
#include "messagedecoder.h"
#include "dbcmodel.h"


/* simple string hash function for signal names */
static unsigned int string_hash(void *k)
{
    unsigned int hash = 0;
    int c;
    while ((c = *(unsigned char *)k++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}


/* string comparison function for signal names */
static int string_equal(void *key1, void *key2 )
{
    return strcmp((char *)key1, (char *)key2) == 0;
}


static unsigned int msg_hash(void *this)
{
    frame_key_t *frame_key_p = (frame_key_t *) this;
    return frame_key_p->id;
}


static int msg_equal(void *this, void *that)
{
    frame_key_t *this_p = (frame_key_t *) this;
    frame_key_t *that_p = (frame_key_t *) that;

    return this_p->id == that_p->id && this_p->bus == that_p->bus;
}


/*
 * callback function for processing a CAN message
 */
static void canframe_callback(canMessage_t *canMessage, void *cb_data)
{
    struct hashtable *msg_hashmap = (struct hashtable *) cb_data;

    /* look for signal in time series hash */
    frame_key_t frame_key = {canMessage->id, canMessage->bus};
    msg_series_t *msg_series_p = hashtable_search(msg_hashmap,
                                                  (void *) &frame_key);
    if (!msg_series_p) {
        frame_key_t *frame_key_p = malloc(sizeof(frame_key_t));
        frame_key_p->id = canMessage->id;
        frame_key_p->bus = canMessage->bus;

        msg_series_p = malloc(sizeof(msg_series_t));
        msg_series_p->n = 0;
        msg_series_p->cap = 0;
        msg_series_p->data = NULL;
        msg_series_p->time = NULL;
        msg_series_p->dlc = canMessage->dlc;
        msg_series_p->name = NULL;
        msg_series_p->dbcname = NULL;
        msg_series_p->ts_hash = NULL;

        hashtable_insert(msg_hashmap,
                         (void *) frame_key_p,
                         (void *) msg_series_p);
    }

    if (msg_series_p->dlc != canMessage->dlc) {
        fprintf(stderr, "DLC MISMATCH!\n");
        return;
    }

    if (msg_series_p->n == msg_series_p->cap) {
        msg_series_p->cap += 1024;
        msg_series_p->data = realloc(msg_series_p->data,
                                     msg_series_p->dlc * msg_series_p->cap);
        msg_series_p->time = realloc(msg_series_p->time,
                                     sizeof(double) * msg_series_p->cap);
    }

    memcpy(msg_series_p->data + msg_series_p->n * msg_series_p->dlc,
           canMessage->byte_arr, msg_series_p->dlc);

    uint64_t sec = canMessage->t.tv_sec;
    int64_t nsec = canMessage->t.tv_nsec;
    msg_series_p->time[msg_series_p->n] = sec + nsec * 1e-9;

    msg_series_p->n++;
}


/*
 * process CAN trace file with given input parser
 *
 * If filename is NULL, uses stdin instead.
 */
struct hashtable *read_messages(const char *filename,
                                parserFunction_t parserFunction)
{
    /* open input file */
    FILE *fp = filename ? fopen(filename, "rb") : stdin;
    if (!fp) {
        fprintf(stderr, "Opening input file failed.\n");
        return NULL;
    }

    // TODO: One hashmap for each channel to avoid collisions
    struct hashtable *msg_hashmap = create_hashtable(16, msg_hash, msg_equal);

    /*
     * Invoke the file format parser on file pointer fp.
     * This function will call the passed cb for all msgs.
     * The parser function is responsible for closing the input
     * file stream
     * One of: blfReader_processFile or friends...
     */
    parserFunction(fp, canframe_callback, msg_hashmap);

    if (filename != NULL)
        fclose(fp);
    return msg_hashmap;
}


void destroy_messages(struct hashtable *msg_hashmap)
{
    if (!msg_hashmap)
        return;

    if (hashtable_count(msg_hashmap)) {
        struct hashtable_itr *itr = hashtable_iterator(msg_hashmap);
        do {
            msg_series_t *msg = hashtable_iterator_value(itr);
            free(msg->time);
            free(msg->data);

            if (msg->ts_hash)
                hashtable_destroy(msg->ts_hash, 1);

            free(msg);
        } while (hashtable_iterator_advance(itr));
        free(itr);
    }
    hashtable_destroy(msg_hashmap, 0);
}


/*
  Find the spec of a frame.
  With preference for dbcs assigned to the frames bus specifically.
*/
static message_t *find_msg_spec(frame_key_t *key,
                                busAssignment_t *bus_lib,
                                char **dbcname_loc)
{
    message_t *match = NULL;
    // Find match in dbc's explicitly set on this bus.
    match = get_msg_spec(bus_lib, key->id, key->bus, dbcname_loc);
    if (match)
        return match;

    // else find match in dbc that are used for any bus.
    return get_msg_spec(bus_lib, key->id, -1, dbcname_loc);
}


/*
  Goes through all msg_series_ts that are values in msg_hashmap.
  Populates the dbcname and ts_hash fields of each member.
  Returns -1 on failure, otherwise the number of signals decoded.
*/
int can_decode(struct hashtable *msg_hashmap, busAssignment_t *bus_lib)
{
    int count = 0;
    static int already_defined_warn = 0;
    if (!msg_hashmap || !hashtable_count(msg_hashmap))
        return -1;

    struct hashtable_itr *itr = hashtable_iterator(msg_hashmap);
    do {
        frame_key_t *frame_key = hashtable_iterator_key(itr);
        msg_series_t *msg = hashtable_iterator_value(itr);


        message_t *msg_spec = find_msg_spec(frame_key,
                                            bus_lib,
                                            &msg->dbcname);
        if (!msg_spec)
            continue; // Decode not possible

        msg->name = msg_spec->name;
        msg->ts_hash = create_hashtable(16, string_hash, string_equal);

        signal_list_t *sl;
        for (sl = msg_spec->signal_list; sl != NULL; sl = sl->next) {
            const signal_t *const spec = sl->signal;

            if (hashtable_search(msg->ts_hash, spec->name)) {
                if (!already_defined_warn) {
                    fprintf(stderr, "WARNING! Signal %s already exists!\n"
                            "Signalname used more than once in the same msg?\n"
                            "Skipping this and all future duplicates!",
                            spec->name);
                    already_defined_warn = 1;
                }
                continue;
            }

            double *data = signal_decode(spec, msg->data, msg->dlc, msg->n);
            if (data) {
                hashtable_insert(msg->ts_hash,
                                 (void *) strdup(spec->name),
                                 (void *) data);
                count++;
            }
        }
    } while (hashtable_iterator_advance(itr));
    free(itr);

    return count;
}
