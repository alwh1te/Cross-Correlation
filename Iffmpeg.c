#include "Iffmpeg.h"

#include "AudioData.h"
int extractData(AudioData *data, int channel)
{
	data->samples = malloc(sizeof(double));
	data->size = 0;
	data->sample_rate = data->formatCtx->streams[data->audioStreamIndex]->codecpar->sample_rate;
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
				return 1;
			}

			for (int j = 0; j < frame->nb_samples; ++j)
			{
				if (data->size >= (sizeof(data->samples) / sizeof(double)))
				{
					double *tmp = realloc(data->samples, 2 * (data->size * sizeof(double)));
					data->samples = tmp;
				}
				data->samples[(data->size)++] = frame->data[channel][j];
			}
			av_frame_free(&frame);
		}
	}
	return 0;
}

int init_audio_data(const char *file, AudioData *data)
{
	data->formatCtx = avformat_alloc_context();

	if (avformat_open_input(&data->formatCtx, file, NULL, NULL) != 0)
	{
		printf("Error: Failed to open file %s\n", file);
		return 1;
	}
	if (avformat_find_stream_info(data->formatCtx, NULL) < 0)
	{
		printf("Error: Failed to find stream information for %s\n", file);
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
		printf("Error: No AudioData stream found in %s\n", file);
		avformat_close_input(&data->formatCtx);
		return 1;
	}
	const AVCodec *codec = avcodec_find_decoder((data->formatCtx)->streams[data->audioStreamIndex]->codecpar->codec_id);
	data->codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(data->codecCtx, (data->formatCtx)->streams[data->audioStreamIndex]->codecpar);
	avcodec_open2(data->codecCtx, codec, NULL);
	data->packet = av_packet_alloc();
	return 0;
}

void close_audio_data(AudioData *data)
{
	av_packet_free(&data->packet);
}