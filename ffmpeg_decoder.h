/*
Copyright (C) 2020 Blinjko 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    Find Blinjko at: <www.github.com/Blinjko>
*/
#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}
#include <string>
#include <queue>


#ifndef RETURN_STATUS
#define RETURN_STATUS
enum Return_Status
{
    STATUS_SUCCESS,
    STATUS_FAILURE,
};
#endif

// SEE "ffmpeg_decoder.cpp" for comments on functions //

class FFmpeg_Decoder
{
    AVFormatContext *m_fmt_ctx;
    AVCodecContext *m_codec_ctx;
    int m_stream_number;
    AVPacket *m_packet;
    AVFrame *m_frame;
    enum AVMediaType m_media_type;
    bool m_end_of_file;
    

    std::string m_filename;
    std::queue<std::string> m_errors;

    public:

    FFmpeg_Decoder(const std::string&, enum AVMediaType);
    FFmpeg_Decoder(const char*, enum AVMediaType);
    ~FFmpeg_Decoder();
    
    Return_Status open_file();
    Return_Status init();
    Return_Status drain();
    void reset(const std::string&, enum AVMediaType);

    AVFrame *decode_frame();

    std::string poll_error();

    AVFormatContext *get_format_context();
    AVCodecContext *get_codec_context();
    enum AVMediaType get_media_type();
    std::string get_filename();
    bool end_of_file_reached();

    private:

    Return_Status decoder_fill();
    void enqueue_error(int error_code);
    void enqueue_error(const std::string &message);
};
