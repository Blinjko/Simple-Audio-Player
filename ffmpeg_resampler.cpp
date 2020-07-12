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

/* FFmpeg_Frame_Resampler Constructror
 * @param out_channel_layout, the output channel layout
 * @param out_sample_format, the output sample format
 * @param out_sample_rate, the output sample rate
 * @param in_channel_layout, the input channel layout
 * @param in_sample_format, the input sample format
 * @param in_sample_rate, the input sample rate
 */
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




/* FFmpeg_Frame_Resampler Destructor
 * @desc Frees m_swr_ctx if allocated, and unreferences and frees m_frame if allocated
 */
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




/* FFmpeg_Frame_Resampler::init() function
 * @desc initializes the resampler context, must be called before any resampling is done
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Frame_Resampler::reset_options() function
 * @desc resets all m_in* and m_out* variables with the ones passed, and re initializes the resampling context
 * @note if this is called after FFmpeg_Frame_Resampler::init() you don't need to call FFmpeg_Frame_Resampler::init() again,
 * @note as the function reinitializes the context automaticlly.
 * @param out_channel_layout, the output channel layout
 * @param out_sample_format, the output sample format
 * @param out_sample_rate, the output sample rate
 * @param in_channel_layout, the input channel layout
 * @param in_sample_format, the input sample format
 * @param in_sample_rate, the input sample rate
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Frame_Resampler::reset_channel_layout() function
 * @desc resets either input channel layout or output channel layout
 * @param out, a boolean to determine wheather to change the output channel layout or input channel layout, true for output, false for input
 * @param new_channel_layout, the new channel layout
 * @note If called after FFmpeg_Frame_Resampler::init() it is not needed to reinitilze as the function does so automatically.
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Frame_Resampler::reset_sample_format() function
 * @desc resets the input or output sample format
 * @param out, a boolean to determine wheather to change the output or input, true for output, false for input
 * @param new_sample_format, the new sample format
 * @note If called after FFmpeg_Frame_Resampler::init() it is not needed to reinitilze as the function does so automatically.
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Frame_Resampler::reset_sample_rate() function
 * @desc resets the input or output sample rate
 * @param out, a boolean to determine wheather to change the output or input, true for output, false for input
 * @param new_sample_rate, the new sample rate
 * @note If called after FFmpeg_Frame_Resampler::init() it is not needed to reinitilze as the function does so automatically.
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmepg_Frame_Resampler::resample_frame() function
 * @desc resamples a decoded audio frame to the set output options
 * @param source_frame, AVFrame* that holds decoded audio data
 * @return valid AVFrame* on success, nullptr on failure
 * @note the returned AVFrame* points to the same data as m_frame, and when this function is called again,
 * @note the previously returned pointer will be invalid.
 */
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




/* FFmpeg_Frame_Resampler::poll_error() function
 * @desc polls an error message from m_errors and returns it
 * @return std::string error message, the string will be empty if there are no messages.
 */
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




/* FFmpeg_Frame_Resampler::enqueue_error() function
 * @desc enqueues an std::string error onto m_errors
 * @param error, the std::string error message to enqueue onto m_errors
 * @note this function is under the private modifier
 */
void FFmpeg_Frame_Resampler::enqueue_error(const std::string &error)
{
    m_errors.push(error);
}




/* FFmpeg_Frame_Resampler::enqueue_error() function
 * @desc enqueues an ffmpeg error message for the given error code onto m_errors
 * @param error_code - The error code to translate to an error message
 * @note this function is under the private modifier
 */
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
