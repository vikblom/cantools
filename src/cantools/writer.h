#ifndef _WRITER_H_
#define _WRITER_H_

#include "hashtable.h"

typedef int (* writer_f)(struct hashtable *msgs, const char *outfile);

// Instancer for a writer format.
typedef struct can_writer_t
{
    const char *name;
    const char *ext;
    writer_f write_fcn;
} can_writer_t;

writer_f guess_writer(const char *outfile);

#endif /* _WRITER_H_ */
