#pragma once
#ifndef CROSSCORRELATION_IFFTW_H
#define CROSSCORRELATION_IFFTW_H
#include "AudioData.h"
#include "fftw3.h"
void perform_fft(AudioData *input, fftw_complex *output, int size);
void perform_ifft(fftw_complex *input, double *out, int size);
#endif	  // CROSSCORRELATION_IFFTW_H
