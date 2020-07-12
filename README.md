# Preface #
This is a simple audio player I wrote as a part of a bigger project. I have written and tested it for Linux, but it will work on any platform that has the
required libraries and headers. Tried to keep the code simple for anyone looking at it as a FFmpeg learning resource.

# Requirements #
* PulseAudio & PulseAudio-Simple headers and libraries
* FFmpeg headers and libraries
* A C++ compiler
* GNU Make, if you want to use the makefile

# Installing Dependicies #
* For Debian: `apt install git g++ make libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libpulse-dev`
* For Arch: TODO
* For Fedora: TODO

# Building #
Before you start building make sure all the requirements stated above are met. Firstly clone the repository using `git clone`, then execute `make`, then 
`make clean`. This will compile and link the program, making a executable program in the working directory called `Player`. There are some reasons why the program 
might not build, they are stated below.
* The program failed to link because the libraries needed not found because, they are named differently, in a different location, or not installed.
* The program failed to compile because the compiler could not find the needed header files, this can happen because, they are named differently, placed in a 
different location, or not installed.

For a reference the header file locations on my computer are:
* `/usr/include/pulse/simple.h` For the pulsemixer header
* `/usr/include/libavformat/avforamt.h` For FFmpeg libavformat header
* `/usr/include/libavutil/avutil.h` For FFmpeg libavutil header
* `/usr/include/libavutil/frame.h` For FFmpeg AVFrame header
* `/usr/include/libavutil/opt.h` For FFmpeg Options header
* `/usr/include/libavcodec/avcodec.h` For FFmpeg libavcodec header
* `/usr/include/` For C++ standard libary headers

# Supported Formats #
m4a, mp3, aac, flac, m4b, ogg, oga, opus, ra, rm, tta, webm, au, wav, mkv, avi

# Usage #
`./Player <input_file>`
If the makefile is used the program will be named `Player`, otherwise use whatever you named it. `<input_file>` is the audio file you want to play, for supported formats see
the Supported Formats section.

# Sources #
* [FFmpeg](https://ffmpeg.org)
* [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
