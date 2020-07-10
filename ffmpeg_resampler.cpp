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
#include "ffmpeg_resampler.h"

extern "C"
{
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
}

#include <string>
#include <queue>


FFmpeg_Frame_Resampler::FFmpeg_Frame_Resampler(int64_t out_channel_layout,
                       enum AVSampleFormat out_sample_format, 
                       int out_sample_rate,

                       int64_t in_channel_layout,
                       enum AVSampleFormat in_sample_format,
                       int in_sample_rate) :

    m_out_channel_layout{out_channel_layout},
    m_out_sample_format{out_sample_format},
    m_out_sample_rate{out_sample_rate},

    m_in_channel_layout{in_channel_layout},
    m_in_sample_format{in_sample_format},
    m_in_sample_rate{in_sample_rate}
{
    m_swr_ctx = nullptr;
    m_frame = nullptr;
}

FFmpeg_Frame_Resampler::~FFmpeg_Frame_Resampler()
{
    if(m_swr_ctx)
    {
        swr_free(&m_swr_ctx);
    }

    if(m_frame)
    {
        av_frame_unref(m_frame);
        av_frame_free(&m_frame);
    }
}

Return_Status FFmpeg_Frame_Resampler::init()
{
    int error = 0;
    m_swr_ctx = swr_alloc_set_opts(m_swr_ctx,
            m_out_channel_layout,
            m_out_sample_format,
            m_out_sample_rate,

            m_in_channel_layout,
            m_in_sample_format,
            m_in_sample_rate,
            0,
            nullptr);

    if(!m_swr_ctx)
    {
        enqueue_error("Failed to allocate SwrContext");
        return STATUS_FAILURE;
    }

    error = swr_init(m_swr_ctx);
    if(error < 0)
    {
        enqueue_error("Failed to initialize SwrContext");
        enqueue_error(error);
        return STATUS_FAILURE;
    }


    m_frame = av_frame_alloc();
    if(!m_frame)
    {
        enqueue_error("Failed to allocate frame");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Frame_Resampler::reset_options(int64_t out_channel_layout,
                                                    enum AVSampleFormat out_sample_format, 
                                                    int out_sample_rate, 
                                                    int64_t in_channel_layout, 
                                                    enum AVSampleFormat in_sample_format, 
                                                    int in_sample_rate)
{
    int error = 0;
    m_out_channel_layout = out_channel_layout;
    m_out_sample_format = out_sample_format;
    m_out_sample_rate = out_sample_rate;

    m_in_channel_layout = in_channel_layout;
    m_in_sample_format = in_sample_format;
    m_in_sample_rate = in_sample_rate;

    if(m_swr_ctx)
    {
        error = av_opt_set_channel_layout(m_swr_ctx, "out_channel_layout", m_out_channel_layout, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set out channel layout");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt", m_out_sample_format, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set out sample format");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = av_opt_set_int(m_swr_ctx, "out_sample_rate", m_out_sample_rate, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set out sample rate");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = av_opt_set_channel_layout(m_swr_ctx, "in_channel_layout", m_in_channel_layout, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set in channel layout");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt", m_in_sample_format, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set in sample format");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = av_opt_set_int(m_swr_ctx, "in_sample_rate", m_in_sample_rate, 0);
        if(error < 0)
        {
            enqueue_error("Failed to set in sample rate");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        error = swr_init(m_swr_ctx);
        if(error < 0)
        {
            enqueue_error("Failed to reinitialize SwrContext / Resampling context");
            enqueue_error(error);
            return STATUS_FAILURE;
        }
    }
    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Frame_Resampler::reset_channel_layout(bool out, int64_t new_channel_layout)
{
    if(out)
    {
        m_out_channel_layout = new_channel_layout;
    }
    else
    {
        m_in_channel_layout = new_channel_layout;
    }

    if(m_swr_ctx)
    {
        int error = 0;

        if(out)
        {
            error = av_opt_set_channel_layout(m_swr_ctx, "out_channel_layout", m_out_channel_layout, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set out channel layout");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        else
        {
            error = av_opt_set_channel_layout(m_swr_ctx, "in_channel_layout", m_in_channel_layout, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set in channel layout");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        error = swr_init(m_swr_ctx);
        if(error < 0)
        {
            enqueue_error("Failed to reinitialize SwrContext / Resampling context");
            enqueue_error(error);
            return STATUS_FAILURE;
        }
    }

    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Frame_Resampler::reset_sample_format(bool out, enum AVSampleFormat new_sample_format)
{
    if(out)
    {
        m_out_sample_format = new_sample_format;
    }
    else
    {
        m_in_sample_format= new_sample_format;
    }

    if(m_swr_ctx)
    {
        int error = 0;

        if(out)
        {
            error = av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt", m_out_sample_format, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set out sample format");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        else
        {
            error = av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt", m_in_sample_format, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set in sample format");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        error = swr_init(m_swr_ctx);
        if(error < 0)
        {
            enqueue_error("Failed to reinitialize SwrContext / Resampling context");
            enqueue_error(error);
            return STATUS_FAILURE;
        }
    }

    return STATUS_SUCCESS;
}

Return_Status FFmpeg_Frame_Resampler::reset_sample_rate(bool out, int new_sample_rate)
{
    if(out)
    {
        m_out_sample_rate = new_sample_rate;
    }
    else
    {
        m_in_sample_rate= new_sample_rate;
    }

    if(m_swr_ctx)
    {
        int error = 0;

        if(out)
        {
            error = av_opt_set_int(m_swr_ctx, "out_sample_rate", m_out_sample_rate, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set out sample rate");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        else
        {
            error = av_opt_set_int(m_swr_ctx, "in_sample_rate", m_in_sample_rate, 0);
            if(error < 0)
            {
                enqueue_error("Failed to set in sample rate");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        error = swr_init(m_swr_ctx);
        if(error < 0)
        {
            enqueue_error("Failed to reinitialize SwrContext / Resampling context");
            enqueue_error(error);
            return STATUS_FAILURE;
        }
    }

    return STATUS_SUCCESS;
}

AVFrame *FFmpeg_Frame_Resampler::resample_frame(AVFrame *source_frame)
{
    if(!m_frame || !m_swr_ctx)
    {
        enqueue_error("Resampler not initialized");
        return nullptr;
    }

    int error = 0;

    av_frame_unref(m_frame);

    m_frame->channel_layout = m_out_channel_layout;
    m_frame->format = m_out_sample_format;
    m_frame->sample_rate = m_out_sample_rate;

    error = swr_convert_frame(m_swr_ctx, m_frame, source_frame);
    if(error < 0)
    {
        enqueue_error("Failed to convert frame");
        enqueue_error(error);
        return nullptr;
    }

    return m_frame;
}

std::string FFmpeg_Frame_Resampler::poll_error()
{
    if(!m_errors.empty())
    {
        std::string return_val = m_errors.front();
        m_errors.pop();
        return return_val;
    }

    return std::string{};
}

void FFmpeg_Frame_Resampler::enqueue_error(const std::string &error)
{
    m_errors.push(error);
}

void FFmpeg_Frame_Resampler::enqueue_error(int error_code)
{
    char buff[256];
    int error = av_strerror(error_code, buff, sizeof(buff));

    if(error < 0)
    {
        m_errors.push("Unknown Error");
    }

    else
    {
        m_errors.push(std::string{buff});
    }
}
