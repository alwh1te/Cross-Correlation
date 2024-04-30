#include "Ifftw.h"

void perform_fft(double *input, fftw_complex *output, int64_t size)
{
	fftw_plan plan = fftw_plan_dft_r2c_1d(size, input, output, FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}

void perform_ifft(fftw_complex *input, double *out, int64_t size)
{
	fftw_plan plan = fftw_plan_dft_c2r_1d(size, input, out, FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}
