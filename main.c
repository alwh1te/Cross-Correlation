#include <fftw3.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void cross_correlation(fftw_complex *data1, fftw_complex *data2, int size) {
    fftw_complex *result;
    fftw_plan plan;

    // Выделение памяти для результата кросс-корреляции
    result = fftw_malloc(sizeof(fftw_complex) * size);

    // Создание плана для преобразования Фурье
    plan = fftw_plan_dft_1d(size, data1, result, FFTW_FORWARD, FFTW_ESTIMATE);

    // Выполнение преобразования Фурье для первого массива данных
    fftw_execute(plan);

    // Очистка плана
    fftw_destroy_plan(plan);

    // Здесь можно выполнить умножение на преобразование второго массива данных и обратное преобразование

    // Освобождение памяти
    fftw_free(result);
}

void perform_fft(double *input, fftw_complex *data, int size) {
    fftw_plan plan;

    // Создание плана для преобразования Фурье
    plan = fftw_plan_dft_r2c_1d(size, input, data, FFTW_ESTIMATE);

    // Выполнение преобразования Фурье
    fftw_execute(plan);
//    free(input);
    // Очистка плана
    fftw_destroy_plan(plan);
}

void perform_ifft(fftw_complex *input, double *out, int size) {
    fftw_plan plan;

    // Создание плана для обратного преобразования Фурье
//    plan = fftw_plan_dft_c2r_1d(size, input, out, FFTW_ESTIMATE);
    plan = fftw_plan_dft_c2r_1d(size, input, out, FFTW_ESTIMATE);

    // Выполнение обратного преобразования Фурье
    fftw_execute(plan);

    // Нормализация значений после обратного преобразования
//    for (int i = 0; i < size; ++i) {
//        input[i][0] /= size;
//        input[i][1] /= size;
//    }

    // Очистка плана
    fftw_destroy_plan(plan);
}

double *extractData(AVFormatContext *formatCtx, AVPacket *packet, AVCodecContext *codecCtx, int audioStreamIndex, int size) {
    int localSize = ((size / 2) + 1);
    double *data = malloc(sizeof(fftw_complex) * localSize);
    int i = 0;
    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            AVFrame *frame = av_frame_alloc();
            int response = avcodec_send_packet(codecCtx, packet);
            if (response < 0) {
                fprintf(stderr, "Ошибка при отправке пакета в декодер\n");
                //                return 1;
            }
            response = avcodec_receive_frame(codecCtx, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                av_frame_free(&frame);
                fprintf(stderr, "Ошибка\n");
                continue;
            } else if (response < 0) {
                fprintf(stderr, "Ошибка при получении кадра из декодера\n");
                //                            return 1;
            }

//            SwrContext *swrCtx = swr_alloc();
//            swr_init(swrCtx);
//            swrCtx = swr_alloc_set_opts(&swrCtx,
//                                AV_CH_LAYOUT_STEREO,              // Выходной каналы (стерео)
//                                AV_SAMPLE_FMT_DBL,                                          // Выходной формат сэмплов (16-битный)
//                                formatCtx->streams[audioStreamIndex]->codecpar->sample_rate,// Частота дискретизации (44.1 кГц)
//                                AV_CH_LAYOUT_STEREO,              // Входной каналы (стерео)
//                                AV_SAMPLE_FMT_FLTP,                                         // Входной формат сэмплов (плавающая точка)
//                                formatCtx->streams[audioStreamIndex]->codecpar->sample_rate,// Частота дискретизации (44.1 кГц)
//                                0,                                                          // Логический номер первого измерения
//                                NULL);
//            num_samples = swr_convert(swrCtx, (uint8_t **) &samples, 1024, (const uint8_t **) frame->data, frame->nb_samples);

            for (int j = 0; j < frame->nb_samples; ++j) {
                data[i++] = frame->data[0][j];
//                data[j][1] = 0;
            }
//            i++;
//            swr_free(&swrCtx);
            av_frame_free(&frame);
        }
    }
//    while (i < localSize) {
//        printf("%f", data[i++]);
////        data[i++] = 0;
//    }
    return data;
}

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

    AVFormatContext *formatCtx1 = avformat_alloc_context();
    AVFormatContext *formatCtx2 = avformat_alloc_context();
    int audioStreamIndex1 = -1;
    int audioStreamIndex2 = -1;
    if (avformat_open_input(&formatCtx1, file1, NULL, NULL) != 0) {
        printf("Error: Failed to open file %s\n", file1);
        return 1;
    }
    if (avformat_find_stream_info(formatCtx1, NULL) < 0) {
        printf("Error: Failed to find stream information for %s\n", file1);
        avformat_close_input(&formatCtx1);
        return 1;
    }

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
        for (int i = 0; i < formatCtx2->nb_streams; ++i) {
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

    AVCodec *codec = avcodec_find_decoder(formatCtx1->streams[audioStreamIndex1]->codecpar->codec_id);
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx1->streams[audioStreamIndex1]->codecpar);
    AVPacket *packet = av_packet_alloc();
    avcodec_open2(codecCtx, codec, NULL);

    int size = 800000;
    double *data1 = extractData(formatCtx1, packet, codecCtx, audioStreamIndex1, size);
    codec = avcodec_find_decoder(formatCtx2->streams[audioStreamIndex2]->codecpar->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx2->streams[audioStreamIndex2]->codecpar);
    packet = av_packet_alloc();
    avcodec_open2(codecCtx, codec, NULL);

    double *data2 = extractData(formatCtx2, packet, codecCtx, audioStreamIndex2, size);
    fftw_complex *dataComplex1 = (fftw_complex *) fftw_alloc_real(sizeof(fftw_complex) * ((size / 2) + 1));
    fftw_complex *dataComplex2 = (fftw_complex *) fftw_alloc_real(sizeof(fftw_complex) * ((size / 2) + 1));
    perform_fft(data1, dataComplex1, size);
    perform_fft(data2, dataComplex2, size);

//    for (int i = 0; i < size; ++i) {
//        double real_part = (data1[i][0] * data2[i][0]) - (data1[i][1] * data2[i][1]);
//        double imag_part = (data1[i][0] * data2[i][1]) + (data1[i][1] * data2[i][0]);
//        data1[i][0] = real_part;
//        data1[i][1] = imag_part;
//    }

    // Обратное преобразование Фурье для получения кросс-корреляционной функции
//    perform_ifft(data1, size);
    fftw_complex *corr = (fftw_complex *) fftw_alloc_real(sizeof(fftw_complex) * ((size / 2) + 1));
    for (int i = 0; i < size; ++i) {
        corr[i][0] = dataComplex1[i][0] * dataComplex2[i][0] + dataComplex1[i][1] * dataComplex2[i][1];
        corr[i][1] = dataComplex1[i][1] * dataComplex2[i][0] - dataComplex1[i][0] * dataComplex2[i][1];
//        printf("%f\n", corr[i][0]);
    }
    double *out = (double *) fftw_alloc_real(sizeof(fftw_complex) * size * 2);
    perform_ifft(corr, out, size);
    // Анализ результата (максимальное значение и его позиция)
    double max_value = 0;
    int max_position = 0;
    for (int i = 0; i < size; i++) {
        if (out[i] > max_value) {
            max_value = out[i];
            max_position = i;
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
        time_shift = (max_position * 1000)/sample_rate;// Placeholder for the actual calculation
    }

    // Print results
    printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n", max_position, sample_rate, (int) (time_shift));

    av_packet_unref(packet);
    fftw_free(data1);
    fftw_free(data2);
    av_free(formatCtx1);
    av_free(formatCtx2);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    av_packet_free(&packet);
    avformat_close_input(&formatCtx1);

    if (formatCtx2) {
        avformat_close_input(&formatCtx2);
    }

    return 0;
}
