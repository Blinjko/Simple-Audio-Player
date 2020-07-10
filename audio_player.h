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
#include <pulse/simple.h>
#include <libavutil/frame.h>
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

class Audio_Player
{
    pa_simple *m_player;
    pa_sample_spec m_sample_spec;

    pa_sample_format_t m_sample_format;
    uint8_t m_channels;
    uint32_t m_sample_rate;

    std::string m_name;
    std::string m_stream_name;

    std::queue<std::string> m_errors;

    public:

    Audio_Player(pa_sample_format_t, uint8_t, uint32_t, const std::string&, const std::string&);
    ~Audio_Player();

    Return_Status init();
    Return_Status play_frame(AVFrame *);

    void reset_sample_format(pa_sample_format_t);
    void reset_number_of_channels(uint8_t);
    void reset_sample_rate(uint32_t);

    std::string poll_error();

    private:
    
    std::size_t calculate_size(AVFrame *);
    void enqueue_error(const std::string &error);
};
