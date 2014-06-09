#!/usr/bin/env python

import math
import numpy
import scipy.interpolate
import matplotlib.pyplot as plt
from iminuit import Minuit
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

def model(t0,direction,lo,hi,tabs,D=2.,L=1108.,dtheta=4.66,T=2.0,nsamples=1024,adcTick=1664e-7):
    """
    t0 = offset of dead center relative to first sample (ADC samples)
    """
    # initialize an array of times (secs) relative to the assumed dead center
    dt = direction*adcTick*(numpy.arange(nsamples) - t0)
    # convert to transverse positions (mm) relative to the tabs
    dx = motion(dt,L,dtheta,T)
    # calculate expected tranmission fraction (0-1) at each transverse position
    trans = tabsTransmission(dx,tabs,D)
    # scale to ADC units
    return lo + (hi-lo)*trans

def fit(frame,tabs,args):

    # use absolute min/max to initialize lo/hi parameters
    loGuess = numpy.min(frame)
    hiGuess = numpy.max(frame)

    # hard code initial parameter guesses for now
    t0Guess = 495.
    DGuess = 1.7
    LGuess = 1020.
    dthetaGuess = 4.66

    # initialize model prediction
    global prediction
    prediction = numpy.zeros_like(frame)

    # define chi-square function to use
    def chiSquare(t0,direction,lo,hi,D,L,dtheta):
        global prediction
        prediction = model(t0,direction,lo,hi,tabs,D,L,dtheta,nsamples=args.frame_size)
        residuals = frame - prediction
        return numpy.dot(residuals,residuals)

    # pick direction based on smallest chisq with initial parameter guesses
    chiSqP = chiSquare(t0Guess,+1,loGuess,hiGuess,DGuess,LGuess,dthetaGuess)
    chiSqM = chiSquare(t0Guess,-1,loGuess,hiGuess,DGuess,LGuess,dthetaGuess)
    print 'chisq +/-',chiSqP,chiSqM
    direction = +1. if chiSqP < chiSqM else -1.

    # initialize fitter
    print_level = 1 if args.verbose else 0
    engine = Minuit(chiSquare,errordef=1.0,print_level=print_level,
        direction=direction,fix_direction=True,
        t0=t0Guess,error_t0=1.,
        lo=loGuess,error_lo=1.,
        hi=hiGuess,error_hi=1.,
        D=DGuess,error_D=0.1,fix_D=False,
        L=LGuess,error_L=10.,fix_L=True,
        dtheta=dthetaGuess,error_dtheta=0.5,fix_dtheta=False)
    engine.tol = 10.

    # do the fit
    minimum = engine.migrad()
    if not minimum[0]['has_valid_parameters']:
        raise RuntimeError('Fit failed!')

    # calculate the best fit model
    chiSquare(*engine.args)

    # return best-fit parameter values and best-fit model prediction
    return engine.args,prediction

class FrameProcessor(object):
    def __init__(self,tabs,args):
        self.tabs = tabs
        self.args = args
        self.frameSize = args.frame_size
        self.lastDirection = None
    def process(elapsed,frame):
        """
        Processes the next frame of raw IR ADC samples. The parameter elapsed gives the number
        of ADC samples elapsed between the first sample of this frame and the first sample of
        the previous frame, or zero if this information is not available.
        """
        if len(frame) != self.frameSize:
            raise RuntimeError('Got frame size %d but expected %d' % (len(frame),self.frameSize))
        # do the fit
        fitParams,bestFit = fit(frame,self.tabs,self.args)
        print 'FIT:',fitParams
        offset,direction = fitParams[0:1]
        # check that this frame reverses the direction of the previous frame
        if direction == self.lastDirection:
            raise RuntimeError('Got two frames in the same direction')
        self.lastDirection = direction

def main():

    # parse command-line args
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--replay', type=str, default='',
        help = 'name of input data file to replay')
    parser.add_argument('--frame-size', type=int, default=1024,
        help = 'number of samples per frame')
    parser.add_argument('--verbose', action = 'store_true',
        help = 'generate verbose output')
    args = parser.parse_args()

    # initialize plot display
    fig = plt.figure(figsize=(12,12))
    plt.ion()
    plt.show()
    xvec = numpy.arange(args.frame_size)

    # define tab geometry
    tabs = numpy.array([[-15.,-5.],[0.,5.],[10.,15.]])

    # replay a pre-recorded data file if requested
    if args.replay:
        # load the input data file
        data = numpy.loadtxt(args.replay)
        if len(data.shape) != 1:
            print 'Data has unexpected shape',data.shape
            return -1
        if args.verbose:
            print 'Read %d bytes from %s' % (len(data),args.replay)
        # loop over data frames
        nframe = len(data)/(1+args.frame_size)
        if len(data) % 1+args.frame_size:
            print 'WARNING: Ignoring extra data beyond last frame'
        frames = data[:nframe*(1+args.frame_size)].reshape((nframe,1+args.frame_size))
        for frame in frames:
            elapsed,samples = frame[0],frame[1:]
            print 'Elapsed samples since last frame =',elapsed
            plt.cla()
            plt.plot(xvec,samples,'g+')
            params,bestFit = fit(samples,tabs,args)
            plt.plot(xvec,bestFit,'b-')
            plt.draw()
            q = raw_input('Hit ENTER for next frame or q ENTER to quit...')
            if q == 'q':
                break

if __name__ == "__main__":
    main()
