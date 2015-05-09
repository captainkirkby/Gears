from iminuit import Minuit
import scipy.interpolate
import numpy

class Template(object):

    def __init__(self,data,timestamp):
        self.template = scipy.interpolate.UnivariateSpline(data[0],data[1],k=3,s=0.)
        self.templateTimestamp = timestamp

    def fitTemplateModel(self,frame,args,direction,lo,hi,offset,rise,fall):
        # initial parameter guesses
        loGuess = lo
        rngGuess = hi-lo
        t0Guess = offset
        durationGuess = (rise[-1] - fall[0])/2.
        
        # initialize a vector of ADC sample countsaf,
        tadc = numpy.linspace(0.,args.nsamples-1.,args.nsamples)
    
        # initialize model prediction
        global prediction
        prediction = numpy.zeros_like(frame.samples)
    
        # define chi-square function to use
        def chiSquare(t0,duration,lo,rng):
            global prediction
            # convert this frame's ADC timing to s values, correcting for the direction of
            # travel, the estimate fiducial crossing time, and the stretch factor.
            svec = direction*(tadc - t0)/duration
            # evaluate self.template at these s values
            prediction[:] = 1.
            mask = numpy.abs(svec) < 1 + args.spline_pad
            prediction[mask] = self.template(svec[mask])
            # rescale from [0,1] to [lo,hi]
            prediction = lo + rng*prediction
            # evaluate the chi-square
            residuals = frame.samples - prediction
            # return numpy.dot(residuals,residuals)
            newResiduals = numpy.concatenate((residuals[:offset-200],residuals[offset+200:]))
            return numpy.dot(newResiduals,newResiduals)
    
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
    
        # return best-fit parameter values and best-fit model prediction
        return engine.args,prediction

if __name__ == "__main__":
    main()
