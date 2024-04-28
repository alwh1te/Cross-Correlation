#pragma once
#ifndef CROSSCORRELATION_IFFMPEG_H
#define CROSSCORRELATION_IFFMPEG_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "libswresample/swresample.h"
typedef struct AudioData AudioData;
int extractData(AudioData *data, int channel, int maxSampleRate);
int init_audio_data(const char *file, AudioData *data);
void close_audio_data(AudioData *data);
#endif	  // CROSSCORRELATION_IFFMPEG_H
