/*  messageDecoder.c --  decode CAN messages
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

#include "dbcmodel.h"
#include "messagedecoder.h"

union thirtytwo {
    uint32_t raw;
    float phys;
};

union sixtyfour {
    uint64_t raw;
    double phys;
};

/*
 * Signal bit order:
 *
 *     7  6  5  4  3  2  1  0 offset
 *    bit
 * 0   7  6  5  4  3  2  1  0
 * 1  15 14 13 12 11 10  9  8
 * 2  23 22 21 20 19 18 17 16
 * 3  31 30 29 28 27 26 25 24
 * 4  39 38 37 36 35 34 33 32
 * 5  47 46 45 44 43 42 41 40
 * 6  55 54 53 52 51 50 49 48
 * 7  63 62 61 60 59 58 57 56
 * |
 * byte
 *
 * big endian place value exponent
 *                15 14 13 12   <- start_byte
 *    11 10  9  8  7  6  5  4
 *     3  2  1  0               <- end_byte
 *
 * little endian place value exponent
 *     3  2  1  0               <- start_byte
 *    11 10  9  8  7  6  5  4
 *                15 14 13 12   <- end_byte
 */

/*
 * Returns "raw value" of a signal.
 * I.e. the value as it is transmitted over the network.
 */
uint64 extract_raw_signal(const signal_t *const s,
                          const uint8 *const msgpayload,
                          const size_t dlc)
{
    uint64 raw_value = 0ULL;
    unsigned char *p = (unsigned char *) &raw_value;
    size_t i;
    for (i = 0; i < dlc; i++) {
        // endianess 1 is little, 0 is big
        p[i] = s->endianess ? msgpayload[i] : msgpayload[dlc-1 - i];
    }

    // Big endian has bit start to msb, we have flipped to order of bytes.
    // So we need to calculate where the lsb is in this new order.
    uint8_t start = s->bit_start;
    if (!s->endianess) {
        start = 8*(dlc-1 - start/8) + start%8;
        start -= s->bit_len - 1;
    }

    // We cannot handle these signals.
    if (start < 0 || start > 63) {
        // TODO: This should propagate to maybe not even passing on signal?
        //fprintf(stderr, "WARNING: Start bit out of bounds, skipping!\n");
        return 0;
    }

    raw_value >>= start;
    if (s->bit_len < 64) {
        raw_value &= ((1ULL << s->bit_len) - 1);
    }

    return raw_value;
}


double raw_to_physical(uint64_t rawValue, const signal_t *const s)
{
    // [Physical value] = ([Raw value] * [Factor]) + [Offset]
    double physical = 0;
    if (s->signal_val_type == svt_float) {
        union thirtytwo u = {.raw = rawValue};
        physical = u.phys;

    } else if (s->signal_val_type == svt_double) {
        union sixtyfour u = {.raw = rawValue};
        physical = u.phys;

    } else {
        if (s->signedness) {
            int64_t signedValue = 0;
            if (rawValue & (1ULL << (s->bit_len-1))) {
                rawValue = ~rawValue + 1;
                if (s->bit_len < 8*sizeof(rawValue)) {
                    rawValue &= (1ULL << s->bit_len) - 1;
                }
                signedValue = -((int64_t)rawValue);
            } else {
                signedValue = rawValue;
            }

            physical = signedValue;
        } else {
            physical = rawValue;
        }
    }
    return physical * s->scale + s->offset;
}


double *signal_decode(const signal_t *const spec,
                      unsigned char *bytes,
                      uint32_t dlc, uint32_t n)
{
    static int bitlen_warned = 0;
    if (spec->bit_len > 64) {
        if (!bitlen_warned) {
            fprintf(stderr,
                    "WARNING: Decoding more than 64 bits not yet implemented! "
                    "These signals will be skipped.\n");
            bitlen_warned = 1;
        }
        return NULL;
    }

    double *data = malloc(n * sizeof(double));
    if (!data)
        return NULL;

    for (int i = 0; i < n; ++i) {
        uint64 raw = extract_raw_signal(spec, bytes + i * dlc, dlc);
        //data[i] = raw_to_physical(raw, spec);
        double foo = raw_to_physical(raw, spec);
    }
    return data;
}


void canMessage_decode(message_t      *dbcMessage,
                       canMessage_t   *canMessage,
                       sint32          timeResolution,
                       signalProcCb_t  signalProcCb,
                       void           *cbData)
{
    // This is redundant for BLF, but just to be safe...
    static int big_dlc_warned = 0;
    if (canMessage->dlc > 8) {
        if (!big_dlc_warned) {
            fprintf(stderr,
                    "WARNING: DLC > 8 decoding not yet implemented! "
                    "Messages skipped.\n");
            big_dlc_warned = 1;
        }
        return;
    }

    static int bitlen_warned = 0;

    /* limit time resolution */
    uint64_t sec = canMessage->t.tv_sec;
    int64_t nsec = canMessage->t.tv_nsec;
    if(timeResolution)
        nsec -= (nsec % timeResolution);
    double dtime = sec + nsec * 1e-9;

    /* iterate over all signals */
    signal_list_t *sl;
    for(sl = dbcMessage->signal_list; sl != NULL; sl = sl->next) {

        const signal_t *const s = sl->signal;

        if (s->bit_len > 64) {
            if (!bitlen_warned) {
                fprintf(stderr,
                        "WARNING: Decoding more than 64 bits not yet implemented! "
                        "Signals skipped.\n");
                bitlen_warned = 1;
            }
            continue;
        }

        //fprintf(stderr, "id: %zu ", canMessage->id);
        uint64 rawValue = extract_raw_signal(s, canMessage->byte_arr,
                                             canMessage->dlc);

        double physicalValue = raw_to_physical(rawValue, s);

        /* invoke signal processing callback function */
        signalProcCb(s, dtime, rawValue, physicalValue, cbData);
    }
}
