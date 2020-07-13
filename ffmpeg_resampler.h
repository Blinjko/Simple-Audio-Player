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

/* FFmpeg_Frame_Resampler Class, resamples AVFrames into a given format
 * @note This Class only works with audio
 * @member m_swr_ctx, struct SwrContext* that is used for libswresample resampling functions
 * @member m_frame, AVFrame* used to hold resampled audio AVFrame
 * @member m_out_channel_layout, the output channel layout
 * @member m_out_sample_format, the output sample format EX: 16 bit native endian
 * @member m_sample_rate, the output sample rate EX: 48000 Hz
 * @member m_in_channel_layout, the input channel layout
 * @member m_in_sample_format, the input sample format
 * @member m_in_sample_rate the input sample rate
 * @member m_errors, a std::queue<std::string> that holds error messages
 */
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
