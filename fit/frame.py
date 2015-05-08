import numpy
from scipy.stats import linregress

class Frame(object):
    def __init__(self,samples):
        self.samples = samples

    def padded(self,samples,whalf):
        return numpy.r_[[samples[0]]*whalf,samples,[samples[-1]]*whalf]

    def kernel(self,wlen):
        return numpy.ones(wlen,'d')/wlen

    def runningAvg(self,samples,wlen=31):
        # returns a running-average smoothing of self.samples
        assert samples.size > wlen and wlen % 2 == 1
        cumulativeSum = numpy.cumsum(numpy.insert(self.padded(samples,(wlen-1)/2),0,0))
        return (cumulativeSum[wlen:] - cumulativeSum[:-wlen])/float(wlen)

    def findNMaxes(self,hist,npeaks=3,sharpThreshold=20,levelThreshold=10):
        """
        Given a histogram and the expected number of peaks, returns an array of
        the approximate center of each of the peaks.
        """
        lastE = 0
        # Define shape (n,2)
        largest = numpy.zeros((0,2),dtype='int16')
        # Loop over elements in the differences of the histogram of the smoothed graph
        count = 0
        for i,e in enumerate(self.runningAvg(numpy.diff(hist),wlen=3)):
            # Looking for a sharp sign change
            if (e <= 0 and lastE >= 0) or (e >= 0 and lastE <= 0):
                if lastE-e > sharpThreshold:
                    # Add potential candidates to an array
                    largest = numpy.append(largest,[[e-lastE,i]],axis=0)
                    count = count + 1
            lastE = e
        # If greater than n choices, choose the best n
        if count > npeaks:
            # Get rid of any indices that are too close to each other
            nDeleted = 0
            for index,d in enumerate(numpy.diff(largest[:,1])):
                if d < levelThreshold:
                    # Collapse two rows into one
                    largest = numpy.delete(largest,index+1-nDeleted,axis=0)
                    nDeleted = nDeleted + 1
        # Choose indices of best n candidates
        return largest[numpy.argsort(largest[:,0])][:,1][0:npeaks]
    
    def findPeakValues(self,smooth,npeaks=3,window=10):
        """
        Given a smoothed array of data with 3 discrete levels (low-mid-hi) returns a sorted
        tuple low,mid,hi
        """
        hist = numpy.histogram(smooth,bins=1023,range=(0,1023))
        # Average values
        averages = numpy.zeros(npeaks)
        i = 0
        for max in self.findNMaxes(hist[0],npeaks=npeaks):
            lowerBound = max - window
            upperBound = max + window
            # Bounds Checking
            if lowerBound < 0:
                lowerBound = 0
            if upperBound > len(hist[0]):
                upperBound = len(hist[0])
            averages[i] = numpy.average(hist[1][lowerBound:upperBound],weights=hist[0][lowerBound:upperBound])
            i = i+1
        return tuple(numpy.sort(averages))
    
    def findRiseAndFallPositions(self,smooth,lo,mid,hi,nfingers=5,threshold=2):
        riseMask = numpy.zeros_like(smooth[1:])
        fallMask = numpy.zeros_like(smooth[1:])
        # Create boolean Mask where we expect falling edges
        booleanValue = 1
        for index,val in enumerate(smooth[1:]):
            if abs(val-mid) < threshold and booleanValue is 0:
                booleanValue = 1
            elif abs(val+mid) < threshold and booleanValue is 1:
                booleanValue = 0
            fallMask[index] = booleanValue
        # Create boolean Mask where we expect rising edges
        booleanValue = 1
        for index,val in enumerate(smooth[1:][::-1]):
            if abs(val-mid) < threshold and booleanValue is 0:
                booleanValue = 1
            elif abs(val+mid) < threshold and booleanValue is 1:
                booleanValue = 0
            riseMask[-index] = booleanValue
        # Get Rising Edges
        rising = numpy.logical_and(numpy.logical_and(smooth[:-1] <= 0,smooth[1:] > 0),riseMask)
        nrise = numpy.count_nonzero(rising)
        risePos = numpy.sort(numpy.argsort(rising)[-1*nrise:])
        # Get Falling Edges
        falling = numpy.logical_and(numpy.logical_and(smooth[:-1] > 0, smooth[1:] <= 0),fallMask)
        nfall = numpy.count_nonzero(falling)
        fallPos = numpy.sort(numpy.argsort(falling)[-1*nfall:])
        # Raise Exception if there is a problem
        if nrise != nfingers:
            raise RuntimeError("findRiseAndFallPositions: expected %d rising edges but found %d" % (nfingers, nrise))
        if nfall != nfingers:
            raise RuntimeError("findRiseAndFallPositions: expected %d falling edges but found %d" % (nfingers, nfall))
        return risePos,fallPos
    
    def lineFit(self,y,t1,t2):
        """
        Performs a linear fit to determine the value of t where y(t) crosses zero
        using samples y[t1:t2].
        """
        t = numpy.arange(t1,t2)
        slope, intercept, r_value, p_value, std_err = linregress(t,y[t1:t2])
        return -intercept/slope

    def getDirection(self,risePos,fallPos,nfingers=5):
        if risePos[0] - fallPos[0] > self.samples.size/(2.0 * nfingers):
            direction = +1.
        else:
            direction = -1.
        return direction

    def normalizeWithCenter(self,direction,t0,riseFit,fallFit):
        if direction == +1:
            riseFit -= t0
            fallFit -= t0
        elif direction == -1:
            tmp = numpy.copy(riseFit)
            riseFit = (t0 - fallFit)[::-1]
            fallFit = (t0 - tmp)[::-1]
        return riseFit,fallFit
    
    def quickFit(self,args,smoothing=15,fitsize=5,avgWindow=50):
        """
        Attempts a quick fit of the specified sample data or returns a RuntimeError.
        Returns the direction (+/-1) of travel, the estimated lo and hi ADC levels,
        the estimated fiducial crossing time t0 relative to the first sample, and
        the estimated rise and fall times relative to the fiducial and corrected for
        the direction of travel. All times are measured in ADC samples.
        """
        # perform a running-average smoothing of the frame's raw sample data
        smooth = self.runningAvg(self.samples,wlen=1+2*smoothing)
        # Find the range of the smoothed data. Use the min of the smooth samples to estimate the
        # lo value. Use the mean of the left and right margins to estimate the hi value. The
        # reason why don't use the max of the smooth samples to estimate the hi value is that we
        # observe some peaking (transmission > 1) near the edges.
        lo,height,hi = self.findPeakValues(smooth)
        # find edges as points where the smoothed data crosses the midpoints between lo,hi
        midpt = 0.5*(lo+hi)
        smooth -= midpt
        risePos,fallPos = self.findRiseAndFallPositions(smooth,lo,midpt,hi,nfingers=args.nfingers)
        # perform linear fits to locate each edge to subsample precision
        riseFit = numpy.empty((args.nfingers,))
        fallFit = numpy.empty((args.nfingers,))
        for i in range(args.nfingers):
            riseFit[i] = self.lineFit(smooth,risePos[i]-fitsize,risePos[i]+fitsize+1)
            fallFit[i] = self.lineFit(smooth,fallPos[i]-fitsize,fallPos[i]+fitsize+1)
        # t0 is the time between the 3rd falling and the 3rd rising edge
        t0 = (fallFit[2]+riseFit[2])/2
        # use the distance between the first falling and rising edges to 
        # discriminate between the two possible directions of travel
        direction = self.getDirection(risePos,fallPos,nfingers=args.nfingers)
        # calculate edge times relative to the fiducial, corrected for the direction of travel.
        riseFit,fallFit = self.normalizeWithCenter(direction,t0,riseFit,fallFit)
        if args.verbose:
            print direction,lo,hi,t0,riseFit,fallFit,height
        return direction,lo,hi,t0,riseFit,fallFit,height

if __name__ == "__main__":
    main()
