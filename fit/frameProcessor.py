import numpy
import math

import model
import template

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
        self.mostRecentTemplateTimestamp = None
        # we use python arrays here since we do not know the final size in advance
        # and they are more time efficient for appending than numpy arrays (but
        # less space efficient)
        self.periods = [ ]
        self.swings = [ ]
        # initialize plot display if requested
        if self.args.show_plots:
            self.fig = plt.figure(figsize=(8,8))
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
                self.template = template.Template(templateData,self.mostRecentTemplateTimestamp)
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
        if self.args.verbose:
            print "Attempting to Update Template"
        if self.args.load_template == "db":
            # Try and create a template if one doesn't exist
            if self.template is None:
                # Try and create one
                dataTuple = self.db.loadData()
                data = dataTuple[0]
                timestamp = dataTuple[1]
                if len(data.shape) != 1:
                    if self.args.verbose:
                        print "Bad data shape!"
                    return
                # loop over data frames
                nframe = len(data)/(1+self.args.nsamples)
                if(nframe < self.args.fetch_limit):
                    if self.args.verbose:
                        print "Insufficient Data to update template!"
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
                self.template = template.Template(templateData,templateTimestamp)
                self.mostRecentTemplateTimestamp = templateTimestamp



    def process(self,samplesSinceBoot,frame):
        """
        Processes the next frame of raw IR ADC samples. The parameter samplesSinceBoot gives the number
        of ADC samples elapsed between the first sample of this frame and the last boot packet recieved, 
        or zero if this information is not available. Returns the estimated
        period in seconds (nominally 2 secs) or raises a RuntimeError.
        """
        if len(frame.samples) != self.args.nsamples:
            # Something is seriously wrong.
            return -2,-2,-2
        if self.args.load_template and self.template is None:
            return -3,-3,-3
        # always start with a quick fit
        direction,lo,hi,offset,rise,fall,height = frame.quickFit(self.args)
        if self.args.physical:
            fitParams,bestFit = model.Model().fitPhysicalModel(frame,self.tabs,self.args,direction,lo,hi,offset)
            offset,amplitude = fitParams[0],fitParams[5]
        elif self.template is not None:
            fitParams,bestFit = self.template.fitTemplateModel(frame,self.args,
                direction,lo,hi,offset,rise,fall)
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
            plt.plot(self.plotx,frame.samples,'g+')
            if self.args.show_centers:
                plt.plot(numpy.zeros(100)+offset,numpy.linspace(0,2**10,100),'b-');
            if self.args.show_levels:
                plt.plot(numpy.linspace(0,self.args.nsamples,100),numpy.zeros(100)+lo,'y-');
                plt.plot(numpy.linspace(0,self.args.nsamples,100),numpy.zeros(100)+height,'y-');
                plt.plot(numpy.linspace(0,self.args.nsamples,100),numpy.zeros(100)+hi,'y-');
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

if __name__ == "__main__":
    main()
