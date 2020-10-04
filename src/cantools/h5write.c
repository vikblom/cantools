#include <stdio.h>
#include "hdf5.h"
#include "hdf5_hl.h"

#include "measurement.h"
#include "hashtable_itr.h"

// https://support.hdfgroup.org/HDF5/doc/HL/RM_H5LT.html



/*
 * matWrite - write signals from measurement structure to MAT file
 */
int write_h5(struct hashtable *msg_hash, const char *out_file)
{
    herr_t err;
    int ret = -1;
    /* loop over all time series */
    if (hashtable_count(msg_hash) == 0) {
        fprintf(stderr, "error: measurement empty, nothing to write\n");
        return 1;
    }

    hid_t h5_file = H5Fcreate(out_file, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (h5_file < 0)
        goto exit;

    /* Iterator constructor only returns a valid iterator if
     * the hashtable is not empty */
    struct hashtable_itr *msg_itr = hashtable_iterator(msg_hash);
    do {
        frame_key_t *msg_key = hashtable_iterator_key(msg_itr);
        msg_series_t *msg = hashtable_iterator_value(msg_itr);
        if (!msg->ts_hash || hashtable_count(msg->ts_hash) == 0)
            continue;

        hid_t h5_msg = H5Gcreate(h5_file, msg->name,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (h5_msg < 0)
            goto exit;

        // TODO: More fields
        // Channel number: &msg_key->bus
        // DBC name: msg->dbcname

        const hsize_t samples_in_msg = msg->n;
        // Light interface, HDF5 space+data+write in one go!
        err = H5LTmake_dataset_double(h5_msg, "__time", 1,
                                      &samples_in_msg, msg->time);
        if (err < 0)
            goto exit; //FIXME: This will leak stuff...

        struct hashtable_itr *sig_itr = hashtable_iterator(msg->ts_hash);
        do {
            char *signame = hashtable_iterator_key(sig_itr);
            double *sigdata = hashtable_iterator_value(sig_itr);

            // Light interface, HDF5 space+data+write in one go!
            err = H5LTmake_dataset_double(h5_msg, signame, 1,
                                          &samples_in_msg, sigdata);
            if (err < 0)
                goto exit;

        } while (hashtable_iterator_advance(sig_itr));
        free(sig_itr);
        H5Gclose(h5_msg);

    } while (hashtable_iterator_advance(msg_itr));
    free(msg_itr);

    ret = 0;
exit:
    if (h5_file > 0) H5Fclose(h5_file);
    return ret;
}
