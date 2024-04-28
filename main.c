#include "Iffmpeg.h"
#include "Ifftw.h"
#include "return_codes.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	av_log_set_level(AV_LOG_QUIET);
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "Usage: %s <file1> [file2]\n", argv[0]);
		return ERROR_CANNOT_OPEN_FILE;
	}
	const char *file1 = argv[1];
	const char *file2 = argv[1];
	int channel = 1;
	if (argc == 3)
	{
		file2 = argv[2];
		channel = 0;
	}

	int response = 0;

	AudioData *data1 = malloc(sizeof(AudioData));
	AudioData *data2 = malloc(sizeof(AudioData));
	response += init_audio_data(file1, data1);
	response += init_audio_data(file2, data2);

	if (response)
	{
		fprintf(stderr, "Cant allocate memory\n");
		return ERROR_NOTENOUGH_MEMORY;
	}

	int maxSampleRate = data1->sample_rate > data2->sample_rate ? data1->sample_rate : data2->sample_rate;

	response += extractData(data1, channel, maxSampleRate);
	response += extractData(data2, channel, maxSampleRate);

	if (response)
	{
		fprintf(stderr, "Invalid data format\n");
		return ERROR_FORMAT_INVALID;
	}

	fftw_complex *dataComplex1 = fftw_alloc_complex(data1->size + data2->size);
	fftw_complex *dataComplex2 = fftw_alloc_complex(data1->size + data2->size);
	perform_fft(data1, dataComplex1, data1->size + data2->size);
	perform_fft(data2, dataComplex2, data1->size + data2->size);
	fftw_complex *corr = fftw_alloc_complex(((data1->size + data2->size) / 2) + 1);
	for (int i = 0; i < (((data1->size + data2->size) / 2) + 1); ++i)
	{
		corr[i][0] = dataComplex1[i][0] * dataComplex2[i][0] + dataComplex1[i][1] * dataComplex2[i][1];
		corr[i][1] = dataComplex1[i][1] * dataComplex2[i][0] - dataComplex1[i][0] * dataComplex2[i][1];
	}
	double *out = malloc(sizeof(double) * (data1->size + data2->size));
	perform_ifft(corr, out, data1->size + data2->size);
	double max_value = 0;
	int max_position = 0;
	for (int i = 0; i < (data1->size + data2->size); i++)
	{
		if (out[i] > max_value)
		{
			max_value = out[i];
			max_position = i;
		}
	}
	int sample_rate = maxSampleRate;
	double time_shift;
	if (max_position > (data1->size + data2->size) / 2)
	{
		max_position = (data1->size + data2->size) - max_position;
		max_position *= -1;
	}
	time_shift = (double) (max_position * 1000) / sample_rate;
	printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", max_position, sample_rate, (int)(time_shift));
//	free(data1->samples);
//	free(data2->samples);
	close_audio_data(data1);
	close_audio_data(data2);
	fftw_free(dataComplex1);
	fftw_free(dataComplex2);
	fftw_free(corr);
	free(out);
//	close_audio_data(data1);
//	close_audio_data(data2);
	return SUCCESS;
}
