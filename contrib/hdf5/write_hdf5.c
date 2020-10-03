#include <stdlib.h>
#include <stdio.h>
#include "hdf5.h"
#include "hdf5_hl.h"

// https://support.hdfgroup.org/HDF5/doc/HL/RM_H5LT.html


const hsize_t size = 5;
double bytes[] = {1.0, 2, 3, 4, 5};

int main(int argc, char *argv[])
{
    herr_t err;
    hid_t file, group, space, data, data2 = -1;

    file = H5Fcreate("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0)
        goto exit;

    group = H5Gcreate(file, "foo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group < 0)
        goto exit;

    group = H5Gcreate(group, "bar", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group < 0)
        goto exit;

    // Light interface, HDF5 space+data+write in one go1
    err = H5LTmake_dataset_double(group, "lite", 1, &size, bytes);
    if (err < 0)
        goto exit;

exit:
    if (group > 0) H5Gclose(group);
    if (file > 0) H5Fclose(file);
    return 0;
}
