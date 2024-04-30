#include "Iffmpeg.h"

#include "AudioData.h"

int8_t init_audio_data(const char *file, AVData *data)
{
	data->formatCtx = avformat_alloc_context();
	if (!data->formatCtx)
	{
		fprintf(stderr, "ERROR: could not allocate memory for Format Context");
		return ERROR_NOTENOUGH_MEMORY;
	}
	if (avformat_open_input(&data->formatCtx, file, NULL, NULL) != 0 || avformat_find_stream_info(data->formatCtx, NULL) < 0)
	{
		fprintf(stderr, "It was not possible to open the file and get samples from there");
		return ERROR_CANNOT_OPEN_FILE;
	}
	data->audioStreamIndex = av_find_best_stream(data->formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (data->audioStreamIndex < 0)
	{
		fprintf(stderr, "The audio stream could not be found");
		return ERROR_FORMAT_INVALID;
	}
	data->sample_rate = data->formatCtx->streams[data->audioStreamIndex]->codecpar->sample_rate;
	data->codec = avcodec_find_decoder(data->formatCtx->streams[data->audioStreamIndex]->codecpar->codec_id);
	if (data->codec == NULL)
	{
		fprintf(stderr, "Couldn't find a suitable decoder");
		return ERROR_FORMAT_INVALID;
	}
	data->codecCtx = avcodec_alloc_context3(data->codec);
	if (!data->codecCtx)
	{
		fprintf(stderr, "Failed to allocate memory for codec initialization");
		return ERROR_NOTENOUGH_MEMORY;
	}
	if (avcodec_parameters_to_context(data->codecCtx, data->formatCtx->streams[data->audioStreamIndex]->codecpar) < 0 ||
		avcodec_open2(data->codecCtx, data->codec, NULL) < 0)
	{
		fprintf(stderr, "Failed to convert codec parameters and open it");
		return ERROR_FORMAT_INVALID;
	}
	data->packet = av_packet_alloc();
	data->frame = av_frame_alloc();
	if (data->packet == NULL || data->frame == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for packets and frames");
		return ERROR_NOTENOUGH_MEMORY;
	}
	return SUCCESS;
}

int8_t increase_array_size(AVData *avData, AudioData *data)
{
	if (data->size + avData->frame->nb_samples > data->arrSize - 1)
	{
		data->arrSize += avData->frame->nb_samples * 100;
		double *tmp = realloc(data->samples, data->arrSize * sizeof(double));
		data->samples = tmp;
		if (data->samples == NULL)
		{
			fprintf(stderr, "Memory allocation for reading samples failed");
			return ERROR_NOTENOUGH_MEMORY;
		}
	}
	return SUCCESS;
}

int8_t decode_packet(AVData *avData, AudioData *data)
{
	int32_t response = avcodec_send_packet(avData->codecCtx, avData->packet);
	if (response < 0)
	{
		fprintf(stderr, "Error while sending a packet to the decoder");
		return ERROR_FORMAT_INVALID;
	}
	while (1)
	{
		response = avcodec_receive_frame(avData->codecCtx, avData->frame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
		{
			break;
		}
		else if (response < 0)
		{
			fprintf(stderr, "Error while receiving a frame from the decoder");
			return ERROR_FORMAT_INVALID;
		}
		response += increase_array_size(avData, data);
		if (response)
		{
			return response;
		}
		for (int32_t i = 0; i < avData->frame->nb_samples; ++i)
		{
			data->samples[(data->size)++] = (double)avData->frame->data[avData->channel][i];
		}
		av_frame_unref(avData->frame);
	}
	return SUCCESS;
}

int8_t read_packet(AVData *avData, AudioData *data)
{
	while (av_read_frame(avData->formatCtx, avData->packet) >= 0)
	{
		if (avData->packet->stream_index == avData->audioStreamIndex)
		{
			int8_t response = decode_packet(avData, data);
			if (response)
			{
				return response;
			}
		}
		av_packet_unref(avData->packet);
	}
	return 0;
}

int8_t extractSamples(AVData *avData1, AudioData *data1, AVData *avData2, AudioData *data2)
{
	data1->arrSize = INITIAL_CONST;
	data1->size = 0;
	data1->samples = malloc(data1->arrSize * sizeof(double));

	data2->arrSize = INITIAL_CONST;
	data2->size = 0;
	data2->samples = malloc(data2->arrSize * sizeof(double));

	int8_t response;
	response = read_packet(avData1, data1);
	response += read_packet(avData2, data2);

	if (response)
	{
		close_audio_data(avData1);
		close_audio_data(avData1);
		return response;
	}

	return SUCCESS;
}

void close_audio_data(AVData *avData)
{
	if (avData != NULL)
	{
		if (avData->formatCtx != NULL)
		{
			avformat_free_context(avData->formatCtx);
		}
		if (avData->codecCtx != NULL)
		{
			avcodec_free_context(&avData->codecCtx);
		}
	}
}

int8_t fill_zeroes(AudioData *data, int32_t size)
{
	if (data->arrSize < size)
	{
		data->arrSize = size;
		double *tmp = realloc(data->samples, data->arrSize * sizeof(double));
		data->samples = tmp;
		if (data->samples == NULL)
		{
			fprintf(stderr, "It was not possible to allocate memory for filling the array with zeros");
			return ERROR_NOTENOUGH_MEMORY;
		}
	}
	while (data->size < size)
	{
		data->samples[data->size++] = 0;
	}
	return SUCCESS;
}

int8_t extractData(const char *file1, const char *file2, AudioData *data1, AudioData *data2, int32_t *maxSampleRate)
{
	int8_t response;
	AVData avData1;
	AVData avData2;
	response = init_audio_data(file1, &avData1);
	if (response)
	{
		return 1;
	}
	avData1.channel = 0;
	if (file2)
	{
		avData2.channel = 0;
	}
	else
	{
		// TODO check if nb.channels < channel
		avData2.channel = 1;
	}
	response = init_audio_data(file2, &avData2);

	if (response)
	{
		return response;
	}

	response = extractSamples(&avData1, data1, &avData2, data2);

	if (response)
	{
		return response;
	}

	*maxSampleRate = avData1.sample_rate;

	int32_t size = data1->size + data2->size - 1;

	response = fill_zeroes(data1, size);
	response += fill_zeroes(data2, size);

	close_audio_data(&avData1);
	close_audio_data(&avData2);

	return response;
}
