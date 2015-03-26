#!/usr/bin/env python

import sys
import math
import numpy
import scipy.interpolate
from scipy.stats import linregress
import matplotlib.pyplot as plt
from iminuit import Minuit
import argparse
from pymongo import MongoClient
from pymongo import DESCENDING

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

def fitTemplateModel(samples,args,direction,lo,hi,offset,rise,fall,template):

    # initial parameter guesses
    loGuess = lo
    rngGuess = hi-lo
    t0Guess = offset
    durationGuess = (rise[-1] - fall[0])/2.
    
    # initialize a vector of ADC sample counts
    tadc = numpy.linspace(0.,args.nsamples-1.,args.nsamples)

    # initialize model prediction
    global prediction
    prediction = numpy.zeros_like(samples)

    # define chi-square function to use
    def chiSquare(t0,duration,lo,rng):
        global prediction
        # convert this frame's ADC timing to s values, correcting for the direction of
        # travel, the estimate fiducial crossing time, and the stretch factor.
        svec = direction*(tadc - t0)/duration
        # evaluate the template at these s values
        prediction[:] = 1.
        mask = numpy.abs(svec) < 1 + args.spline_pad
        prediction[mask] = template(svec[mask])
        # rescale from [0,1] to [lo,hi]
        prediction = lo + rng*prediction
        # evaluate the chi-square
        residuals = samples - prediction
        return numpy.dot(residuals,residuals)

    # initialize fitter
    print_level = 1 if args.verbose else 0
    engine = Minuit(chiSquare,errordef=1.0,print_level=print_level,
        t0=t0Guess,error_t0=1.,
        duration=durationGuess,error_duration=1.,
        lo=loGuess,error_lo=1.,
        rng=rngGuess,error_rng=1.)
    engine.tol = 1.

    # do the fit
    minimum = engine.migrad()
    if not minimum[0]['has_valid_parameters']:
        raise RuntimeError('Fit failed!')

    # calculate the best fit model
    chiSquare(*engine.args)

    # return best-fit parameter values and best-fit model prediction
    return engine.args,prediction

def fitPhysicalModel(samples,tabs,args,direction,lo,hi,offset):

    # initial parameter guesses
    loGuess = lo
    hiGuess = hi
    t0Guess = offset
    DGuess = 1.7
    LGuess = 1020.
    dthetaGuess = 4.66

    # initialize model prediction
    global prediction
    prediction = numpy.zeros_like(samples)

    # define chi-square function to use
    def chiSquare(t0,lo,hi,D,L,dtheta):
        global prediction
        prediction = physicalModel(t0,direction,lo,hi,tabs,D,L,dtheta,
            nsamples=args.nsamples,adcTick=args.adc_tick)
        residuals = samples - prediction
        return numpy.dot(residuals,residuals)

    # initialize fitter
    print_level = 1 if args.verbose else 0
    engine = Minuit(chiSquare,errordef=1.0,print_level=print_level,
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

def quickFit(samples,args,smoothing=15,fitsize=5,avgWindow=50):
    """
    Attempts a quick fit of the specified sample data or returns a RuntimeError.
    Returns the direction (+/-1) of travel, the estimated lo and hi ADC levels,
    the estimated fiducial crossing time t0 relative to the first sample, and
    the estimated rise and fall times relative to the fiducial and corrected for
    the direction of travel. All times are measured in ADC samples.
    """
    # perform a running-average smoothing of the frame's raw sample data
    smooth = runningAvg(samples,wlen=1+2*smoothing)
    # Find the range of the smoothed data. Use the min of the smooth samples to estimate the
    # lo value. Use the mean of the left and right margins to estimate the hi value. The
    # reason why don't use the max of the smooth samples to estimate the hi value is that we
    # observe some peaking (transmission > 1) near the edges.
    lo = numpy.min(smooth)
    margin = int(math.floor(samples.size/16.))
    hi = 0.5*(numpy.mean(smooth[:margin]) + numpy.mean(smooth[-margin:]))
    # find edges as points where the smoothed data crosses the midpoints between lo,hi
    midpt = 0.5*(lo+hi)
    smooth -= midpt
    rising = numpy.logical_and(smooth[:-1] <= 0,smooth[1:] > 0)
    falling = numpy.logical_and(smooth[:-1] > 0, smooth[1:] <= 0)
    nrise = numpy.count_nonzero(rising)
    nfall = numpy.count_nonzero(falling)
    # check for the expected number of rising and falling edges
    if nrise != args.nfingers:
        raise RuntimeError("quickFit: expected %d rising edges but found %d" % (args.nfingers, numpy.count_nonzero(rising)))
    if nfall != args.nfingers:
        raise RuntimeError("quickFit: expected %d falling edges but found %d" % (args.nfingers, numpy.count_nonzero(falling)))
    # locate the nearest ADC sample to each edge
    risePos = numpy.sort(numpy.argsort(rising)[-1*args.nfingers:])
    fallPos = numpy.sort(numpy.argsort(falling)[-1*args.nfingers:])
    # perform linear fits to locate each edge to subsample precision
    riseFit = numpy.empty((args.nfingers,))
    fallFit = numpy.empty((args.nfingers,))
    for i in range(args.nfingers):
        riseFit[i] = lineFit(smooth,risePos[i]-fitsize,risePos[i]+fitsize+1)
        fallFit[i] = lineFit(smooth,fallPos[i]-fitsize,fallPos[i]+fitsize+1)
    # use the distance between the first falling and rising edges to discriminate between
    # the two possible directions of travel and calculate edge times relative to the
    # fiducial, corrected for the direction of travel.
    if risePos[0] - fallPos[0] > samples.size/(2.0 * args.nfingers):
        direction = +1.
        t0 = (fallFit[2]+riseFit[2])/2
        riseFit -= t0
        fallFit -= t0
    else:
        direction = -1.
        t0 = (fallFit[2]+riseFit[2])/2
        tmp = numpy.copy(riseFit)
        riseFit = (t0 - fallFit)[::-1]
        fallFit = (t0 - tmp)[::-1]
    # calculate the height of the mid-notch by getting the mean value of a window around t0
    lb = t0 - avgWindow
    rb = t0 + avgWindow
    # add one to get same number of samples on both sides of t0
    # x x x x x x x x x x x x x x x x x
    #     l-------t0-------r
    # note: you can access non integer elements of a numpy array (truncates decimal)
    height = numpy.mean(samples[lb:rb+1])
    return direction,lo,hi,t0,riseFit,fallFit,height

def buildSplineTemplate(frames,args):
    """
    Builds a universal template for the transmission function from the specified frames.
    """
    # get the number of frames and check the frame size
    nframes,frameSize = frames.shape
    assert frameSize == 1+args.nsamples
    samples = frames[:,1:]
    # perform quick fits to each frame
    if args.verbose:
        print 'Performing quick fits...'
    dirvec = numpy.empty((nframes,))
    lovec = numpy.empty((nframes,))
    hivec = numpy.empty((nframes,))
    t0vec = numpy.empty((nframes,))
    risevec = numpy.empty((nframes,args.nfingers))
    fallvec = numpy.empty((nframes,args.nfingers))
    for i in range(nframes):
        dirvec[i],lovec[i],hivec[i],t0vec[i],risevec[i],fallvec[i],unusedHeight = quickFit(samples[i],args)
    # calculate the mean lo,hi levels
    lo = numpy.mean(lovec)
    hi = numpy.mean(hivec)
    # calculate the mean positions of each edge relative to the fiducial
    rise = numpy.mean(risevec,axis=0)
    fall = numpy.mean(fallvec,axis=0)
    if args.verbose:
        print 'rise/fall',rise,fall
    # Estimate the stretch factors that map the time between the outer edges to ds = 2
    stretch = (risevec[:,-1] - fallvec[:,0])/2.
    # initialize a vector of ADC sample counts
    tadc = numpy.linspace(0.,args.nsamples-1.,args.nsamples)
    # initialize our resampling grid
    smax = 1 + args.spline_pad
    sgrid = numpy.linspace(-smax,+smax,args.nspline)
    # initialize the resampled values we will average
    resampled = numpy.zeros_like(sgrid)
    # loop over frames
    if args.verbose:
        print 'Stacking frames to build spline template...'
    for i in range(nframes):
        # convert this frame's ADC timing to s values, correcting for the direction of
        # travel, the estimate fiducial crossing time, and the stretch factor.
        svec = dirvec[i]*(tadc - t0vec[i])/stretch[i]
        # normalize this frame's samples to [0,1]
        yvec = (samples[i] - lo)/(hi - lo)
        # ensure that svec is increasing
        if dirvec[i] < 0:
            svec = svec[::-1]
            yvec = yvec[::-1]
        # build a cubic spline interpolation of (svec,ynorm)
        splineFit = scipy.interpolate.UnivariateSpline(svec,yvec,k=3,s=0.,)
        # resample the spline fit to our s grid
        resampled += splineFit(sgrid)
    # average and return the resampled values
    resampled /= nframes
    return numpy.vstack((sgrid,resampled))

class FrameProcessor(object):
    def __init__(self,tabs,args,db):
        self.tabs = tabs
        self.args = args
        self.db = db
        self.lastDirection = None
        self.nextLastOffset = None
        self.lastOffset = None
        self.lastElapsed = None
        self.lastAmplitude = None
        self.lastSamplesSinceBoot = None
        self.nextLastSamplesSinceBoot = None
        # we use python arrays here since we do not know the final size in advance
        # and they are more time efficient for appending than numpy arrays (but
        # less space efficient)
        self.periods = [ ]
        self.swings = [ ]
        # initialize plot display if requested
        if self.args.show_plots:
            self.fig = plt.figure(figsize=(12,12))
            plt.ion()
            plt.show()
            self.plotx = numpy.arange(self.args.nsamples)
        # load template if requested
        if self.args.load_template is not None:
            if self.args.physical:
                raise RuntimeError('Options --physical and --load-template cannot be combined')
            if self.args.load_template == "db":
                # Load from database
                templateTuple = self.db.loadTemplate()
                templateData = templateTuple[0]
                self.mostRecentTemplateTimestamp = templateTuple[1]
            else:
                #load from file specified
                templateData = numpy.transpose(numpy.loadtxt(self.args.load_template))
            try:
                self.template = scipy.interpolate.UnivariateSpline(templateData[0],templateData[1],k=3,s=0.)
            except IndexError:
                self.template = None
                # Output zeroes
        else:
            self.template = None

    def updateTemplate(self):
        """
        Checks the database for a newer version of the template.  If one exists, replace
        self.template with it and replace self.mostRecentTemplateTimestamp with its
        timestamp
        """
        if self.args.load_template == "db":
            # Try and create a template if one doesn't exist
            if self.template is None:
                # Try and create one
                dataTuple = self.db.loadData()
                data = dataTuple[0]
                timestamp = dataTuple[1]
                if len(data.shape) != 1:
                    return
                # loop over data frames
                nframe = len(data)/(1+self.args.nsamples)
                if(nframe < self.args.fetch_limit):
                    return
                if not (self.args.max_frames == 0 or nframe <= self.args.max_frames):
                    nframe = self.args.max_frames
                frames = data[:nframe*(1+self.args.nsamples)].reshape((nframe,1+self.args.nsamples))
                template = buildSplineTemplate(frames,self.args)
                # Save to database as array of ordered pairs (arrays)
                self.db.saveTemplate(template,timestamp)
            # Look for newer template from database
            templateTuple = self.db.loadTemplate()
            templateData = templateTuple[0]
            templateTimestamp = templateTuple[1]
            if templateTimestamp != self.mostRecentTemplateTimestamp:
                self.template = scipy.interpolate.UnivariateSpline(templateData[0],templateData[1],k=3,s=0.)
                self.mostRecentTemplateTimestamp = templateTimestamp



    def process(self,samplesSinceBoot,samples):
        """
        Processes the next frame of raw IR ADC samples. The parameter samplesSinceBoot gives the number
        of ADC samples elapsed between the first sample of this frame and the last boot packet recieved, 
        or zero if this information is not available. Returns the estimated
        period in seconds (nominally 2 secs) or raises a RuntimeError.
        """
        if len(samples) != self.args.nsamples:
            # Something is seriously wrong.
            return -2
        if self.args.load_template and self.template is None:
            return 0,0
        # always start with a quick fit
        direction,lo,hi,offset,rise,fall,height = quickFit(samples,self.args)
        if self.args.physical:
            fitParams,bestFit = fitPhysicalModel(samples,self.tabs,self.args,direction,lo,hi,offset)
            offset,amplitude = fitParams[0],fitParams[5]
        elif self.template is not None:
            fitParams,bestFit = fitTemplateModel(samples,self.args,
                direction,lo,hi,offset,rise,fall,self.template)
            offset,duration = fitParams[0],fitParams[1]
            # convert duration in ADC ticks to a swing amplitude
            velocity = 0.5*self.args.width/(duration*self.args.adc_tick) # mm/sec
            if len(self.periods) > 0:
                period = self.periods[-1]
            else:
                period = 2.
            cosTheta = 1 - 0.5*(velocity*period/(2*math.pi*self.args.length))**2
            amplitude = math.degrees(math.acos(cosTheta))
        else:
            amplitude = None
            bestFit = None
        # update our plots, if requested
        if self.args.show_plots and not self.args.batch_replay:
            plt.clf()
            ax = plt.subplot(1,1,1)
            ax.set_ylim([-4.,1024.])
            plt.plot(self.plotx,samples,'g+')
            if self.args.show_centers:
                plt.plot(numpy.zeros(100)+offset,numpy.linspace(0,2**10,100),'b+');
            if self.args.show_heights:
                plt.plot(numpy.linspace(0,self.args.nsamples,100),numpy.zeros(100)+height,'y+');
            if bestFit is not None:
                plt.plot(self.plotx,bestFit,'r-')
            plt.draw()
        # Calculate the elapsed time since the last dead-center crossing in the same direction,
        # which is nominally 2 seconds, and the peak-to-peak angular amplitude of two consecutive
        # swings in opposite directions.
        if direction != self.lastDirection:
            if self.nextLastSamplesSinceBoot and self.lastOffset:
                period = (samplesSinceBoot - self.nextLastSamplesSinceBoot - self.lastOffset + offset)*self.args.adc_tick
            else:
                # We must have processed at least two frames before we can calculate a period.
                period = 0
            if self.lastAmplitude is not None:
                swing = amplitude + self.lastAmplitude
            else:
                swing = 0
        else:
            # We require alternating directions, so something went wrong but we will try to recover.
            period = -1
            swing = -1
        # update our saved info
        self.lastDirection = direction
        self.lastOffset = self.nextLastOffset
        self.nextLastOffset = offset
        self.nextLastSamplesSinceBoot = self.lastSamplesSinceBoot
        self.lastSamplesSinceBoot = samplesSinceBoot
        self.lastAmplitude = amplitude
        # remember this result
        if abs(period-2) < 0.1 and swing > 0:
            self.periods.append(period)
            self.swings.append(swing)
        # return the estimated period in seconds
        return period,swing,height

    def finish(self):
        periods = numpy.array(self.periods)
        swings = numpy.array(self.swings)
        if self.args.show_plots:
            if self.args.batch_replay:
                plt.clf()
                plt.subplot(2,1,1)
                mean = numpy.mean(periods)
                # calculate the going in ppm
                going = 1e6*(periods - mean)/mean
                plt.plot(going,'x')
                plt.subplot(2,1,2)
                plt.plot(swings,'x')
            self.fig.savefig('fit.png')
        numpy.savetxt('fit.dat',numpy.vstack([periods,swings]).T)

class DB(object):
    def __init__(self,args):
        # Set fetch constants
        self.ID              = u"_id"
        self.RAW             = u"raw"
        self.TIMESTAMP       = u"timestamp"
        self.CRUDE_PERIOD    = u"crudePeriod"
        self.TEMPLATE        = u"template"

        self.args = args
        # Connect to Mongod (fails and exits if mongod is not running)
        self.client = MongoClient('localhost', args.mongo_port)
        # Get db object
        self.db = self.client[args.db_name]
        # Get collection object
        self.dataCollection = self.db[args.collection_name]
        self.templateCollection = self.db[self.args.template_collection]

    def loadData(self):
        """
        Loads IR data from the database into the expected numpy format
        Crude Period
        IR
        IR...
        Returns the tuple (data,timestamp)
        """
        # Perform Query
        results = self.dataCollection.find({},{self.ID:False,self.TIMESTAMP:True, self.CRUDE_PERIOD:True, 
            self.RAW:True}).sort(self.TIMESTAMP, DESCENDING).limit(self.args.fetch_limit)
        # Construct numpy array
        data = numpy.array([], dtype=numpy.uint16)
        for document in results:
            data = numpy.append(data,document[self.CRUDE_PERIOD])
            data = numpy.append(data,document[self.RAW])
            # Get last timestamp
            timestamp = document[self.TIMESTAMP]
        return (data,timestamp)

    def saveTemplate(self,template,timestamp):
        """
        Saves a given template into the database
        """
        self.templateCollection.insert({self.TIMESTAMP:timestamp, self.TEMPLATE:template.T.tolist()})

    def loadTemplate(self):
        """
        Loads a given template from the database, returns the tuple (template,timestamp)
        """
        # Perform Query
        results = self.templateCollection.find({},{self.ID:False,self.TIMESTAMP:True, 
            self.TEMPLATE:True}).sort(self.TIMESTAMP, DESCENDING).limit(1)

        data = []
        templateTimestamp = None
        for document in results:
            templateTimestamp = document[self.TIMESTAMP]
            for pair in document[self.TEMPLATE]:
                data.append(pair)
        return (numpy.transpose(numpy.array(data)),templateTimestamp)

def main():

    # parse command-line args
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--replay', type=str, default='',
        help = 'name of input data file to replay')

    parser.add_argument('--from-db', action = 'store_true',
        help = 'fetch data from a Mongo database')
    parser.add_argument('--db-name', type=str, default='TickTock',
        help = 'name of Mongo database to fetch from')
    parser.add_argument('--collection-name', type=str, default='datapacketmodels',
        help = 'name of Mongo collection to fetch from')
    parser.add_argument('--template-collection', type = str, default = 'templatemodels',
        help = 'database collection where spline template should be saved and loaded')
    parser.add_argument('--fetch-limit', type=int, default=100,
        help = 'how many documents to fetch from Mongo database')
    parser.add_argument('--mongo-port', type=int, default=27017,
        help = 'port to use when connecting to a Mongo database')
    parser.add_argument('--batch-replay', action = 'store_true',
        help = 'no interactive prompting for each frame during replay')
    parser.add_argument('--max-frames', type = int, default = 0,
        help = 'maximum number of frames to replay (or no limit if zero)')
    parser.add_argument('--nsamples', type=int, default=3072,
        help = 'number of IR ADC samples per frame')
    parser.add_argument('--adc-tick', type = float, default = 832e-7,
        help = 'ADC sampling period in seconds')
    parser.add_argument('--nfingers', type = int, default = 5,
        help = 'number of fingers on the fiducial bob (')
    parser.add_argument('--length', type = float, default = 1020.,
        help = 'nominal length of pendulum to fiducial marker in milimeters')
    parser.add_argument('--width', type = float, default = 54.,
        help = 'nominal width of the fiducial marker in milimeters')
    parser.add_argument('--show-plots', action = 'store_true',
        help = 'display analysis plots')
    parser.add_argument('--show-centers', action = 'store_true',
        help = 'display center mark on analysis plots')
    parser.add_argument('--show-heights', action = 'store_true',
        help = 'display height mark on analysis plots')
    parser.add_argument('--physical', action = 'store_true',
        help = 'fit frames to a physical model')
    parser.add_argument('--spline', action = 'store_true',
        help = 'fit frames to a spline model')
    parser.add_argument('--save-template', type = str, default = None,
        help = 'filename where spline template should be saved')
    parser.add_argument('--load-template', type = str, default = None,
        help = 'filename of spline template to load and use')
    parser.add_argument('--nspline', type = int, default = 1024,
        help = 'number of spline knots used to build spline template')
    parser.add_argument('--spline-pad', type = float, default = 0.25,
        help = 'amount of padding to use for spline fit region')
    parser.add_argument('--verbose', action = 'store_true',
        help = 'generate verbose output')
    args = parser.parse_args()

    # define tab geometry
    tabs = numpy.array([[-27.,-19.],[-15.,-11.],[-7.,-3.],[3.,7.],[11.,15.],[23.,27.]])

    # initialize our database connection
    db = DB(args)

    # initialize our frame processor
    processor = FrameProcessor(tabs,args,db)

    # replay a pre-recorded data file if requested
    if args.replay or args.from_db:
        if args.from_db:
            dataTuple = db.loadData()
            data = dataTuple[0]
            timestamp = dataTuple[1]
        elif args.replay:
            # load the input data file
            data = numpy.loadtxt(args.replay)
        if len(data.shape) != 1:
            print 'Data has unexpected shape',data.shape
            return -1
        if args.verbose:
            print 'Read %d bytes from %s' % (len(data),args.replay)
        # loop over data frames
        nframe = len(data)/(1+args.nsamples)

        if args.max_frames == 0 or nframe <= args.max_frames:
            if len(data) % 1+args.nsamples:
                print 'WARNING: Ignoring extra data beyond last frame'
        else:
            print 'Will only replay %d of %d frames' % (args.max_frames,nframe)
            nframe = args.max_frames
        frames = data[:nframe*(1+args.nsamples)].reshape((nframe,1+args.nsamples))
        if args.save_template:
            template = buildSplineTemplate(frames,args)
            if args.save_template == "db":
                # Save to database as array of ordered pairs (arrays)
                db.saveTemplate(template,timestamp)
            else:
                # Save to file
                numpy.savetxt(args.save_template,template.T)
            if args.show_plots:
                plt.plot(template[0],template[1])
                plt.show()
            return 0
        for i,frame in enumerate(frames):
            samplesSinceBoot,samples = frame[0],frame[1:]
            try:
                period,swing,height = processor.process(samplesSinceBoot,samples)
                print 'Period = %f secs, swing = %f, height = %f (%d/%d)' % (period,swing,height,i,nframe)
                # check for more recent template
                processor.updateTemplate()
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
                    samplesSinceBoot = value
                    remaining = args.nsamples
                elif remaining > 0:
                    samples[args.nsamples - remaining] = value
                    remaining -= 1
                    if remaining == 0:
                        period,swing = processor.process(samplesSinceBoot,samples)
                        # send the calculated period to our STDOUT and flush the buffer!
                        print period,swing,height
                        sys.stdout.flush()
                        # check for more recent template
                        processor.updateTemplate()
            except Exception,e:
                # Try to keep going silently after any error
                pass

    # clean up
    processor.finish()

if __name__ == "__main__":
    main()
