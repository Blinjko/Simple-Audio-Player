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

# Usage #
`./Player <input_file>`
If the makefile is used the program will be named `Player`, otherwise use whatever you named it. `<input_file>` is the audio file you want to play, one is included
in the repository called `Panda_Eyes-KIKO.webm`. NOTE: I do now own the song nor am I claiming to own the song contained within this file. The link to the song 
can be found below. To run the song execute the following in the repository directory `./Player Panda_Eyes-KIKO.webm`. This should play the song to the default
output device. To adjust volume install the `pulsemixer` program and adjust the stream volume, also in pulsemixer you can switch what audio device the program will
output to.

# Sources #
* The supplied song can be found: [Here](https://www.youtube.com/watch?v=BwR8LOWZ1hg)
* [FFmpeg](https://ffmpeg.org)
* [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
