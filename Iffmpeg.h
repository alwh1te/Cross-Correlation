#pragma once
#ifndef CROSSCORRELATION_IFFMPEG_H
#define CROSSCORRELATION_IFFMPEG_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "libswresample/swresample.h"
#include "return_codes.h"
#define INITIAL_CONST 1048576
typedef struct AudioData
{
	AVFormatContext *formatCtx;
	const AVCodec *codec;
	AVCodecContext *codecCtx;
	AVPacket *packet;
	AVFrame *frame;
	int sample_rate;
	int audioStreamIndex;
	int channel;
	double *samples;
	int size;
	int arrSize;
} AudioData;
int8_t read_packet(AudioData *data);
int8_t increase_array_size(AudioData *data);
int8_t extractSamples(AudioData *data1, AudioData *data2, const int32_t *maxSampleRate);
int8_t fill_zeroes(AudioData *data, int32_t size);
int8_t extractData(const char* file1, const char* file2, AudioData *data1, AudioData *data2, int32_t *maxSampleRate);
int8_t decode_packet(AudioData *data);
int8_t init_audio_data(const char *file, AudioData *data);
void resample(AudioData *data, int32_t maxSampleRate);
void init_data_struct(AudioData *data);
void close_av_data(AudioData *data);
void close_audio_data(AudioData *data);
#endif	  // CROSSCORRELATION_IFFMPEG_H
