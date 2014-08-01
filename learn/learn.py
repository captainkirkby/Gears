#!/usr/bin/env python

import math
import numpy as np
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
    meanTemp = np.mean(dump['thermistor'][rng])
    tempVariation = dump['thermistor'][rng] - meanTemp
    meanPres = np.mean(dump['pressure'][rng])
    presVariation = dump['pressure'][rng] - meanPres
    meanHumid = np.mean(dump['humidity'][rng])
    humidVariation = dump['humidity'][rng] - meanHumid

    # Use a periodogram to find the dominant periodic frequencies in the signal.
    freq,psd = signal.periodogram(periodVariation,nfft=3*60*60,detrend='linear')
    maxPsd = np.max(psd)
    dominant = np.argsort(psd)[-args.nfreq:][::-1]
    print psd[dominant]
    print 1./freq[dominant]

    # Initialize plotting.
    fig = plt.figure(figsize=(12,12))
    fig.set_facecolor('white')

    # Plot the periodogram
    plt.subplot(2,1,1)
    plt.loglog(freq,psd)
    plt.ylim([1e-3*maxPsd,1.5*maxPsd])

    # Add grid lines
    ax = plt.gca()
    ticks = np.sort(freq[dominant])
    ax.set_xticks(ticks,minor=False)
    ax.xaxis.grid(True)
    ax.yaxis.grid(True)

    # Initialize periodic features for linear regression.
    t = np.arange(n)
    X = np.empty((n,2*args.nfreq+3))
    for i,f in enumerate(freq[dominant]):
        X[:,2*i+0] = np.sin(2*math.pi*f*t)
        X[:,2*i+1] = np.cos(2*math.pi*f*t)        
    # Add environmental monitoring features for linear regression.
    X[:,2*args.nfreq+0] = tempVariation
    X[:,2*args.nfreq+1] = presVariation
    X[:,2*args.nfreq+2] = humidVariation

    # Do the fit.
    ##model = linear_model.RidgeCV(alphas=[0.1, 1.0, 10.0])
    model = linear_model.LinearRegression()
    model.fit(X,periodVariation)
    periodFit = model.predict(X)

    # Look up the environmental coefficients.
    tempCoef,presCoef,humidCoef = model.coef_[-3:]
    print 'Temperature: mean = %.3f C, coef = %+.5f ppm/C' % (meanTemp,tempCoef)
    print 'Pressure: mean = %.3f Pa, coef = %+.5f ppm/Pa' % (meanPres,presCoef)
    print 'Humidity: mean = %.2f%%, coef = %+.5f ppm/%%' % (meanHumid,humidCoef)

    # Plot the fitted period data.
    plt.subplot(2,1,2)
    plt.xlabel('Elapsed Time (days)')
    plt.ylabel('Period Variation (ppm)')
    plt.plot(sampleDays,periodVariation,'.')
    plt.plot(sampleDays,periodFit,'-')

    plt.show()

def main():
    # parse command-line args
    import argparse
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--verbose', action = 'store_true',
        help = 'generate verbose output')
    parser.add_argument('-i','--input', type=str, default='/Volumes/Data/clock/clockData.npy',
        help = 'name of converted database dump file to read')
    parser.add_argument('-n','--nsamples', type=int, default=0,
        help = 'number of samples to analyze (or zero for all)')
    parser.add_argument('--nskip', type = int, default = 2,
        help = 'number of initial samples to skip')
    parser.add_argument('--nominal-period', type = float, default = 2.0,
        help = 'nominal pendulum period in seconds')
    parser.add_argument('--nfreq', type = int, default = 10,
        help = 'number of periodic frequencies to fit for')
    args = parser.parse_args()
    # do the analysis
    analyze(args)

if __name__ == "__main__":
    main()
