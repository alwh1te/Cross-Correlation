cmake_minimum_required(VERSION 3.27)
project(CrossCorrelation C)

set(CMAKE_C_STANDARD 17)

set(FFMPEG_PATH /opt/homebrew/Cellar/ffmpeg@6/6.1.1)
set(FFTW3_PATH /opt/homebrew/Cellar/fftw/3.3.10_1)

include_directories(${FFMPEG_PATH}/include/)
link_directories(${FFMPEG_PATH}/lib/)

include_directories(${FFTW3_PATH}/include/)
link_directories(${FFTW3_PATH}/lib/)

add_executable(CrossCorrelation main.c)

target_link_libraries(
        CrossCorrelation
        avcodec
        avdevice
        avutil
        avfilter
        avformat
        avutil
        swresample
        swscale
        "-framework AudioToolbox" "-lbz2"
        fftw3
)
