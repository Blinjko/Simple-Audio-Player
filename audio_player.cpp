#include "audio_player.h"

extern "C"
{
#include <pulse/simple.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}

#include <string>
#include <queue>




/* Audio_Player constructor
 * @desc sets varaibles, does not initialze the pa_simple context
 * @param sample_format - the sample format of audio data to be played
 * @param channels - the number of channels in the audio data to be played
 * @param sample_rate - the sample rate of the audio to be played
 * @param name - the name of the pulseaudio context
 * @param stream_name - the stream name for the pulseaudio context
 */
Audio_Player::Audio_Player(pa_sample_format_t sample_format, uint8_t channels, uint32_t sample_rate, const std::string &name, const std::string &stream_name) :
    m_sample_format{sample_format}, m_channels{channels}, m_sample_rate{sample_rate}, m_name{name}, m_stream_name{stream_name}
{
    m_player = nullptr;
}




/* Audio_Player destructor
 * @desc frees the pulseaudio context if allocated
 */
Audio_Player::~Audio_Player()
{
    if(m_player)
    {
        pa_simple_free(m_player);
    }
}




/* Audio_Player::init() function
 * @desc initialze the audio player
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* Audio_Player::play_frame() function
 * @desc plays the given AVFrame*
 * @param frame - The AVFrame containing audio data to be played
 * @note If @param frame is not in the expected format expect undefined behaviour
 * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
 */
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




/* Audio_Player::reset_sample_format() function
 * @desc resets the players sample format, m_sample_format
 * @note in order for new specifications to take affect Audio_Player::init() must be called again
 */
void Audio_Player::reset_sample_format(pa_sample_format_t sample_format)
{
    m_sample_format = sample_format;
}




/* Audio_Player::reset_number_of_channels() function
 * @desc resets the number of audio channels, m_channels
 * @note in order for new specifications to take affect Audio_Player::init() must be called again
 */
void Audio_Player::reset_number_of_channels(uint8_t channels)
{
    m_channels = channels;
}




/* Audio_Player::reset_sample_rate() function
 * @desc resets the sample rate, m_sample_rate
 * @note in order for new specifications to take affect Audio_Player::init() must be called again
 */
void Audio_Player::reset_sample_rate(uint32_t sample_rate)
{
    m_sample_rate = sample_rate;
}



/* Audio_Player::poll_error() function
 * @desc used to get std::string errors enqueued onto m_errors
 * @return error message as std::string, if no errors on enqueued and empty std::string is returned
 */
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




/* Audio_Player::calculate_size() function
 * @desc used to calculate the correct size of the data in an AVFrame
 * @param frame - the AVFrame whos data size is to be calculated
 * @return the calculated data size of the passed AVFrame
 * @note this function is under the private specifier
 */
std::size_t Audio_Player::calculate_size(AVFrame *frame)
{
   return av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, static_cast<enum AVSampleFormat>(frame->format), 0);
}




/* Audio_Player::endqueue_error() function
 * @desc enqueues an std::string error message onto m_errors
 * @note this function is under the private specifier
 */
void Audio_Player::enqueue_error(const std::string &error)
{
    m_errors.push(error);
}
