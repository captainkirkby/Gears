#!/usr/bin/env python

import sys
import math
import numpy
import scipy.interpolate
from scipy.stats import linregress
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

def physicalModel(t0,direction,lo,hi,tabs,D=2.,L=1108.,dtheta=4.66,T=2.0,nsamples=1024,adcTick=1664e-7):
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

def fitPhysicalModel(frame,tabs,args):

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
        prediction = physicalModel(t0,direction,lo,hi,tabs,D,L,dtheta,
            nsamples=args.nsamples,adcTick=args.adc_tick)
        residuals = frame - prediction
        return numpy.dot(residuals,residuals)

    # pick direction based on smallest chisq with initial parameter guesses
    chiSqP = chiSquare(t0Guess,+1,loGuess,hiGuess,DGuess,LGuess,dthetaGuess)
    chiSqM = chiSquare(t0Guess,-1,loGuess,hiGuess,DGuess,LGuess,dthetaGuess)
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

def runningAvg(data,wlen=31):
    # returns a running-average smoothing of data
    assert data.size > wlen and wlen % 2 == 1
    whalf = (wlen-1)/2
    # pad the input data by repeating the first and last elements
    padded = numpy.r_[[data[0]]*whalf,data,[data[-1]]*whalf]
    kernel = numpy.ones(wlen,'d')/wlen
    smooth = numpy.convolve(kernel,padded,mode='same')
    # remove the padding so the returned array has the same size and alignment as the input data
    return smooth[whalf:-whalf]

def lineFit(y,t1,t2):
    """
    Performs a linear fit to determine the value of t where y(t) crosses zero
    using samples y[t1:t2].
    """
    t = numpy.arange(t1,t2)
    slope, intercept, r_value, p_value, std_err = linregress(t,y[t1:t2])
    return -intercept/slope

def quickFit(frame,smoothing=15,fitsize=5):
    """
    Attempts a quick fit of the specified frame data or returns a RuntimeError
    """
    # perform a running-average smoothing of the frame's raw sample data
    smooth = runningAvg(frame,wlen=1+2*smoothing)
    # find the range of the smoothed data
    lo = numpy.min(smooth)
    hi = numpy.max(smooth)
    # find edges as points where the smoothed data crosses the midpoints between lo,hi
    midpt = 0.5*(lo+hi)
    smooth -= midpt
    rising = numpy.logical_and(smooth[:-1] <= 0,smooth[1:] > 0)
    falling = numpy.logical_and(smooth[:-1] > 0, smooth[1:] <= 0)
    nrise = numpy.count_nonzero(rising)
    nfall = numpy.count_nonzero(falling)
    # check for the expected number of rising and falling edges
    if nrise != 3:
        raise RuntimeError("quickFit: expected 3 rising edges but found %d" % numpy.count_nonzero(rising))
    if nfall != 3:
        raise RuntimeError("quickFit: expected 3 falling edges but found %d" % numpy.count_nonzero(falling))
    risePos = numpy.sort(numpy.argsort(rising)[-3:])
    fallPos = numpy.sort(numpy.argsort(falling)[-3:])
    # use the distance between the first falling and rising edges to discriminate between
    # the two possible directions of travel and identify the index of the fiducal crossing
    if risePos[0] - fallPos[0] > 150:
        direction = +1.
        xing = fallPos[1]
    else:
        direction = -1.
        xing = risePos[1]
    # perform a linear fit to the fiducial crossing
    t0 = lineFit(smooth,xing-fitsize,xing+fitsize+1)
    t = numpy.arange(xing-fitsize,xing+fitsize+1)
    # perform linear fits to the first and last edges
    tfirst = lineFit(smooth,fallPos[0]-fitsize,fallPos[0]+fitsize+1)
    tlast = lineFit(smooth,risePos[-1]-fitsize,risePos[-1]+fitsize+1)
    # build a visual representation of these fit results
    quick = numpy.empty((2,6))
    if direction > 0:
        quick[0] = numpy.array((0.,tfirst,tfirst,t0,t0,1024.))
    else:
        quick[0] = numpy.array((0.,t0,t0,tlast,tlast,1024.))
    quick[1] = numpy.array((hi,hi,lo,lo,hi,hi))
    return direction,t0,tlast-tfirst,quick

def fitSplineModel(frames,nknots,smax,args):
    # initialize a vector of frame sample offsets
    nFrames,frameSize = frames.shape
    assert frameSize == args.nsamples
    tsample = args.adc_tick*numpy.arange(frameSize)
    # use absolute min/max of frame samples to initialize range of transmission function
    lo = numpy.min(frames.flat)
    hi = numpy.max(frames.flat)
    # prepare vector of positions where the transmission function is tabulated for interpolation
    knots = numpy.linspace(-tmax,+tmax,nknots)
    # initialize parameters for each tabulated tranmission function value
    plist = [ ]
    pdict = { }
    for i,t in enumerate(knots):
        s = 3*t/(0.9*tmax)
        if s < -3:
            pinit = lo
        elif s < -1:
            pinit = hi
        elif s < 0:
            pinit = lo
        elif s < 1:
            pinit = hi
        elif s < 2:
            pinit = lo
        elif s < 3:
            pinit = hi
        else:
            pinit = lo
        pname = 'T%d' % i
        pdict[pname] = pinit
        plist.append(pname)

    # initialize parameters for each frame's offset and relative speed
    for i in range(len(frames)):
        pname = 't%d' % i
        pdict[pname] = 0.
        plist.append(pname)
        pname = 'v%d' % i
        pdict[pname] = 1.
        plist.append(pname)

    # define chi-square function to use
    def chiSquare(*params):
        chisq = 0.
        # initialize an interpolator of the transmission function using the specified knot values
        transf = scipy.interpolate.UnivariateSpline(knots,params[:nknots])
        # loop over frames
        for i,frame in frames:
            # lookup this frame's offset and relative speed parameter values
            t0,v = params[nknots+2*i:nknots+2*i+2]
            # transform the sample times to transmission function times
            ttrans = (tsample - t0)/v
            # evaluate the predicted transmission function for each sample
            pred = transf(ttrans)
            # update the chisq value
            residuals = pred - frame
            chisq += numpy.dot(residuals,residuals)
        return chisq

    # initialize fitter
    print_level = 1 if args.verbose else 0
    engine = Minuit(chiSquare,errordef=1.0,print_level=print_level,
        forced_parameters=plist,**pdict)
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
        self.lastDirection = None
        self.nextLastOffset = None
        self.lastOffset = None
        self.lastElapsed = None
        self.periods = numpy.array([],dtype=numpy.float64)
        # initialize plot display if requested
        if args.show_plots:
            self.fig = plt.figure(figsize=(12,12))
            plt.ion()
            plt.show()
            self.plotx = numpy.arange(args.nsamples)

    def process(self,elapsed,samples):
        """
        Processes the next frame of raw IR ADC samples. The parameter elapsed gives the number
        of ADC samples elapsed between the first sample of this frame and the first sample of
        the previous frame, or zero if this information is not available. Returns the estimated
        period in seconds (nominally 2 secs) or raises a RuntimeError.
        """
        if len(samples) != self.args.nsamples:
            # Something is seriously wrong.
            return -2
        # do the fit
        direction,offset,span,quick = quickFit(samples)
        if self.args.physical:
            fitParams,bestFit = fitPhysicalModel(samples,self.tabs,self.args)
            offset,direction = fitParams[:2]
        else:
            bestFit = None 
        # update our plots, if requested
        if self.args.show_plots and not self.args.batch_replay:
            plt.clf()
            plt.subplot(2,1,1)
            plt.plot(self.plotx,samples,'g+')
            if bestFit is not None:
                plt.plot(self.plotx,bestFit,'r-')
            plt.plot(quick[0],quick[1],'b:')
            plt.draw()
        # calculate the elapsed time since the last dead-center crossing in the same direction
        # which is nominally 2 seconds
        if direction != self.lastDirection:
            if self.lastElapsed and self.lastOffset:
                period = (self.lastElapsed - self.lastOffset + elapsed + offset)*self.args.adc_tick
            else:
                # We must have processed at least two frames before we can calculate a period.
                period = 0
        else:
            # We require alternating directions, so something went wrong but we will try to recover.
            period = -1
        # update our saved info
        self.lastDirection = direction
        self.lastOffset = self.nextLastOffset
        self.nextLastOffset = offset
        self.lastElapsed = elapsed
        # remember this result
        if abs(period - 2) < 0.1:
            self.periods = numpy.append(self.periods,period)
            if self.args.show_plots and not self.args.batch_replay:
                plt.subplot(2,1,2)
                plt.plot(1e6*(self.periods-2.0),'x')
                plt.draw()
        # return the estimated period in seconds
        return period

    def finish(self):
        if self.args.show_plots:
            if self.args.batch_replay:
                plt.clf()
                plt.subplot(1,1,1)
                plt.plot(1e6*(self.periods-2.0),'x')
            self.fig.savefig('fit.png')
        numpy.savetxt('fit.dat',self.periods)

def main():

    # parse command-line args
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--replay', type=str, default='',
        help = 'name of input data file to replay')
    parser.add_argument('--batch-replay', action = 'store_true',
        help = 'no interactive prompting for each frame during replay')
    parser.add_argument('--nsamples', type=int, default=1024,
        help = 'number of IR ADC samples per frame')
    parser.add_argument('--adc-tick', type = float, default = 1664e-7,
        help = 'ADC sampling period in seconds')
    parser.add_argument('--show-plots', action = 'store_true',
        help = 'display analysis plots')
    parser.add_argument('--physical', action = 'store_true',
        help = 'fit frames to a physical model')
    parser.add_argument('--verbose', action = 'store_true',
        help = 'generate verbose output')
    args = parser.parse_args()

    # define tab geometry
    tabs = numpy.array([[-15.,-5.],[0.,5.],[10.,15.]])

    # initialize our frame processor
    processor = FrameProcessor(tabs,args)

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
        nframe = len(data)/(1+args.nsamples)
        if len(data) % 1+args.nsamples:
            print 'WARNING: Ignoring extra data beyond last frame'
        frames = data[:nframe*(1+args.nsamples)].reshape((nframe,1+args.nsamples))
        for i,frame in enumerate(frames):
            elapsed,samples = frame[0],frame[1:]
            try:
                period = processor.process(elapsed,samples)
                print 'Period = %f secs (%d/%d)' % (period,i,nframe)
            except RuntimeError,e:
                print str(e)
            if not args.batch_replay:
                response = raw_input('Hit ENTER for next frame, q to quit, b to enter batch mode: ')
                if response == 'q':
                    break
                elif response == 'b':
                    args.batch_replay = True
    else:
        remaining = 0
        samples = numpy.zeros(args.nsamples)
        # read from STDIN to accumulate frames
        while True:
            try:
                line = sys.stdin.readline()
                value = int(line)
                if remaining == 0:
                    elapsed = value
                    remaining = args.nsamples
                elif remaining > 0:
                    samples[args.nsamples - remaining] = value
                    remaining -= 1
                    if remaining == 0:
                        period = processor.process(elapsed,samples)
                        # send the calculated period to our STDOUT and flush the buffer!
                        print period
                        sys.stdout.flush()
            except Exception,e:
                # Try to keep going silently after any error
                pass

    # clean up
    processor.finish()

if __name__ == "__main__":
    main()
