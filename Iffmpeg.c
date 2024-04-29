#include "Iffmpeg.h"
#include "AudioData.h"

int extractData(AudioData *data, int channel, int maxSampleRate)
{
	if (maxSampleRate != data->sample_rate)
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
	while (av_read_frame(data->formatCtx, data->packet) >= 0)
	{
		if (data->packet->stream_index == data->audioStreamIndex)
		{
			AVFrame *frame = av_frame_alloc();
			int response = avcodec_send_packet(data->codecCtx, data->packet);
			if (response < 0)
			{
				return 1;
			}
			response = avcodec_receive_frame(data->codecCtx, frame);
			if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
			{
				av_frame_free(&frame);
				continue;
			}
			else if (response < 0)
			{
				av_frame_free(&frame);
				return 1;
			}
			while (!response)
			{
				for (int j = 0; j < frame->nb_samples; ++j)
				{
					if (data->size >= ((int)sizeof(data->samples) / sizeof(double)))
					{
						double *tmp = realloc(data->samples, 2 * (data->size * sizeof(double)));
						data->samples = tmp;
					}
					if (data->codecCtx->ch_layout.nb_channels <= channel)
					{
						av_frame_free(&frame);
						return 1;
					}
					data->samples[(data->size)++] = frame->data[channel][j];
				}
				av_frame_unref(frame);
				response = avcodec_receive_frame(data->codecCtx, frame);
			}
			av_frame_free(&frame);
		}
		av_packet_unref(data->packet);
		int arr_size = (int)sizeof(data->samples) / sizeof(double);
		int i = (data->size);
		while (i < (arr_size))
		{
			data->samples[i++] = 0.0;
		}
	}

	return 0;
}

int init_audio_data(const char *file, AudioData *data)
{
	data->formatCtx = avformat_alloc_context();

	if (avformat_open_input(&data->formatCtx, file, NULL, NULL) != 0)
	{
		fprintf(stderr, "Error: Failed to open file %s\n", file);
		return 1;
	}
	if (avformat_find_stream_info(data->formatCtx, NULL) < 0)
	{
		fprintf(stderr, "Error: Failed to find stream information for %s\n", file);
		avformat_close_input(&data->formatCtx);
		return 1;
	}
	for (int i = 0; i < (data->formatCtx)->nb_streams; i++)
	{
		if ((data->formatCtx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			data->audioStreamIndex = i;
			break;
		}
	}
	if (data->audioStreamIndex == -1)
	{
		fprintf(stderr, "Error: No AudioData stream found in %s\n", file);
		avformat_close_input(&data->formatCtx);
		return 1;
	}
	const AVCodec *codec = avcodec_find_decoder((data->formatCtx)->streams[data->audioStreamIndex]->codecpar->codec_id);
	data->codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(data->codecCtx, (data->formatCtx)->streams[data->audioStreamIndex]->codecpar);
	avcodec_open2(data->codecCtx, codec, NULL);
	data->packet = av_packet_alloc();
	data->samples = malloc(sizeof(double) * 2);
	data->size = 0;
	data->sample_rate = data->formatCtx->streams[data->audioStreamIndex]->codecpar->sample_rate;
	return 0;
}

void close_audio_data(AudioData *data)
{
	free(data->samples);
	avformat_free_context(data->formatCtx);
	avcodec_close(data->codecCtx);
	av_packet_free(&data->packet);
	free(data);
}