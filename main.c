#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <fftw3.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define MAX_FRAME_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <file1> [file2]\n", argv[0]);
        return 1;
    }

    const char *file1 = argv[1];
    const char *file2 = NULL;
    if (argc == 3) {
        file2 = argv[2];
    }

    // Initialize FFmpeg
//    av_register_all();

    // Open the first input file
    AVFormatContext *formatCtx1 = NULL;
    if (avformat_open_input(&formatCtx1, file1, NULL, NULL) != 0) {
        printf("Error: Failed to open file %s\n", file1);
        return 1;
    }
    if (avformat_find_stream_info(formatCtx1, NULL) < 0) {
        printf("Error: Failed to find stream information for %s\n", file1);
        avformat_close_input(&formatCtx1);
        return 1;
    }

    // Check if the first file has audio stream
    int audioStreamIndex1 = -1;
    for (int i = 0; i < formatCtx1->nb_streams; i++) {
        if (formatCtx1->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex1 = i;
            break;
        }
    }
    if (audioStreamIndex1 == -1) {
        printf("Error: No audio stream found in %s\n", file1);
        avformat_close_input(&formatCtx1);
        return 1;
    }

    // Open the second input file if provided
    AVFormatContext *formatCtx2 = NULL;
    int audioStreamIndex2 = -1;
    if (file2) {
        if (avformat_open_input(&formatCtx2, file2, NULL, NULL) != 0) {
            printf("Error: Failed to open file %s\n", file2);
            avformat_close_input(&formatCtx1);
            return 1;
        }
        if (avformat_find_stream_info(formatCtx2, NULL) < 0) {
            printf("Error: Failed to find stream information for %s\n", file2);
            avformat_close_input(&formatCtx1);
            avformat_close_input(&formatCtx2);
            return 1;
        }

        // Check if the second file has audio stream
        for (int i = 0; i < formatCtx2->nb_streams; i++) {
            if (formatCtx2->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioStreamIndex2 = i;
                break;
            }
        }
        if (audioStreamIndex2 == -1) {
            printf("Error: No audio stream found in %s\n", file2);
            avformat_close_input(&formatCtx1);
            avformat_close_input(&formatCtx2);
            return 1;
        }
    }

    // Get sample rate
    int sample_rate = formatCtx1->streams[audioStreamIndex1]->codecpar->sample_rate;

    // Calculate time shift
    double time_shift = 0.0;
    if (file2) {
        // Perform cross-correlation
        // (You need to implement this part)

        // Calculate time shift in milliseconds
        time_shift = 0.0; // Placeholder for the actual calculation
    }

    // Print results
    printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", 0, sample_rate, (int)(time_shift * 1000));

    // Close input files
    avformat_close_input(&formatCtx1);
    if (formatCtx2) {
        avformat_close_input(&formatCtx2);
    }

    return 0;
}
