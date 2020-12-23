/*  cantomat -- convert CAN log files to MAT files
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
#include <stdlib.h>
#include <getopt.h>

#include "busassignment.h"
#include "measurement.h"

// readers
//#include "ascreader.h"
//#include "clgreader.h"
#include "blfreader.h"
//#include "vsbreader.h"

// writers
#include "matwrite.h"
#include "h5write.h"


const char *program_name;
int verbose_flag = 0;
int debug_flag   = 0;


static int str_ends_with(char *str, char *tail)
{
    int str_len = strlen(str);
    int tail_len = strlen(tail);

    if (str_len < tail_len)
        return 0;

    char *needle = str + str_len - tail_len;

    return 0 == strcmp(needle, tail);
}


static void help(void)
{
    fprintf(stderr,
            "Usage: %s [OPTION] -d dbcfile\n"
            "cantomat " VERSION ": Convert CAN trace file to MAT file.\n"
            "\n"
            "Options:\n"
            "  -b, --bus <busid>          specify bus for next database\n"
            "  -d, --dbc <dbcfile>        assign database to previously specified bus\n"
            "  -i, --in <infile>          input file, default to stdin. \n"
            "  -o, --out <outfile>        output file, defaults to stdout. \n"
            "  -t, --timeres <nanosec>    time resolution\n"
            "      --verbose              verbose output\n"
            "      --brief                brief output (default)\n"
            "      --debug                output debug information\n"
            "      --help                 display this help and exit\n"
            "\n",
            program_name);
}

int cantomat(char *in_file,
             parserFunction_t parserFunction,
             busAssignment_t *busAssignment,
             char *out_file)
{
    // READ
    struct hashtable *can_hashmap = read_messages(in_file,
                                                  parserFunction);
    if (!can_hashmap) {
        fprintf(stderr, "Reading msgs from input file failed.\n");
        return 1;
    }

    // DECODE
    int signal_count = can_decode(can_hashmap, busAssignment);
    if (signal_count < 0) {
        fprintf(stderr, "Reading signals from msgs failed.\n");
        return 1;
    }
    if (verbose_flag)
        fprintf(stderr, "Decoded %d timeseries\n", signal_count);

    // WRITE
    // FIXME: Dispatch on out_file ext
    if (str_ends_with(out_file, ".mat")) {
        matWrite(can_hashmap, out_file);
    } else if (str_ends_with(out_file, ".h5")) {
        write_h5(can_hashmap, out_file);
    }

    destroy_messages(can_hashmap);

    return 0;
}


int main(int argc, char **argv)
{
    program_name = argv[0];
    int ret = 1; // default to failure

    // Program arguments
    char *in_file = NULL;
    char *out_file = NULL;
    busAssignment_t *busAssignment = busAssignment_create();
    int bus = -1;
    sint32 timeResolution = 0;

    /* parse arguments */
    while (1) {

        static struct option long_options[] = {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            {"debug",   no_argument,       &debug_flag,   1},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"in",      required_argument, NULL, 'i'},
            {"out",     required_argument, NULL, 'o'},
            {"bus",     required_argument, NULL, 'b'},
            {"dbc",     required_argument, NULL, 'd'},
            {"timeres", required_argument, NULL, 't'},
            {"help",    no_argument,       NULL, 'h'},
            {0, 0, 0, 0}
        };

        // Also short options, with req. arguments. as above
        char short_options[] = "i:o:b:d:t:h";

        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long(argc, argv, short_options,
                            long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
        case 0:
            break;

        case 'i':
            if (in_file) {
                fprintf(stderr, "Multiple input files not supported!\n");
                exit(1);
            }
            in_file = optarg;
            break;

        case 'o':
            if (out_file) {
                fprintf(stderr, "Multiple output files not supported!\n");
                exit(1);
            }
            out_file = optarg;
            break;

        case 'b':
            bus = atoi(optarg);
            break;

        case 'd':
            if (verbose_flag) {
                if (bus == -1) {
                    fprintf(stderr, "Assigning DBC file %s to all busses\n", optarg);
                } else {
                    fprintf(stderr, "Assigning DBC file %s to bus %d\n", optarg, bus);
                }
            }
            busAssignment_associate(busAssignment, bus, optarg);

            /* reset bus specification */
            bus = -1;
            break;

        case 't':
            timeResolution = atoi(optarg);
            break;

        case 'h':
            help();
            exit(0);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            fprintf(stderr, "error: unknown option %c\n", c);
            goto exit;
        }
    }

    if (out_file == NULL) {
        fprintf(stderr, "error: Output file not specified\n");
        goto exit;
    }

    /* parse DBC files */
    if (busAssignment_parseDBC(busAssignment)) {
        goto exit;
    }

    // The actual decision is down in measurement.c...
    if (verbose_flag) {
        if (in_file != NULL) {
            fprintf(stderr, "Parsing input file %s\n",
                    in_file ? in_file : "<stdin>");
        }
    }

    // FIXME: Dispatch on input file extension.
    parserFunction_t parserFunction = blfReader_processFile;

    ret = cantomat(in_file,
                   parserFunction,
                   busAssignment,
                   out_file);
exit:
    busAssignment_free(busAssignment);
    return ret;
}
