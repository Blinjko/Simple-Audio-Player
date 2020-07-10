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
#include "ffmpeg_decoder.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}
#include <string>
#include <queue>

FFmpeg_Decoder::FFmpeg_Decoder(const std::string& filename, enum AVMediaType media_type) : 
    m_filename{filename}, m_media_type{media_type}
{
    m_fmt_ctx = nullptr;
    m_codec_ctx = nullptr;
    m_packet = nullptr;
    m_frame = nullptr;
}

FFmpeg_Decoder::FFmpeg_Decoder(const char* filename, enum AVMediaType media_type) : FFmpeg_Decoder(std::string{filename}, media_type)
{}

FFmpeg_Decoder::~FFmpeg_Decoder()
{
    reset("NO NAME", AVMEDIA_TYPE_UNKNOWN);
}


Return_Status FFmpeg_Decoder::open_file()
{
    int error = 0;

    m_fmt_ctx = avformat_alloc_context();
    if(!m_fmt_ctx)
    {
        // allocation failed
        enqueue_error("Failed to allocate AVFormatContext");
        return STATUS_FAILURE;
    }

    error = avformat_open_input(&m_fmt_ctx, m_filename.c_str(), nullptr, nullptr);
    if(error < 0)
    {
        // failed to open file
        enqueue_error("Failed to open file");
        enqueue_error(error);
        return STATUS_FAILURE;
    }

    error = avformat_find_stream_info(m_fmt_ctx, nullptr);
    if(error < 0)
    {
        // failed to read stream info
        enqueue_error("Failed to read stream info");
        enqueue_error(error);
        return STATUS_FAILURE;
    }

    error = av_find_best_stream(m_fmt_ctx, m_media_type, -1, -1, nullptr, 0);
    if(error < 0)
    {
        // failed to find a stream
        enqueue_error("Failed to find a stream");
        enqueue_error(error);
        return STATUS_FAILURE;
    }

    m_stream_number = error;

    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Decoder::init()
{
    int error = 0;
    
    AVStream *stream = m_fmt_ctx->streams[m_stream_number];

    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!codec)
    {
        // failed to find a codec
        enqueue_error("Failed to find a codec");
        return STATUS_FAILURE;
    }

    m_codec_ctx = avcodec_alloc_context3(codec);
    if(!m_codec_ctx)
    {
        // failed to allocate a codec context
        enqueue_error("Failed to allocate an AVCodecContext");
        return STATUS_FAILURE;
    }

    error = avcodec_parameters_to_context(m_codec_ctx, stream->codecpar);
    if(error < 0)
    {
        // failed to fill codec context with extra paramters, potentially needed for future operations
        enqueue_error("Failed to fill AVCodecContext with AVStream codec parameters");
        enqueue_error(error);
        return STATUS_FAILURE;
    }

    error = avcodec_open2(m_codec_ctx, codec, nullptr);
    if(error < 0)
    {
        // failed to open / initialize the codec context
        enqueue_error("Failed to open AVCodecContext");
        enqueue_error(error);
        return STATUS_FAILURE;
    }

    m_packet = av_packet_alloc();
    if(!m_packet)
    {
        // failed to allocate packet
        enqueue_error("Failed to allocate packet");
        return STATUS_FAILURE;
    }

    m_frame = av_frame_alloc();
    if(!m_frame)
    {
        // failed to allocate frame
        enqueue_error("Failed to allocate frame");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Decoder::drain()
{
    int error = 0;

    // enter draining mode
    error = avcodec_send_packet(m_codec_ctx, nullptr);
    if(error < 0)
    {
        enqueue_error("Failed to enter draining mode");
        enqueue_error(error);
    }

    while(1)
    {
        error = avcodec_receive_frame(m_codec_ctx, m_frame);

        if(error == AVERROR_EOF)
        {
            break;
        }

        else if(error < 0)
        {
            enqueue_error("Failed to drain codec");
            enqueue_error(error);
            return STATUS_FAILURE;
        }
    }

    avcodec_flush_buffers(m_codec_ctx);
    return STATUS_SUCCESS;
}
void FFmpeg_Decoder::reset(const std::string &filename, enum AVMediaType media_type)
{
    if(m_fmt_ctx)
    {
        avformat_close_input(&m_fmt_ctx);
        avformat_free_context(m_fmt_ctx);
    }

    if(m_packet)
    {
        av_packet_unref(m_packet);
        av_packet_free(&m_packet);
        av_frame_unref(m_frame);
    }
    
    if(m_frame)
    {
        av_frame_unref(m_frame);
        av_frame_free(&m_frame);
    }

    if(m_codec_ctx)
    {
        avcodec_free_context(&m_codec_ctx);
    }

    m_filename = filename;
    m_media_type = media_type;

    m_fmt_ctx = nullptr;
    m_codec_ctx = nullptr;
    m_packet = nullptr;
    m_frame = nullptr;
    m_stream_number = -1;
    m_end_of_file = false;
}

AVFrame *FFmpeg_Decoder::decode_frame()
{
    Return_Status status = decoder_fill();
    if(status == STATUS_FAILURE)
    {
        // failed to fill decoder with data
        enqueue_error("Failed to fill decoder");
        return nullptr;
    }

    int error = 0;

    av_frame_unref(m_frame);

    error = avcodec_receive_frame(m_codec_ctx, m_frame);
    if(error == AVERROR(EAGAIN))
    {
        // decoder needs more data
        return nullptr;
    }

    else if(error < 0)
    {
        // some error occurred
        enqueue_error("Failed to receive frame from decoder");
        enqueue_error(error);
        return nullptr;
    }

    return m_frame;
}

std::string FFmpeg_Decoder::poll_error()
{
    if(!m_errors.empty())
    {
        std::string return_val = m_errors.front();
        m_errors.pop();
        return return_val;
    }

    return std::string{};
}


AVFormatContext *FFmpeg_Decoder::get_format_context()
{
    return m_fmt_ctx;
}

AVCodecContext *FFmpeg_Decoder::get_codec_context()
{
    return m_codec_ctx;
}

enum AVMediaType FFmpeg_Decoder::get_media_type()
{
    return m_media_type;
}

std::string FFmpeg_Decoder::get_filename()
{
    return m_filename;
}

bool FFmpeg_Decoder::end_of_file_reached()
{
    return m_end_of_file;
}
Return_Status FFmpeg_Decoder::decoder_fill()
{
    int error = 0;


    while(1)
    {
        if(!m_packet->data)
        {
            // packet is not referencing any data, so read some
            error = av_read_frame(m_fmt_ctx, m_packet);
            if(error == AVERROR_EOF)
            {
                // end of file reached
                m_end_of_file = true;
                return STATUS_SUCCESS;
            }

            else if(error < 0)
            {
                // some error occurred when reading a packet
                enqueue_error("Failed to read data from file");
                enqueue_error(error);
                return STATUS_FAILURE;
            }

            if(m_packet->stream_index != m_stream_number)
            {
                // the packets stream doesn't match 
                av_packet_unref(m_packet);
                continue;
            }
        }

        error = avcodec_send_packet(m_codec_ctx, m_packet);
        if(error == AVERROR(EAGAIN))
        {
            // decoder wont take any more data
            return STATUS_SUCCESS;
        }

        else if(error < 0)
        {
            // an error occured when sending a packet to the decoder
            enqueue_error("Failed to send packet to decoder");
            enqueue_error(error);
        }
        av_packet_unref(m_packet);
    }
}

void FFmpeg_Decoder::enqueue_error(int error_code)
{
    char buff[256];
    int error = av_strerror(error_code, buff, sizeof(buff));
    if(error < 0)
    {
        enqueue_error("Error code not found");
    }
    else
    {
        enqueue_error(std::string{buff});
    }
}

void FFmpeg_Decoder::enqueue_error(const std::string &message)
{
    m_errors.push(message);
    return;
}
