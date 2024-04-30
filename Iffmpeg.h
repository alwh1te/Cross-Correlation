#pragma once
#ifndef CROSSCORRELATION_IFFMPEG_H
#define CROSSCORRELATION_IFFMPEG_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "libswresample/swresample.h"
#include "return_codes.h"
#define INITIAL_CONST 1048576;
typedef struct AudioData AudioData;
typedef struct AVData AVData;
int8_t read_packet(AVData *avData, AudioData *data);
int8_t extractSamples(AVData *avData1, AudioData *data1, AVData *avData2, AudioData *data2);
int8_t fill_zeroes(AudioData *data, int32_t size);
int8_t extractData(const char* file1, const char* file2, AudioData *data1, AudioData *data2, int32_t *maxSampleRate);
int8_t decode_packet(AVData *avData, AudioData *data);
int8_t init_audio_data(const char *file, AVData *data);
void close_audio_data(AVData *avData);
#endif	  // CROSSCORRELATION_IFFMPEG_H
