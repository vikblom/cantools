#include <stdlib.h>
#include <stdio.h>
#include "hdf5.h"

char bytes[] = {1, 2, 3, 4, 5};
const hsize_t size = 5;
// const hsize_t max = H5S_UNLIMITED; Requires chunking

int main(int argc, char *argv[])
{
    herr_t err;
    hid_t file, group, space, data = -1;

    file = H5Fcreate("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0)
        goto exit;

    group = H5Gcreate(file, "/foo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group < 0)
        goto exit;

    space = H5Screate_simple(1, &size, NULL); // current is max
    if (space < 0)
        goto exit;


    data = H5Dcreate(group, "bar", H5T_NATIVE_CHAR, space,
                     H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (data < 0)
        goto exit;
    err = H5Dwrite(data, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes);
    if (err < 0)
        goto exit;

    data = H5Dcreate(group, "baz", H5T_NATIVE_CHAR, space,
                     H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (data < 0)
        goto exit;
    err = H5Dwrite(data, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes);
    if (err < 0)
        goto exit;


exit:
    if (data > 0) H5Dclose(data);
    if (space > 0) H5Sclose(space);
    if (group > 0) H5Gclose(group);
    if (file > 0) H5Fclose(file);
    return 0;
}
