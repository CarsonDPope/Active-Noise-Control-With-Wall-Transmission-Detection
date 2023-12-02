#!/usr/bin/env python
# coding: utf-8

# In[1]:


# Generate Broadband Noise for training
# Author: Carson Pope
# Last Updated: 11/12/2023

# Based on code from StackOverflow further based on Matlab code
# https://stackoverflow.com/a/35091295
# answered by FrankZalkow, edited by Eric Johnson


import numpy as np
import sys
import csv


# In[2]:


def fftnoise(f):
    f = np.array(f, dtype='complex')
    Np = (len(f) - 1) // 2
    phases = np.random.rand(Np) * 2 * np.pi
    phases = np.cos(phases) + 1j * np.sin(phases)
    f[1:Np+1] *= phases
    f[-1:-1-Np:-1] = np.conj(f[1:Np+1])
    return np.fft.ifft(f).real


# In[3]:


def band_limited_noise(min_freq, max_freq, samples=1024, samplerate=1):
    freqs = np.abs(np.fft.fftfreq(samples, 1/samplerate))
    f = np.zeros(samples)
    idx = np.where(np.logical_and(freqs>=min_freq, freqs<=max_freq))[0]
    f[idx] = 1
    return fftnoise(f)


# In[12]:


numSamples = 1200

Noise = np.zeros((101,numSamples))

SampleIdx = np.zeros(numSamples)


#Below is where to set the band frequencies
#each 'i'th entry into the sample corresponds to an 'i'th bandpass noise
# So the first noise generated is from frequencies 0-600 Hz
lowBand = [0, 300, 700]
highBand = [600, 699, 20000]

for i in range(3):
    for j in range(400):
        C1 = band_limited_noise(lowBand[i],highBand[i] , 100, 48000)
        C1 = np.int8(C1 * (2**15 - 1))
        SampleIdx[j+i*400] = j+i*400
        Noise[0][j+i*400] = i
        for h in range(100):
            Noise[h+1][j+i*400] = C1[h]
            


# In[14]:


# writing to csv file  
with open('CapstoneMachineLearningTrainingInputs.csv', 'w') as csvfile:  
    # creating a csv writer object  
    csvwriter = csv.writer(csvfile)  
        
    # writing the fields  
    csvwriter.writerow(SampleIdx)  
        
    # writing the data rows  
    csvwriter.writerows(Noise)

