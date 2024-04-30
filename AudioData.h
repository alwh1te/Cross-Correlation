#pragma once
#ifndef CROSSCORRELATION_AUDIODATA_H
#define CROSSCORRELATION_AUDIODATA_H

#include "Iffmpeg.h"

typedef struct AudioData
{
//	AVFormatContext *formatCtx;
//	AVCodecContext *codecCtx;
//	AVPacket *packet;
	double *samples;
	int size;
	int arrSize;
} AudioData;

typedef struct AVData {
	AVFormatContext *formatCtx;
	const AVCodec *codec;
	AVCodecContext *codecCtx;
	AVPacket *packet;
	AVFrame *frame;
	int sample_rate;
	int audioStreamIndex;
	int channel;
} AVData;

#endif	  // CROSSCORRELATION_AUDIODATA_H
