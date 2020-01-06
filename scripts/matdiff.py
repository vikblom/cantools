from sys import argv
import numpy as np
from scipy.io import loadmat


def keyer(tup):
    bus = str(tup[0])
    channel = tup[1].tostring().decode('utf32')
    message = tup[2].tostring().decode('utf32')
    signal = tup[3].tostring().decode('utf32')
    return f"{bus}::{channel}::{message}::{signal}"


def valuer(tup):
    time = tup[4].squeeze()
    data = tup[5].squeeze()
    return (time, data)



def main(this_file, that_file):
    this = {keyer(x) : valuer(x) for x in loadmat(this_file)["data"][0]}
    that = {keyer(x) : valuer(x) for x in loadmat(that_file)["data"][0]}

    this_keys = set(this.keys())
    that_keys = set(that.keys())

    if not this_keys <= that_keys:
        print("Keys removed!")

    if not that_keys <= this_keys:
        print(that_keys - this_keys)
        print("Keys added!")

    common_keys = this_keys & that_keys

    for k in common_keys:
        this_time = this[k][0]
        this_data = this[k][1]
        that_time = that[k][0]
        that_data = that[k][1]
        if this_time.size != that_time.size or \
           this_data.size != that_data.size:
            print(k, "is different in size!", this_time.size, that_time.size)
            continue

        if not np.allclose(this_time, that_time) or \
           not np.allclose(this_data, that_data):
            print(k, "is different in content!")


if __name__ == '__main__':
    if len(argv) != 3:
        print("Usage: matdiff this.mat that.mat")

    main(argv[1], argv[2])
