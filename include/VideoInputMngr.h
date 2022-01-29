#ifndef VIDEOINPUTMNGR_H
#define VIDEOINPUTMNGR_H

#include <opencv2/opencv.hpp>
#include <thread>

class VideoInputMngr
{
public:
    bool closed;
    bool hasNewImg;
    cv::Mat img;
    std::mutex mutex;
    std::thread *videoThread;

    VideoInputMngr();
    ~VideoInputMngr();
    cv::Mat getImgCopy();
    void setImg(const cv::Mat& img);
};

#endif // VIDEOINPUTMNGR_H
