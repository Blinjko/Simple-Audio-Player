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

/* Audio_Player Class
 * @desc The Audio_Player class utilizes the pulsaudio simple api to play audio from AVFrames
 * @member m_player - pa_simple* the pulseaudio simple player
 * @member m_sample_spec - pa_sample_spec* specifications regarding the samples to be played
 * @member m_sample_format - the format of the samples to be played
 * @member m_channels - number of audio channels
 * @member m_sample_rate - the sample rate of the input audio, EX: 48000 Hz
 * @member m_name - The name of the audio player, for pulseaudio
 * @member m_stream_name - The name of the stream, for pulseaudio
 * @member m_errors - a std::queue<std::string> of error messages
 * @note see audio_player.cpp for comments on functions
 */
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
