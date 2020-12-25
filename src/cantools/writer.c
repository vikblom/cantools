#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "writer.h"


// Collection of all available writers by pointer.
extern can_writer_t matfile_writer;
extern can_writer_t hdf5_writer;
static can_writer_t *all_writers[] = {
    &matfile_writer,
    &hdf5_writer,
};


/* Check if str ends with tail. */
static int endswith(const char *str, const char *tail)
{
    int str_len = strlen(str);
    int tail_len = strlen(tail);

    if (str_len < tail_len)
        return 0;

    const char *needle = str + str_len - tail_len;

    return 0 == strcmp(needle, tail);
}

/* Finds the most suitable writer for writing the given file. */
writer_f guess_writer(const char *outfile)
{
    size_t n_formats = sizeof(all_writers)/sizeof(all_writers[0]);

    for (int i=0; i < n_formats; i++)
    {
        if (endswith(outfile, all_writers[i]->ext))
            return all_writers[i]->write_fcn;
    }
    return NULL;
}
