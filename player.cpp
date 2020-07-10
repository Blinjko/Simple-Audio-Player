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
#include "ffmpeg_resampler.h"
#include "audio_player.h"
#include <iostream>
#include <cstdlib>

void poll_errors(FFmpeg_Decoder &decoder)
{
    std::string error = decoder.poll_error();
    do
    {
        std::cerr << error << std::endl;
        error = decoder.poll_error();
    }
    while(!error.empty());

}

void poll_errors(FFmpeg_Frame_Resampler &resampler)
{
    std::string error = resampler.poll_error();
    do
    {
        std::cerr << error << std::endl;
        error = resampler.poll_error();
    }
    while(!error.empty());
}

void poll_errors(Audio_Player &audio_player)
{
    std::string error = audio_player.poll_error();
    do
    {
        std::cerr << error << std::endl;
        error = audio_player.poll_error();
    }
    while(!error.empty());
}

void check_status(FFmpeg_Decoder &decoder, Return_Status status, bool exit)
{
    if(status == STATUS_FAILURE)
    {
        poll_errors(decoder);

        if(exit)
        {
            std::exit(1);
        }
    }
}

void check_status(FFmpeg_Frame_Resampler &resampler, Return_Status status, bool exit)
{
    if(status == STATUS_FAILURE)
    {
        poll_errors(resampler);

        if(exit)
        {
            std::exit(1);
        }
    }
}

void check_status(Audio_Player &audio_player, Return_Status status, bool exit)
{
    if(status == STATUS_FAILURE)
    {
        poll_errors(audio_player);

        if(exit)
        {
            std::exit(1);
        }
    }
}

void main_loop(FFmpeg_Decoder &decoder, FFmpeg_Frame_Resampler &resampler, Audio_Player &audio_player)
{
    AVFrame *decoded_frame;
    AVFrame *resampled_frame;

    int i = 0;
    while(1)
    {
        Return_Status status;
        std::cout << "Iteration: " << i++ << '\n';

        decoded_frame = decoder.decode_frame();

        if(!decoded_frame && decoder.end_of_file_reached())
        {
            std::cout << "End of file reached\n";
            break;
        }

        else if(!decoded_frame)
        {
            poll_errors(decoder);
            std::exit(1);
        }

        if(i == 1)
        {
            status = resampler.reset_channel_layout(false, decoded_frame->channel_layout);
            check_status(resampler, status, true);

            status = resampler.reset_sample_format(false, static_cast<enum AVSampleFormat>(decoded_frame->format));
            check_status(resampler, status, true);

            status = resampler.reset_sample_rate(true, decoded_frame->sample_rate);
            check_status(resampler, status, true);

            status = resampler.reset_sample_rate(false, decoded_frame->sample_rate);
            check_status(resampler, status, true);

            status = resampler.init();
            check_status(resampler, status, true);

            audio_player.reset_sample_rate(decoded_frame->sample_rate);

            status = audio_player.init();
            check_status(audio_player, status, true);

        }

        resampled_frame = resampler.resample_frame(decoded_frame);

        if(!resampled_frame)
        {
            poll_errors(resampler);
            std::exit(1);
        }

        status = audio_player.play_frame(resampled_frame); 
        check_status(audio_player, status, true);


    }
}

int main(int argc, char **argv)
{
    const int NUMBER_CHANNELS = 2;
    const enum AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
    const pa_sample_format_t SAMPLE_FORMAT_PULSE = PA_SAMPLE_S16NE;

    if(argc != 2)
    {
        std::cerr << "Invalid usage\n";
        std::cerr << "Valid Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::cout << "Decoding Audio\n";
    FFmpeg_Decoder decoder{argv[1], AVMEDIA_TYPE_AUDIO};
    Return_Status status;

    status = decoder.open_file();
    check_status(decoder, status, true);

    status = decoder.init();
    check_status(decoder, status, true);

    FFmpeg_Frame_Resampler resampler{
        av_get_default_channel_layout(NUMBER_CHANNELS), // set out channel layout
        SAMPLE_FORMAT,                                  // set out sample format
        0,                                              // set temporary out sample rate, will be set when decoding starts
        0,                                              // set in channel layout, unknown right now, will be set when decoding starts
        AV_SAMPLE_FMT_NONE,                             // set in sample format, unkwonw right now, will be set when decoding starts
        0};                                             // set in sample rate, unkown, will be set when decoding starts

    Audio_Player audio_player{SAMPLE_FORMAT_PULSE, NUMBER_CHANNELS, 0, "Simple Audio Player", std::string{argv[1]}};
    main_loop(decoder, resampler, audio_player);

    return 0;
}
