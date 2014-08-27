#!/usr/bin/env python

import math
import numpy as np
import pandas as pd
import scipy.interpolate
import matplotlib.pyplot as plt

from scipy import signal
from sklearn import linear_model

def analyzeOne(y,nfreq,nfft=3*60*60):
    # Use a periodogram to find the dominant periodic frequencies in the signal.
    freq,psd = signal.periodogram(y,nfft,detrend='linear')
    maxPsd = np.max(psd)
    dominant = np.argsort(psd)[-nfreq:][::-1]

def analyze(args):
    # Load the database dump.
    dump = np.load(args.input)

    # Determine the range of samples to analyze.
    n = args.nsamples
    if n == 0:
        n = len(dump) - args.nskip
    rng = slice(args.nskip,args.nskip+n)

    # Build a sample number array in units of days.
    sampleDays = np.arange(n)/(24.*60.*60.)

    # Calculate the period variation in ppm.
    periodVariation = 1e6*(dump['refinedPeriod'][rng] - args.nominal_period)
    nanMask = np.isnan(periodVariation)
    if np.any(nanMask):
        print 'Found %d missing values in periodVariation' % np.count_nonzero(nanMask)
        return

    # Calculate the amplitude variation in arcmins.
    meanAmplitude = np.mean(dump['angle'][rng])
    print 'Mean swing amplitude is %.3f deg' % meanAmplitude
    amplitudeVariation = 60.*(dump['angle'][rng] - meanAmplitude)

    # Calculate mean-subtracted variations of environmental parameters.
    temp = dump['blockTemperature'][rng]
    meanTemp = np.mean(temp)
    rmsTemp = np.std(temp)
    tempVariation = temp - meanTemp
    pres = dump['pressure'][rng]
    meanPres = np.mean(pres)
    rmsPres = np.std(pres)
    presVariation = pres - meanPres
    humid = dump['humidity'][rng]
    meanHumid = np.mean(humid)
    rmsHumid = np.std(humid)
    humidVariation = humid - meanHumid

    # Smooth the humidity signal, if requested.
    if args.humidity_smoothing > 0:
        humidSpline = scipy.interpolate.UnivariateSpline(sampleDays,humidVariation,
            s=args.humidity_smoothing*n)
        humidVariation = humidSpline(sampleDays)

    what = periodVariation
    ##what = amplitudeVariation

    # Calculate filtered signal, to remove most of the gear train errors.
    print 'filtering...'
    series = pd.Series(what,index=sampleDays)
    filtered = pd.rolling_mean(series,window=60*60,center=True)

    # Use a periodogram to find the dominant periodic frequencies in the signal.
    freq,psd = signal.periodogram(what,nfft=3*60*60,detrend='linear')
    maxPsd = np.max(psd)
    dominant = np.argsort(psd)[-args.nfreq:][::-1]
    print psd[dominant]
    print 1./freq[dominant]

    # Initialize plotting.
    fig = plt.figure(figsize=(12,12))
    fig.set_facecolor('white')

    '''
    # Plot the periodogram
    plt.subplot(2,2,1)
    plt.loglog(freq,psd)
    plt.ylim([1e-3*maxPsd,1.5*maxPsd])

    # Add grid lines
    ax = plt.gca()
    ticks = np.sort(freq[dominant])
    ax.set_xticks(ticks,minor=False)
    ax.xaxis.grid(True)
    ax.yaxis.grid(True)
    '''

    # Initialize periodic features for linear regression.
    nenv = 4
    t = np.arange(n)
    X = np.empty((n,2*args.nfreq+nenv))
    for i,f in enumerate(freq[dominant]):
        X[:,2*i+0] = np.sin(2*math.pi*f*t)
        X[:,2*i+1] = np.cos(2*math.pi*f*t)        
    # Add environmental monitoring features for linear regression.
    X[:,2*args.nfreq+0] = tempVariation
    X[:,2*args.nfreq+1] = presVariation
    X[:,2*args.nfreq+2] = humidVariation
    X[:,2*args.nfreq+3] = 0.5*tempVariation**2

    # Do the fit.
    model = linear_model.LinearRegression()
    ##model = linear_model.Ridge(alpha=0.5)
    ##model = linear_model.RidgeCV(alphas=[0.1, 1.0, 10.0])
    ##model = gaussian_process.GaussianProcess(theta0=1e-1,nugget=1e-3,storage_mode = 'light')
    model.fit(X,what)
    bestFit = model.predict(X)
    ##bestFit,fitErrors = model.predict(x, eval_MSE=False)

    # Look up the environmental coefficients.
    tempCoef,presCoef,humidCoef,tempCoef2 = model.coef_[-nenv:]
    print 'Temperature: mean = %.3f C, rms = %.3f C, coef = %+.5f ppm/C, coef2 = %+.5f ppm/C^2' % (
        meanTemp,rmsTemp,tempCoef,tempCoef2)
    print 'Pressure: mean = %.3f Pa, rms = %.3f Pa, coef = %+.5f ppm/Pa' % (
        meanPres,rmsPres,presCoef)
    print 'Humidity: mean = %.2f%%, rms = %.2f%%, coef = %+.5f ppm/%%' % (
        meanHumid,rmsHumid,humidCoef)

    # Plot the fitted period data.
    plt.subplot(3,1,1)
    plt.xlabel('Elapsed Time (secs)')
    plt.ylabel('Period Variation (ppm)')
    nsub = 600
    tsec = np.arange(nsub)
    plt.plot(tsec,what[:nsub],'.')
    plt.plot(tsec,bestFit[:nsub],'-')

    Xenv = np.copy(X)
    Xenv[:,:-4] = 0.
    envFit = model.predict(Xenv)

    plt.subplot(3,1,2)
    plt.xlabel('Elapsed Time (days)')
    plt.ylabel('Environmental Variations (ppm)')
    plt.plot(sampleDays,filtered,label='data')
    plt.plot(sampleDays,envFit,label='all')
    plt.plot(sampleDays,tempCoef*tempVariation,label='temp')
    plt.plot(sampleDays,presCoef*presVariation,label='pres')
    plt.plot(sampleDays,humidCoef*humidVariation,label='humid')
    plt.legend()

    noise = what - bestFit
    plt.subplot(3,1,3)
    #plt.xlabel('Elapsed Time (days)')
    #plt.ylabel('Fit Residuals (ppm)')
    #plt.plot(sampleDays,noise,'.')
    plt.hist(noise,bins=100,range=(-400.,+400.))
    plt.xlabel('Fit Residuals (ppm)')

    plt.savefig('learn.png')
    plt.show()

def main():
    # parse command-line args
    import argparse
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--verbose', action = 'store_true',
        help = 'generate verbose output')
    parser.add_argument('-i','--input', type=str, default='/Volumes/Data/clock/sampleData.npy',
        help = 'name of converted database dump file to read')
    parser.add_argument('-n','--nsamples', type=int, default=0,
        help = 'number of samples to analyze (or zero for all)')
    parser.add_argument('--nskip', type = int, default = 0,
        help = 'number of initial samples to skip')
    parser.add_argument('--nominal-period', type = float, default = 2.0,
        help = 'nominal pendulum period in seconds')
    parser.add_argument('--humidity-smoothing', type = float, default = 0.01,
        help = 'amount of smoothing to apply to raw humidity data (or zero for none)')
    parser.add_argument('--nfreq', type = int, default = 50,
        help = 'number of periodic frequencies to fit for')
    args = parser.parse_args()
    # do the analysis
    analyze(args)

if __name__ == "__main__":
    main()
