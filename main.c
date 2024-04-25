#include <fftw3.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct audio {
    double *samples;
    int size;
    int sample_rate;
} AudioData;

void perform_fft(AudioData *input, fftw_complex *output, int size) {
    fftw_plan plan = fftw_plan_dft_r2c_1d(size, input->samples, output, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);
}

void perform_ifft(fftw_complex *input, double *out, int size) {
    fftw_plan plan = fftw_plan_dft_c2r_1d(size, input, out, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);
}

int extractData(AudioData *data, AVFormatContext *formatCtx, AVPacket *packet, AVCodecContext *codecCtx, int audioStreamIndex, int channel) {
    data->samples = malloc(sizeof(double) * 800000);
    data->size = 0;
    data->sample_rate = formatCtx->streams[audioStreamIndex]->codecpar->sample_rate;
    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            AVFrame *frame = av_frame_alloc();
            int response = avcodec_send_packet(codecCtx, packet);
            if (response < 0) {
                return 1;
            }
            response = avcodec_receive_frame(codecCtx, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                av_frame_free(&frame);
                continue;
            } else if (response < 0) {
                return 2;
            }
            for (int j = 0; j < frame->nb_samples; ++j) {
                data->samples[(data->size)++] = frame->data[channel][j];
            }
            av_frame_free(&frame);
        }
    }
    return 0;
}

int initialize_codec_context(const char *file, AVFormatContext **formatCtx, int *audioStreamIndex, AVCodecContext **codecCtx) {
    *formatCtx = avformat_alloc_context();
    if (avformat_open_input(formatCtx, file, NULL, NULL) != 0) {
        printf("Error: Failed to open file %s\n", file);
        return 1;
    }
    if (avformat_find_stream_info(*formatCtx, NULL) < 0) {
        printf("Error: Failed to find stream information for %s\n", file);
        avformat_close_input(formatCtx);
        return 1;
    }
    for (int i = 0; i < (*formatCtx)->nb_streams; i++) {
        if ((*formatCtx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            *audioStreamIndex = i;
            break;
        }
    }
    if (*audioStreamIndex == -1) {
        printf("Error: No AudioData stream found in %s\n", file);
        avformat_close_input(formatCtx);
        return 2;
    }
    AVCodec *codec = avcodec_find_decoder((*formatCtx)->streams[*audioStreamIndex]->codecpar->codec_id);
    *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(*codecCtx, (*formatCtx)->streams[*audioStreamIndex]->codecpar);
    avcodec_open2(*codecCtx, codec, NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    av_log_set_level(AV_LOG_QUIET);
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <file1> [file2]\n", argv[0]);
        return 1;
    }
    const char *file1 = argv[1];
    const char *file2 = NULL;
    if (argc == 3) {
        file2 = argv[2];
    }
    AVFormatContext *formatCtx1, *formatCtx2;
    int audioStreamIndex1 = -1, audioStreamIndex2 = -1;
    int channel = 1;
    AVCodecContext *codecCtx1, *codecCtx2;
    AVPacket *packet1 = av_packet_alloc(), *packet2 = av_packet_alloc();
    initialize_codec_context(file1, &formatCtx1, &audioStreamIndex1, &codecCtx1);
    if (file2) {
        channel = 0;
        initialize_codec_context(file2, &formatCtx2, &audioStreamIndex2, &codecCtx2);
    } else {
        initialize_codec_context(file1, &formatCtx2, &audioStreamIndex2, &codecCtx2);
    }
    int size = 800000;
    AudioData *data1 = malloc(sizeof(AudioData));
    AudioData *data2 = malloc(sizeof(AudioData));
    extractData(data1, formatCtx1, packet1, codecCtx1, audioStreamIndex1, channel);
    extractData(data2, formatCtx2, packet2, codecCtx2, audioStreamIndex2, channel);
    fftw_complex *dataComplex1 = fftw_alloc_complex((size / 2) + 1);
    fftw_complex *dataComplex2 = fftw_alloc_complex((size / 2) + 1);
    perform_fft(data1, dataComplex1, size);
    perform_fft(data2, dataComplex2, size);
    fftw_complex *corr = fftw_alloc_complex((size / 2) + 1);
    for (int i = 0; i < size; ++i) {
        corr[i][0] = dataComplex1[i][0] * dataComplex2[i][0] + dataComplex1[i][1] * dataComplex2[i][1];
        corr[i][1] = dataComplex1[i][1] * dataComplex2[i][0] - dataComplex1[i][0] * dataComplex2[i][1];
    }
    double *out = malloc(sizeof(double) * size);
    perform_ifft(corr, out, size);
    double max_value = 0;
    int max_position = 0;
    for (int i = 0; i < size; i++) {
        if (out[i] > max_value) {
            max_value = out[i];
            max_position = i;
        }
    }
    int sample_rate = data1->sample_rate;
    double time_shift;
    if (max_position > size / 2) {
        max_position = size - max_position;
        max_position *= -1;
    }
    time_shift = (max_position * 1000) / sample_rate;
    printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", max_position, sample_rate, (int) (time_shift));
    av_packet_unref(packet1);
    av_packet_unref(packet2);
    fftw_free(data1);
    fftw_free(data2);
    fftw_free(dataComplex1);
    fftw_free(dataComplex2);
    fftw_free(corr);
    free(out);
    avformat_close_input(&formatCtx1);
    avformat_close_input(&formatCtx2);
    return 0;
}
