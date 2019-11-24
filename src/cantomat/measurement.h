#ifndef INCLUDE_MEASUREMENT_H
#define INCLUDE_MEASUREMENT_H

/*  measurement.h -- declarations for measurement
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

#include <stdio.h>

#include "cantomat.h"
#include "hashtable.h"
#include "busassignment.h"
#include "signalformat.h"


typedef struct {
    struct hashtable *timeSeriesHash;
} measurement_t;

typedef struct {
    unsigned int n;
    double *time;
    double *value;
} timeSeries_t;

typedef struct {
    unsigned int n;
    unsigned int cap;
    unsigned char *data;
    double *time;
    unsigned int dlc;
} msg_series_t;


typedef void (* parserFunction_t)(FILE *fp, msgRxCb_t msgRxCb, void *cbData);

struct hashtable *read_messages(const char *filename,
                                parserFunction_t parserFunction);
void destroy_messages(struct hashtable *can_hashmap);
void destroy_timeseries(struct hashtable *ts_hashmap);

struct hashtable *can_decode(struct hashtable *can_hashmap,
                             busAssignment_t *bus_lib);

#endif
