#!/usr/bin/env python

import datetime
import dateutil.parser
import numpy as np
import os.path

# Define the UTC timezone.
ZERO = datetime.timedelta(0)
class UTC(datetime.tzinfo):
    """UTC"""
    def utcoffset(self, dt):
        return ZERO
    def tzname(self, dt):
        return "UTC"
    def dst(self, dt):
        return ZERO
utc = UTC()
# Define the origin of unix time stamps.
tzero = datetime.datetime(1970,1,1,tzinfo=utc)
# Prepare a converter from ISO date strings to unix time stamps.
def isoParse(isoString):
    when = dateutil.parser.parse(isoString)
    return (when - tzero).total_seconds()

# Load the database dump to analyze.
inputName = '/Volumes/Data/clock/sampleData.csv'
dump = np.genfromtxt(inputName,names=True,delimiter=',',
    converters={7:isoParse})

# Save dump as a binary numpy array.
baseName,ext = os.path.splitext(inputName)
outputName = baseName + '.npy'
np.save(outputName,dump)
