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
#include "audio_player.h"

extern "C"
{
#include <pulse/simple.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}

#include <string>
#include <queue>


Audio_Player::Audio_Player(pa_sample_format_t sample_format, uint8_t channels, uint32_t sample_rate, const std::string &name, const std::string &stream_name) :
    m_sample_format{sample_format}, m_channels{channels}, m_sample_rate{sample_rate}, m_name{name}, m_stream_name{stream_name}
{
    m_player = nullptr;
}

Audio_Player::~Audio_Player()
{
    if(m_player)
    {
        pa_simple_free(m_player);
    }
}

Return_Status Audio_Player::init()
{
    m_sample_spec.format = m_sample_format;
    m_sample_spec.channels = m_channels;
    m_sample_spec.rate = m_sample_rate;

    if(m_player)
    {
        pa_simple_free(m_player);
        m_player = nullptr;
    }

    m_player = pa_simple_new(nullptr, m_name.c_str(), PA_STREAM_PLAYBACK, nullptr, m_stream_name.c_str(), &m_sample_spec, nullptr, nullptr, nullptr);

    if(!m_player)
    {
        enqueue_error("Failed to create a PulseAudio client");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

Return_Status Audio_Player::play_frame(AVFrame *frame)
{
    if(!m_player)
    {
        enqueue_error("Not initialized");
        return STATUS_FAILURE;
    }

    int error = 0;
    error = pa_simple_write(m_player, frame->extended_data[0], calculate_size(frame), nullptr);

    if(error < 0)
    {
        enqueue_error("Failed to play frame");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

void Audio_Player::reset_sample_format(pa_sample_format_t sample_format)
{
    m_sample_format = sample_format;
}

void Audio_Player::reset_number_of_channels(uint8_t channels)
{
    m_channels = channels;
}

void Audio_Player::reset_sample_rate(uint32_t sample_rate)
{
    m_sample_rate = sample_rate;
}

std::string Audio_Player::poll_error()
{
    if(!m_errors.empty())
    {
        std::string error;
        error = m_errors.front();
        m_errors.pop();

        return error;
    }

    return std::string{};
}


std::size_t Audio_Player::calculate_size(AVFrame *frame)
{
   return av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, static_cast<enum AVSampleFormat>(frame->format), 0);
}

void Audio_Player::enqueue_error(const std::string &error)
{
    m_errors.push(error);
}
