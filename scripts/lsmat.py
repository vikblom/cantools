#!/usr/bin/env python3
import sys
import numpy as np
from scipy.io import loadmat


def keyer(tup):
    bus = str(tup[0])
    channel = tup[1].tobytes().decode('utf32')
    message = tup[2].tobytes().decode('utf32')
    signal = tup[3].tobytes().decode('utf32')
    return f"{bus}::{channel}::{message}::{signal}"


def valuer(tup):
    time = tup[4].squeeze()
    data = tup[5].squeeze()
    return (time, data)


def main(mat):
    for x in mat['data'][0]:
        print(keyer(x))


if __name__ == '__main__':
    main(loadmat(sys.argv[1]))
