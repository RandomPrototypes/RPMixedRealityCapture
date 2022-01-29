#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>

//based on https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html
class VideoEncoder
{
public:
    bool open(const char *filename, cv::Size size, int fps = 30, int bitrate = 2000000, const char *preset = "fast");
    bool write(const cv::Mat& img);
    void release();

    AVCodec *codec;
    AVCodecContext *codecContext = NULL;
    struct SwsContext *sws_context = NULL;
    AVPacket *pkt;
    FILE *file;
    AVFrame *frame;
    int nbEncodedFrames;
};

#endif // VIDEOENCODER_H
