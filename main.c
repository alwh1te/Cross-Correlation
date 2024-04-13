#include <stdio.h>
#include <fftw3.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avfft.h>
#include <libavutil/audio_fifo.h>
#include <libavcodec/avcodec.h>

#define MAX_AUDIO_FRAME_SIZE 192000


void load_audio_data(const char *filename, float *audio_data, int length) {
    AVFormatContext *format_context = NULL;
    AVCodecContext *codec_context = NULL;
    AVCodec *codec = NULL;
    AVPacket packet;
    AVFrame *frame = NULL;
    int stream_index = -1;
    int got_frame = 0;
    int audio_frame_size = 0;
    int total_samples = 0;
    int sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT); // Size of one sample in bytes
    int sample_count = length / sample_size; // Number of samples to read

//    av_register_all();
    avformat_network_init();

    // Open the input file
    if (avformat_open_input(&format_context, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return;
    }

    // Find stream information
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Error: Could not find stream information\n");
        avformat_close_input(&format_context);
        return;
    }

    // Find the audio stream
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        fprintf(stderr, "Error: Could not find audio stream in the input file\n");
        avformat_close_input(&format_context);
        return;
    }

    // Get a pointer to the codec context for the audio stream
    codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codec_context, format_context->streams[stream_index]->codecpar);

    // Find the decoder for the audio stream
    codec = avcodec_find_decoder(codec_context->codec_id);
    if (!codec) {
        fprintf(stderr, "Error: Unsupported codec\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return;
    }

    // Open codec
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Error: Could not open codec\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return;
    }

    // Allocate audio frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error: Could not allocate audio frame\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return;
    }

    // Initialize packet
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    // Read frames and store audio data
    while (av_read_frame(format_context, &packet) >= 0) {
        if (packet.stream_index == stream_index) {
            int ret = avcodec_send_packet(codec_context, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error: Error sending a packet for decoding\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_context, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error: Error during decoding\n");
                    break;
                }

                // Convert audio frame to float
                for (int i = 0; i < frame->nb_samples; i++) {
                    for (int j = 0; j < codec_context->channels; j++) {
                        if (total_samples < length) {
                            audio_data[total_samples] = (float)frame->data[j][i] / INT16_MAX;
                            total_samples++;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
        av_packet_unref(&packet);
    }

    // Clean up
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <file_one>\n", argv[0]);
        return 1;
    }

    const char *file_one = argv[1];
    char *file_two = NULL;
    if (argc == 3) {
        file_two = argv[2];
    }
//    int64_t ff_(const double *x, const double *y, int n);

    AVFormatContext *fmt_ctx = NULL;

    if (avformat_open_input(&fmt_ctx, file_one, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file: %s\n", file_one);
        return 1;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return 1;
    }

    int audio_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    if (audio_stream_index == -1) {
        fprintf(stderr, "No audio stream found in the input file\n");
        return 1;
    }
//    int max = calculate_cross_correlation(fmt_ctx, );
    AVCodecParameters *codec_params = fmt_ctx->streams[audio_stream_index]->codecpar;
    int sample_rate = codec_params->sample_rate;

    printf("Sample rate: %d Hz\n", sample_rate);

    avformat_close_input(&fmt_ctx);
    return 0;
}