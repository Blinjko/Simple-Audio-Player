
#include "ffmpeg_decoder.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}
#include <string>
#include <queue>


// A LITTLE NOTE //
//
// The functions that return a type of "Return_Status" have the ability to enqueue or set errors
// To get these errors call the FFmpeg_Decoder::poll_error() function.
// NOTE: FFmpeg_Decoder::poll_error() is NOT static
// SIDE NOTE: Genrally when a function returns "Return_Status::STATUS_FAILURE" indicating that it fails
// it will generally enqueue 2 errors, one from the FFmpeg library and one from the code in the function
//
// NOTE END //



/* FFmpeg_Decoder Class constructor
 * @param filename, the file to be opened
 * @param media_type, the media type to be decoded EX: AVMEDIA_TYPE_AUIDO for audio
 * @note This constructor only sets varialbes and does not initialize the decoder
 */
FFmpeg_Decoder::FFmpeg_Decoder(const std::string& filename, enum AVMediaType media_type) : 
    m_filename{filename}, m_media_type{media_type}
{
    m_fmt_ctx = nullptr;
    m_codec_ctx = nullptr;
    m_packet = nullptr;
    m_frame = nullptr;
}




/* FFmpeg_Decoder Class constructor 2
 * @param filename, the file to be opened,
 * @param media_type, the media type to be decoded, EX: AVMEDIA_TYPE_VIDEO for video
 * @note This constructor only sets variables and does not initialize the decoder
 * @note The difference in this constructor is that it takes a "const char*" vs a "std::string"
 */
FFmpeg_Decoder::FFmpeg_Decoder(const char* filename, enum AVMediaType media_type) : FFmpeg_Decoder(std::string{filename}, media_type)
{}




/* FFmpeg_Decoder Class destructor
 * @desc Frees data and resources that require freeing
 */
FFmpeg_Decoder::~FFmpeg_Decoder()
{
    reset("NO NAME", AVMEDIA_TYPE_UNKNOWN);
}




/* FFmpeg_Decoder::open_file function
 * @desc Opens the file passed to the constructor, m_filename, and initializes m_format_ctx
 * @return Return_Status::STATUS_SUCCESS on successful execution, and Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Decoder::init() function, initializes the decoder
 * @note This function must only be called after FFmpeg_Decoder::open_file() has been called.
 * @desc This function allocates, and initializes m_codec_ctx for decoding.
 * @return Return_Status::STATUS_SUCCESS on successful execution, and Return_Status::STATUS_FAILURE on failure
 */
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




/* FFmpeg_Decoder::drain() function, drains the decoder
 * @return Return_Status::STATUS_SUCCESS on successful drain and
 * @return Return_Status::STATUS_FAILURE on an unsuccessful drain.
 */
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




/* FFmpeg_Decoder::reset() function, resets the FFmpeg_Decoder Class
 * @desc This function will free all resources allocated and reset the class as if it were just initialized
 * @param filename, the file to be opened
 * @param media_type, the media type to be decoded EX: AVMEDIA_TYPE_AUDIO
 * @note This function uninitializes the decoder, so the functions FFmpeg_Decoder::open_file(), and
 * @note FFmpeg_Decoder::init() must be called again before the decoder is used.
 */
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




/* FFmpeg_Decoder::decode_frame() function, decodes a frame and returns it
 * @desc This function decodes a frame of the passed AVMediaType
 * @return pointer to an AVFrame on success, nullptr on failure or end of file
 * @note TO CHECK if the end of the file has been reached call FFmepg_Decoder::end_of_file_reached()
 * @note Data referenced by the retured pointer is the same data referenced by m_frame,
 * @note SO DO NOT av_frame_unref() OR av_frame_free() the returned AVFrame, this will mess up the decoder.
 * @note Next time this function is called, the previously returned AVFrame* will be invalid, so do your operations accordingly
 */
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

    // check if frame channel layout is 0
    // if it is we have to set it apropriately
    // or there will be problems hard to decipher down the line
    if(m_frame->channel_layout == 0)
    {
        m_frame->channel_layout = av_get_default_channel_layout(m_frame->channels);
    }

    return m_frame;
}




/* FFmpeg_Decoder::poll_error() function, returns a string error message
 * @return std::string if the queue (m_errors) has any, and returned an empty std::string if the queue is empty
 * @note When functions like FFmpeg_Decoder::init(), encounter errors they will enqueue, 
 * @note an error message or 2 onto m_errors, to get them use this function.
 */
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




/* FFmpeg_Decoder::get_format_context() function
 * @return m_fmt_ctx, a AVFormatContext*
 * @note this function will return nullptr if FFmpeg_Decoder::open_file() hasn't been called.
 */
AVFormatContext *FFmpeg_Decoder::get_format_context()
{
    return m_fmt_ctx;
}




/* FFmpeg_Decoder::get_codec_context() function
 * @return m_codec_ctx, a AVCodecContext*
 * @note this function will return nullptr if FFmpeg_Decoder::init() has not been called.
 */
AVCodecContext *FFmpeg_Decoder::get_codec_context()
{
    return m_codec_ctx;
}




/* FFmpeg_Decoder::get_media_type() function
 * @return m_media_type, a enum AVMediaType
 */
enum AVMediaType FFmpeg_Decoder::get_media_type()
{
    return m_media_type;
}




/* FFmpeg_Decoder::get_filename() function
 * @return m_filename, a std::string
 */
std::string FFmpeg_Decoder::get_filename()
{
    return m_filename;
}




/* FFmpeg_Decoder::end_of_file_reached() function
 * @return m_end_of_file, which indicates weather the end of file has been reached (true) or not (false)
 */
bool FFmpeg_Decoder::end_of_file_reached()
{
    return m_end_of_file;
}




/* FFmpeg_Decoder::decoder_fill() function
 * @desc Fills the decoder with data, called in FFmpeg_Decoder::decode_frame()
 * @return Return_Status::STATUS_SUCCESS on success and Return_Status::STATUS_FAILURE on failure
 * @note If the end of file is reached m_end_of_file will be set
 * @note this function is not public, and thus cannot be called.
 */
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




/* FFmpeg_Decoder::enqueue_error() function, enqueue an ffmpeg error message given an error code
 * @desc This function takes the passed error code and translates it into an FFmpeg error message
 * @desc which then gets pushed onto the m_errors queue.
 * @param error_code, the error code to translate
 * @note NON public function
 */
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




/* FFmpeg_Decoder::enqueue_error() function, enqueue a std::string error message
 * @param message, the std::string message to enqueue to m_errors
 * @note NON public function
 */
void FFmpeg_Decoder::enqueue_error(const std::string &message)
{
    m_errors.push(message);
    return;
}
