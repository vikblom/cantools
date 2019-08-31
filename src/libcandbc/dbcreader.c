/*  dbcReader.c --  frontend for DBC parser
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

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern void yyrestart(FILE *input_file);
extern int yyparse (void *YYPARSE_PARAM);

#include "dbcmodel.h"
#include "dbcreader.h"

dbc_t *dbc_read_file(char *filename)
{
    if (!filename)
        return NULL;

    dbc_t *dbc = dbc_new();

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr,"error: can't open the dbc file '%s'\n", filename);
        dbc_free(dbc);
        return NULL;
    }
    yyrestart(f);
    int error = yyparse((void *)dbc);
    fclose(f);
    if (!error) {
        dbc->filename = strdup(filename);
    } else {
        dbc->filename = NULL;
    }
    return dbc;
}
