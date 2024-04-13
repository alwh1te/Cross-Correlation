#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>

#define MAX_FRAME_SIZE 192000 // Максимальный размер фрейма аудио (для максимальной продолжительности сегмента)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: %s <файл_1> <файл_2>\n", argv[0]);
        return 1;
    }

    const char *filename1 = argv[1];
    const char *filename2 = argv[2];


    // Открытие файлов
    AVFormatContext *formatCtx1 = NULL, *formatCtx2 = NULL;
    if (avformat_open_input(&formatCtx1, filename1, NULL, NULL) != 0 ||
        avformat_find_stream_info(formatCtx1, NULL) < 0) {
        printf("Ошибка при открытии файла 1\n");
        return 1;
    }
    if (avformat_open_input(&formatCtx2, filename2, NULL, NULL) != 0 ||
        avformat_find_stream_info(formatCtx2, NULL) < 0) {
        printf("Ошибка при открытии файла 2\n");
        return 1;
    }

    // Получение аудио-стримов
    int audioStreamIndex1 = -1, audioStreamIndex2 = -1;
    for (int i = 0; i < formatCtx1->nb_streams; i++) {
        if (formatCtx1->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex1 = i;
            break;
        }
    }
    for (int i = 0; i < formatCtx2->nb_streams; i++) {
        if (formatCtx2->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex2 = i;
            break;
        }
    }
    if (audioStreamIndex1 == -1 || audioStreamIndex2 == -1) {
        printf("Аудио-стрим не найден в одном из файлов\n");
        return 1;
    }

    // Получение декодеров
    AVCodec *codec1 = avcodec_find_decoder(formatCtx1->streams[audioStreamIndex1]->codecpar->codec_id);
    AVCodec *codec2 = avcodec_find_decoder(formatCtx2->streams[audioStreamIndex2]->codecpar->codec_id);
    if (!codec1 || !codec2) {
        printf("Декодер не найден\n");
        return 1;
    }
    AVCodecContext *codecCtx1 = avcodec_alloc_context3(codec1);
    AVCodecContext *codecCtx2 = avcodec_alloc_context3(codec2);
    avcodec_parameters_to_context(codecCtx1, formatCtx1->streams[audioStreamIndex1]->codecpar);
    avcodec_parameters_to_context(codecCtx2, formatCtx2->streams[audioStreamIndex2]->codecpar);
    if (avcodec_open2(codecCtx1, codec1, NULL) < 0 || avcodec_open2(codecCtx2, codec2, NULL) < 0) {
        printf("Не удалось открыть декодер\n");
        return 1;
    }

    // Инициализация FFTW
    fftw_plan plan1, plan2, plan3;
    fftw_complex *in1, *in2, *out1, *out2, *corr;
    in1 = (fftw_complex *) fftw_malloc(MAX_FRAME_SIZE * sizeof(fftw_complex));
    in2 = (fftw_complex *) fftw_malloc(MAX_FRAME_SIZE * sizeof(fftw_complex));
    out1 = (fftw_complex *) fftw_malloc(MAX_FRAME_SIZE * sizeof(fftw_complex));
    out2 = (fftw_complex *) fftw_malloc(MAX_FRAME_SIZE * sizeof(fftw_complex));
    corr = (fftw_complex *) fftw_malloc(MAX_FRAME_SIZE * sizeof(fftw_complex));
    plan1 = fftw_plan_dft_1d(MAX_FRAME_SIZE, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
    plan2 = fftw_plan_dft_1d(MAX_FRAME_SIZE, in2, out2, FFTW_FORWARD, FFTW_ESTIMATE);
    plan3 = fftw_plan_dft_1d(MAX_FRAME_SIZE, corr, corr, FFTW_BACKWARD, FFTW_ESTIMATE);

    // Считывание и обработка аудио-данных
    AVPacket packet;
    int frameFinished;
    while (av_read_frame(formatCtx1, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex1) {
            AVFrame *frame = av_frame_alloc();
            avcodec_send_packet(codecCtx1, &packet);
            if (avcodec_receive_frame(codecCtx1, frame) == 0) {
                for (int i = 0; i < frame->nb_samples; i++) {
                    in1[i][0] = frame->data[0][i];
                    in1[i][1] = 0.0;
                }
                fftw_execute(plan1);
            }
            av_frame_free(&frame);
            break;
        }
        av_packet_unref(&packet);
    }
    while (av_read_frame(formatCtx2, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex2) {
            AVFrame *frame = av_frame_alloc();
            avcodec_send_packet(codecCtx2, &packet);
            if (avcodec_receive_frame(codecCtx2, frame) == 0) {
                for (int i = 0; i < frame->nb_samples; i++) {
                    in2[i][0] = frame->data[0][i];
                    in2[i][1] = 0.0;
                }
                fftw_execute(plan2);
            }
            av_frame_free(&frame);
            break;
        }
        av_packet_unref(&packet);
    }

    // Кросс-корреляция
    for (int i = 0; i < MAX_FRAME_SIZE; i++) {
        corr[i][0] = (out1[i][0] * out2[i][0] + out1[i][1] * out2[i][1]) / MAX_FRAME_SIZE;
        corr[i][1] = (out1[i][1] * out2[i][0] - out1[i][0] * out2[i][1]) / MAX_FRAME_SIZE;
    }
    fftw_execute(plan3);
    double max_corr = 0.0;
    int max_index = 0;
    for (int i = 0; i < MAX_FRAME_SIZE; i++) {
        double abs_corr = sqrt(corr[i][0] * corr[i][0] + corr[i][1] * corr[i][1]);
        if (abs_corr > max_corr) {
            max_corr = abs_corr;
            max_index = i;
        }
    }

    // Вычисление временного смещения
    double sample_rate = formatCtx1->streams[audioStreamIndex1]->codecpar->sample_rate;
    double time_shift = (double)max_index / sample_rate;

    // Вывод временного смещения
    printf("Временное смещение: %f секунд\n", time_shift);

    // Освобождение ресурсов
    fftw_destroy_plan(plan1);
    fftw_destroy_plan(plan2);
    fftw_destroy_plan(plan3);
    fftw_free(in1);
    fftw_free(in2);
    fftw_free(out1);
    fftw_free(out2);
    fftw_free(corr);
    avformat_close_input(&formatCtx1);
    avformat_close_input(&formatCtx2);
    avcodec_free_context(&codecCtx1);
    avcodec_free_context(&codecCtx2);

    return 0;
}