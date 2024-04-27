#pragma once
#ifndef CROSSCORRELATION_AUDIODATA_H
#define CROSSCORRELATION_AUDIODATA_H

#include "Iffmpeg.h"

typedef struct AudioData
{
	AVFormatContext *formatCtx;
	AVCodecContext *codecCtx;
	AVPacket *packet;
	double *samples;
	int audioStreamIndex;
	int size;
	int sample_rate;
} AudioData;

#endif	  // CROSSCORRELATION_AUDIODATA_H
