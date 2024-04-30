#pragma once
#ifndef CROSSCORRELATION_IFFTW_H
#define CROSSCORRELATION_IFFTW_H
#include "fftw3.h"
void perform_fft(double *input, fftw_complex *output, int64_t size);
void perform_ifft(fftw_complex *input, double *out, int64_t size);
#endif	  // CROSSCORRELATION_IFFTW_H
