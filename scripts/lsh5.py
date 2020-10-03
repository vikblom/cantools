#!/usr/bin/env python3
import sys
import h5py


def explore(h5, level=0):
    if hasattr(h5, 'items'):
        for k,v in h5.items():
            print("    "*level + f"{k}: {v}")
            explore(h5[k], level=level+1)


if __name__ == '__main__':
    explore(h5py.File(sys.argv[1]))
