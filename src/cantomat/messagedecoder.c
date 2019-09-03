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
                          const uint8 *const msgpayload)
{

    uint64 raw_value = 0;
    unsigned char *p = (unsigned char *) &raw_value;
    uint8 i;
    // endianess 1 is little, 0 is big
    for (i = 0; i < 8; i++) {
        p[i] = s->endianess ? msgpayload[i] : msgpayload[7-i];
        //fprintf(stderr, "%02X ", p[i]);
    }
    //fprintf(stderr, "\n");

    // Big endian has bit start to msb, we have flipped to order of bytes.
    // So we need to calculate where the lsb is in this new order.
    uint8_t start = s->bit_start;
    if (!s->endianess) {
        start = 56 - 8*(start/8) + start%8;
        start -= s->bit_len - 1;
    }

    //fprintf(stderr, "orig: 0x%16llX\n", raw_value);
    //fprintf(stderr, "%u %u\n", start, s->bit_len);

    // We cannot handle these signals.
    if (start > 63) {
        // TODO: This should propagate to maybe not even passing on signal?
        //fprintf(stderr, "WARNING: Start bit out of bounds, skipping!\n");
        return 0;
    }

    raw_value >>= start;
    //fprintf(stderr, "shft: 0x%016llX\n", raw_value);
    if (s->bit_len < 64) {
        raw_value &= ((1ULL << s->bit_len) - 1);
    }
    //fprintf(stderr, "mask: 0x%016llX\n", raw_value);
    return raw_value;
}


void canMessage_decode(message_t      *dbcMessage,
                       canMessage_t   *canMessage,
                       sint32          timeResolution,
                       signalProcCb_t  signalProcCb,
                       void           *cbData)
{
    static int sign_warned = 0;
    uint64_t sec = canMessage->t.tv_sec;
    int64_t nsec = canMessage->t.tv_nsec;

    /* limit time resolution */
    if(timeResolution != 0) {
        nsec -= (nsec % timeResolution);
    }
    double dtime = sec + nsec * 1e-9;

    /* iterate over all signals */
    signal_list_t *sl;
    for(sl = dbcMessage->signal_list; sl != NULL; sl = sl->next) {

        const signal_t *const s = sl->signal;
        uint64 rawValue = extract_raw_signal(s, canMessage->byte_arr);

        /* perform sign extension */
        if (s->signedness && (s->bit_len < 64)) {
            if (!sign_warned) {
                fprintf(stderr, "WARNING: Signed signales not yet verified!\n");
                sign_warned = 1;
            }
            uint64_t sign_mask = 1ULL << (s->bit_len-1);
            rawValue = ((int64_t) rawValue ^ sign_mask) - sign_mask;
        }

        if (s->signal_val_type != svt_integer) {
            fprintf(stderr, "WARNING: Float and double not implemented yet!\n");
            return;
        }

        // [Physical value] = ( [Raw value] * [Factor] ) + [Offset]
        double physicalValue;
        if (s->signedness) {
            physicalValue = (double) (int64_t) rawValue * s->scale + s->offset;
        } else {
            physicalValue = (double) rawValue * s->scale + s->offset;
        }

        /* invoke signal processing callback function */
        signalProcCb(s, dtime, rawValue, physicalValue, cbData);
    }
}
