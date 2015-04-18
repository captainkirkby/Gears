from iminuit import Minuit
import numpy

class Model(object):

    def edgeTransmission(self,x,D):
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
    
    def tabsTransmission(self,x,tabs,D):
        """
        Returns the transmission function (0-1) of a sequence of tabs defined by edges at (x1,x2)
        assuming a beam of diameter D. The input x can be a numpy array. All units are arbitrary but
        must be the same.
        """
        result = numpy.ones_like(x)
        for (x1,x2) in tabs:
            result += edgeTransmission(x-x1,D) - edgeTransmission(x-x2,D)
        return result
    
    def motion(self,dt,L,dtheta,T):
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
    
    def physicalModel(self,t0,direction,lo,hi,tabs,D=2.,L=1108.,dtheta=4.66,T=2.0,nsamples=1024,adcTick=1664e-7):
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
    
    def fitPhysicalModel(self,samples,tabs,args,direction,lo,hi,offset):
    
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


if __name__ == "__main__":
    main()
    