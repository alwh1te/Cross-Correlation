cmake_minimum_required(VERSION 3.27)
project(CrossCorrelation C)

set(CMAKE_C_STANDARD 17)

set(FFMPEG_PATH /opt/homebrew/Cellar/ffmpeg@6/6.1.1)
set(FFTW3_PATH /opt/homebrew/Cellar/fftw/3.3.10_1)


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O1")

include_directories(${FFMPEG_PATH}/include/)
link_directories(${FFMPEG_PATH}/lib/)

include_directories(${FFTW3_PATH}/include/)
link_directories(${FFTW3_PATH}/lib/)

add_executable(CrossCorrelation main.c
        Iffmpeg.h
        Iffmpeg.c
        Ifftw.c
        Ifftw.h
        AudioData.h)
target_link_libraries(
        CrossCorrelation
        avcodec
        avdevice
        avutil
        avfilter
        avformat
        swresample
        swscale
        "-framework AudioToolbox" "-lbz2"
        fftw3
)
target_sources(CrossCorrelation PRIVATE Iffmpeg.c Ifftw.c)

#target_compile_options(CrossCorrelation PUBLIC -fsanitize=address -fno-sanitize-recover=all -fno-optimize-sibling-calls -fno-omit-frame-pointer)