#include "Iffmpeg.h"

int8_t init_audio_data(const char *file, AudioData *data)
{
	data->formatCtx = avformat_alloc_context();
	if (!data->formatCtx)
	{
		fprintf(stderr, "Could not allocate memory for format context\n");
		return ERROR_NOTENOUGH_MEMORY;
	}
	if (avformat_open_input(&data->formatCtx, file, NULL, NULL) != 0 || avformat_find_stream_info(data->formatCtx, NULL) < 0)
	{
		fprintf(stderr, "Open file error\n");
		return ERROR_CANNOT_OPEN_FILE;
	}
	data->audioStreamIndex = av_find_best_stream(data->formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (data->audioStreamIndex < 0)
	{
		fprintf(stderr, "There is no audio stream in file\n");
		return ERROR_FORMAT_INVALID;
	}
	data->sample_rate = data->formatCtx->streams[data->audioStreamIndex]->codecpar->sample_rate;
	data->codec = avcodec_find_decoder(data->formatCtx->streams[data->audioStreamIndex]->codecpar->codec_id);
	if (data->codec == NULL)
	{
		fprintf(stderr, "Couldn't find decoder\n");
		return ERROR_FORMAT_INVALID;
	}
	data->codecCtx = avcodec_alloc_context3(data->codec);
	if (!data->codecCtx)
	{
		fprintf(stderr, "Can't allocate memory for decoder\n");
		return ERROR_NOTENOUGH_MEMORY;
	}
	if (avcodec_parameters_to_context(data->codecCtx, data->formatCtx->streams[data->audioStreamIndex]->codecpar) < 0 ||
		avcodec_open2(data->codecCtx, data->codec, NULL) < 0)
	{
		fprintf(stderr, "Failed to load codec params\n");
		return ERROR_FORMAT_INVALID;
	}
	data->packet = av_packet_alloc();
	data->frame = av_frame_alloc();
	if (data->packet == NULL || data->frame == NULL)
	{
		fprintf(stderr, "Can't allocate memory for packet and frame\n");
		return ERROR_NOTENOUGH_MEMORY;
	}
	return SUCCESS;
}

int8_t increase_array_size(AudioData *data)
{
	if (data->size + data->frame->nb_samples > data->arrSize - 1)
	{
		data->arrSize += data->frame->nb_samples * 2;
		double *tmp = realloc(data->samples, data->arrSize * sizeof(double));
		data->samples = tmp;
		if (data->samples == NULL)
		{
			fprintf(stderr, "Can't allocate memory for samples\n");
			return ERROR_NOTENOUGH_MEMORY;
		}
	}
	return SUCCESS;
}

int8_t decode_packet(AudioData *data)
{
	int32_t response = avcodec_send_packet(data->codecCtx, data->packet);
	if (response < 0)
	{
		fprintf(stderr, "Cant' decode packet\n");
		return ERROR_FORMAT_INVALID;
	}
	while (1)
	{
		response = avcodec_receive_frame(data->codecCtx, data->frame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
		{
			break;
		}
		else if (response < 0)
		{
			fprintf(stderr, "Can't receive frame\n");
			return ERROR_FORMAT_INVALID;
		}
		response += increase_array_size(data);
		if (response)
		{
			return ERROR_NOTENOUGH_MEMORY;
		}
		for (int32_t i = 0; i < data->frame->nb_samples; ++i)
		{
			data->samples[(data->size)++] = (double)data->frame->data[data->channel][i];
		}
		av_frame_unref(data->frame);
	}
	return SUCCESS;
}

int8_t read_packet(AudioData *data)
{
	while (av_read_frame(data->formatCtx, data->packet) >= 0)
	{
		if (data->packet->stream_index == data->audioStreamIndex)
		{
			int8_t response = decode_packet(data);
			if (response)
			{
				return response;
			}
		}
		av_packet_unref(data->packet);
	}
	return 0;
}

void resample(AudioData *data, int32_t maxSampleRate)
{
	if (data->sample_rate != maxSampleRate)
	{
		SwrContext *swrCtx = NULL;
		swr_alloc_set_opts2(
			&swrCtx,
			&(AVChannelLayout)AV_CHANNEL_LAYOUT_MONO,
			AV_SAMPLE_FMT_DBLP,
			maxSampleRate,
			&data->codecCtx->ch_layout,
			data->codecCtx->sample_fmt,
			data->sample_rate,
			0,
			NULL);
		swr_init(swrCtx);
		swr_free(&swrCtx);
	}
}

void init_data_struct(AudioData *data)
{
	data->arrSize = INITIAL_CONST;
	data->size = 0;
	data->samples = malloc(data->arrSize * sizeof(double) * 16);
}

int8_t extractSamples(AudioData *data1, AudioData *data2, const int32_t *maxSampleRate)
{
	init_data_struct(data1);
	init_data_struct(data2);

	int8_t response;

	if (data1->sample_rate != data2->sample_rate)
	{
		resample(data1, *maxSampleRate);
		resample(data2, *maxSampleRate);
	}

	response = read_packet(data1);

	if (response)
	{
		close_av_data(data1);
		close_av_data(data2);
		return response;
	}

	response = read_packet(data2);

	if (response)
	{
		close_av_data(data1);
		close_av_data(data2);
		return response;
	}

	return SUCCESS;
}

void close_av_data(AudioData *data)
{
	if (data != NULL)
	{
		if (data->formatCtx != NULL)
		{
			avformat_free_context(data->formatCtx);
		}
		if (data->codecCtx != NULL)
		{
			avcodec_free_context(&data->codecCtx);
		}
	}
}

void close_audio_data(AudioData *data)
{
	if (data != NULL)
	{
		if (data->samples != NULL)
		{
			free(data->samples);
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
			fprintf(stderr, "It is not possible to align the array to the desired size\n");
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
	response = init_audio_data(file1, data1);
	if (response)
	{
		return response;
	}
	data1->channel = 0;

	response = init_audio_data(file2, data2);

	if (data2->codecCtx->ch_layout.nb_channels <= data2->channel)
	{
		fprintf(stderr, "There is no second channel in the file\n");
		close_av_data(data1);
		close_av_data(data2);
		return ERROR_FORMAT_INVALID;
	}

	if (response)
	{
		return response;
	}

	*maxSampleRate = data1->sample_rate > data2->sample_rate ? data1->sample_rate : data2->sample_rate;

	response = extractSamples(data1, data2, maxSampleRate);

	if (response)
	{
		return response;
	}

	int32_t size = data1->size + data2->size - 1;

	response = fill_zeroes(data1, size) | fill_zeroes(data2, size);

	close_av_data(data1);
	close_av_data(data2);

	return response;
}
