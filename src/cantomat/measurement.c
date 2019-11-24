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
#include "signalformat.h"
#include "hashtable.h"
#include "hashtable_itr.h"
#include "messagedecoder.h"
#include "dbcmodel.h"

/* callback structure for timeSeries signal handler */
typedef struct {
    struct hashtable *timeSeriesHash;
    char             *local_prefix;
} signalProcCbData_t;

/* callback structure for timeSeries message handler */
typedef struct {
    struct hashtable *can_hashmap;
} messageProcCbData_t;

/* simple string hash function for signal names */
static unsigned int signalname_hash( void *k)
{
    unsigned int hash = 0;
    int c;

    while ((c = *(unsigned char *)k++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

/* string comparison function for signal names */
static int signalname_equal ( void *key1, void *key2 )
{
    return strcmp((char *)key1, (char *)key2) == 0;
}


typedef struct {
    uint32_t id;
    uint8_t bus;
} frame_key_t;


static unsigned int can_hash(void *this)
{
    frame_key_t *frame_key_p = (frame_key_t *) this;
    return frame_key_p->id;
}


static int can_equal(void *this, void *that)
{
    frame_key_t *this_p = (frame_key_t *) this;
    frame_key_t *that_p = (frame_key_t *) that;

    return this_p->id == that_p->id;// && this_p->bus == that_p->bus;
}


/*
 * callback function for processing a CAN message
 */
static void canframe_callback(canMessage_t *canMessage, void *cbData)
{
    messageProcCbData_t *messageProcCbData = (messageProcCbData_t *)cbData;
    struct hashtable *can_hashmap = messageProcCbData->can_hashmap;

    /* look for signal in time series hash */
    frame_key_t frame_key = {canMessage->id, canMessage->bus};
    msg_series_t *msg_series_p = hashtable_search(can_hashmap,
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

        hashtable_insert(can_hashmap,
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
 * process CAN trace file with given bus assignment and output
 * signal format
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
    struct hashtable *can_hashmap = create_hashtable(16, can_hash, can_equal);

    /*
     * Invoke the file format parser on file pointer fp.
     * This function will call the passed cb for all msgs.
     * The parser function is responsible for closing the input
     * file stream
     * One of: blfReader_processFile or friends...
     */
    messageProcCbData_t messageProcCbData = {
        can_hashmap
    };
    parserFunction(fp, canframe_callback, &messageProcCbData);

    if (filename != NULL)
        fclose(fp);
    return can_hashmap;
}


void destroy_messages(struct hashtable *can_hashmap)
{
    if (!can_hashmap)
        return;

    if (hashtable_count(can_hashmap)) {
        struct hashtable_itr *itr = hashtable_iterator(can_hashmap);
        do {
            msg_series_t *m = hashtable_iterator_value(itr);
            free(m->time);
            free(m->data);
            free(m);
        } while (hashtable_iterator_advance(itr));
        free(itr);
    }
    hashtable_destroy(can_hashmap, 0);
}


static message_t *find_msg_spec(frame_key_t *key, busAssignment_t *bus_lib,
                                char **dbcname_loc)
{
    message_t *candidate, *match = NULL;

    for (int i = 0; i < bus_lib->n ; i++) {
        busAssignmentEntry_t entry = bus_lib->list[i];

        /* check if bus matches and contains id */
        if ((entry.bus == -1) || (entry.bus == key->bus)) {
            if (candidate = hashtable_search(entry.messageHash, &key->id)) {
                match = candidate;
                *dbcname_loc = entry.basename;
                break;
            }
        }
    }
    return match;
}


struct hashtable *can_decode(struct hashtable *can_hashmap,
                             busAssignment_t *bus_lib)
{
    if (!can_hashmap || !hashtable_count(can_hashmap))
        return NULL;

    struct hashtable *ts_hashmap = create_hashtable(16, signalname_hash,
                                                    signalname_equal);

    struct hashtable_itr *itr = hashtable_iterator(can_hashmap);
    do {
        frame_key_t *frame_key = hashtable_iterator_key(itr);
        char *dbcname; // TODO: Find a smoother way to return this info.
        message_t *msg_spec = find_msg_spec(frame_key, bus_lib, &dbcname);
        if (!msg_spec)
            continue; // Decode not possible

        char *name_base = signalFormat_stringAppend(dbcname, msg_spec->name);
        msg_series_t *val = hashtable_iterator_value(itr);
        signal_list_t *sl;
        for(sl = msg_spec->signal_list; sl != NULL; sl = sl->next) {
            const signal_t *const s = sl->signal;

            char *signal_key = signalFormat_stringAppend(name_base, s->name);
            if (hashtable_search(ts_hashmap, signal_key)) {
                fprintf(stderr, "Signal %s already defined!\n", signal_key);
                free(signal_key);
                continue;
            }

            double *data;
            if (data = signal_decode(s, val->data, val->dlc, val->n)) {
                timeSeries_t *timeSeries = malloc(sizeof(timeSeries_t));
                timeSeries->n = val->n;
                timeSeries->time = val->time;
                timeSeries->value = data;

                hashtable_insert(ts_hashmap,
                                 (void *) signal_key,
                                 (void *) timeSeries);
            }
        }
        free(name_base);
    } while (hashtable_iterator_advance(itr));
    free(itr);

    return ts_hashmap;
}

void destroy_timeseries(struct hashtable *ts_hashmap)
{

    if (!ts_hashmap)
        return;

    if (hashtable_count(ts_hashmap)) {
        struct hashtable_itr *itr = hashtable_iterator(ts_hashmap);
        do {
            timeSeries_t *ts = hashtable_iterator_value(itr);
            free(ts->value);
            free(ts);
        } while (hashtable_iterator_advance(itr));
        free(itr);
    }
    hashtable_destroy(ts_hashmap, 0);
}



/* free measurement structure */
void measurement_free(measurement_t *m)
{
    // fprintf(stderr,"freeing %p (measurement)\n",m);
    if (m != NULL) {

        /* free time series */
        struct hashtable *timeSeriesHash = m->timeSeriesHash;
        struct hashtable_itr *itr;

        /* loop over all time series in hash */
        if (hashtable_count(timeSeriesHash) > 0) {
            itr = hashtable_iterator(timeSeriesHash);
            do {
                char         *signalName = hashtable_iterator_key(itr);
                timeSeries_t *timeSeries = hashtable_iterator_value(itr);

                free(timeSeries->time);
                free(timeSeries->value);
            } while (hashtable_iterator_advance(itr));
            free(itr);
        }

        /* free key and value (timeSeries_t) and hashTable */
        hashtable_destroy(m->timeSeriesHash, 1);
        free(m);
    }
}


/* used only for debugging: print data to stderr */
static void signalProc_print(
    const signal_t *s,
    const canMessage_t *canMessage,
    uint32 rawValue,
    double physicalValue,
    void *cbData)
{
    /* recover callback data */
    signalProcCbData_t *signalProcCbData = (signalProcCbData_t *)cbData;
    char *outputSignalName =
        signalFormat_stringAppend(signalProcCbData->local_prefix, s->name);

    fprintf(stderr,"   %s\t=%f ~ raw=%u\t~ %d|%d@%d%c (%f,%f)"
            " [%f|%f] %d %ul \"%s\"\n",
            outputSignalName,
            physicalValue,
            rawValue,
            s->bit_start,
            s->bit_len,
            s->endianess,
            s->signedness?'-':'+',
            s->scale,
            s->offset,
            s->min,
            s->max,
            s->mux_type,
            (unsigned int)s->mux_value,
            s->comment!=NULL?s->comment:"");

    /* free temp. signal name */
    if (outputSignalName != NULL) free(outputSignalName);
}
