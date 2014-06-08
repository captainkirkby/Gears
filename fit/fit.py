#!/usr/bin/env python

import math
import numpy
import scipy.interpolate
import matplotlib.pyplot as plt
import argparse

def main():

    # parse command-line args
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-i','--input', type=str, default='',
        help = 'name of input data file to analyze')
    parser.add_argument('--frame-size', type=int, default=1024,
        help = 'number of samples per frame')
    args = parser.parse_args()

    # load the input data file
    data = numpy.loadtxt(args.input)
    if len(data.shape) != 1:
        print 'Data has unexpected shape',data.shape
        return -1
    # loop over data frames
    nframe = len(data)/args.frame_size
    if len(data) % args.frame_size:
        print 'WARNING: Ignoring extra data beyond last frame'
    frames = data[:nframe*args.frame_size].reshape((nframe,args.frame_size))
    for frame in frames:
        pass

if __name__ == "__main__":
    main()