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
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
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

class FFmpeg_Frame_Resampler
{
    struct SwrContext *m_swr_ctx;
    AVFrame *m_frame;

    int64_t                 m_out_channel_layout;
    enum AVSampleFormat     m_out_sample_format;
    int                     m_out_sample_rate;

    int64_t                 m_in_channel_layout;
    enum AVSampleFormat     m_in_sample_format;
    int                     m_in_sample_rate;

    std::queue<std::string> m_errors;

    public:

    FFmpeg_Frame_Resampler(int64_t, enum AVSampleFormat, int, int64_t, enum AVSampleFormat, int);
    ~FFmpeg_Frame_Resampler();

    Return_Status init();
    Return_Status reset_options(int64_t, enum AVSampleFormat, int, int64_t, enum AVSampleFormat, int);

    AVFrame *resample_frame(AVFrame*);

    Return_Status reset_channel_layout(bool, int64_t);
    Return_Status reset_sample_format(bool, enum AVSampleFormat);
    Return_Status reset_sample_rate(bool, int);
    
    std::string poll_error();

    private:

    void enqueue_error(const std::string &error);
    void enqueue_error(int error_code);
};
