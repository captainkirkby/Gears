#!/usr/bin/env python

import math
import numpy
import scipy.interpolate
import matplotlib.pyplot as plt
import argparse

def edgeTransmission(x,D):
    """
    Returns the transmission fraction (0-1) when the center of the beam with diameter D is
    a distance x from the edge. The input x can be a numpy array. The units of x and D are
    arbitrary but must be the same.
    """
    r = 2*x/D
    t = numpy.zeros_like(r)
    t[r<=-1] = 1.
    use = r*r<1
    ruse = r[use]
    t[use] = (numpy.arccos(ruse)-ruse*numpy.sqrt(1-ruse*ruse))/numpy.pi
    return t

def tabsTransmission(x,tabs,D):
    """
    Returns the transmission function (0-1) of a sequence of tabs defined by edges at (x1,x2)
    assuming a beam of diameter D. The input x can be a numpy array. All units are arbitrary but
    must be the same.
    """
    result = numpy.ones_like(x)
    for (x1,x2) in tabs:
        result += edgeTransmission(x-x1,D) - edgeTransmission(x-x2,D)
    return result

def motion(dt,L,dtheta,T):
    """
    Converts times dt (secs) relative to dead center to transverse positions (mm) along the tabs.
    The inputs are the effective length (mm) of the pendulum L, the angular displacement (deg)
    of the pendulum's swing relative to dead center, and the effective period T (secs). The
    input t can be a numpy array.
    """
    # Calculate angular frequency in rad/sec
    omega = 2*numpy.pi/T
    # Calculate angular speed at dead center in rad/sec
    thdot = omega*numpy.sqrt(2*(1-numpy.cos(numpy.deg2rad(dtheta))))
    # Calculate x(t) taking into account two corrections to the simplest x(t) = L*thdot*t model.
    # First, the marker raises slightly during the swing so its sideways rate of motion dx/dt is
    # reduced.  Second, the angular speed decreases as the pendulum moves away from dead center.
    return L*numpy.sin(thdot/omega*numpy.sin(omega*dt))

def model(t0,ambient,gain,tabs,D=2.,L=1108.,dtheta=4.66,T=2.0,nsamples=1024,adcTick=1664e-7):
    """
    t0 = offset of dead center relative to first sample (ADC samples)
    """
    # initialize an array of times (secs) relative to the assumed dead center
    dt = adcTick*(numpy.arange(nsamples) - t0)
    print dt
    # convert to transverse positions (mm) relative to the tabs
    dx = motion(dt,L,dtheta,T)
    # calculate expected tranmission fraction (0-1) at each transverse position
    trans = tabsTransmission(dx,tabs,D)
    print trans
    # scale to ADC units
    return gain*(trans + ambient)

def fit(frame):
    return 0

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

    # initialize plot display
    fig = plt.figure(figsize=(12,12))
    plt.ion()
    plt.show()

    # define tab geometry
    tabs = numpy.array([[-15.,-5.],[0.,5.],[10.,15.]])

    # loop over data frames
    xvec = numpy.arange(args.frame_size)
    nframe = len(data)/args.frame_size
    if len(data) % args.frame_size:
        print 'WARNING: Ignoring extra data beyond last frame'
    frames = data[:nframe*args.frame_size].reshape((nframe,args.frame_size))
    for frame in frames:
        plt.cla()
        plt.plot(xvec,frame,'+')
        pred = model(t0=495.,ambient=0.,gain=927.,L=1020.,dtheta=4.66,D=1.7,tabs=tabs)
        print pred
        plt.plot(xvec,pred,'-')
        plt.draw()
        params = fit(frame)
        q = raw_input('hit ENTER...')
        if q == 'q':
            break

if __name__ == "__main__":
    main()
