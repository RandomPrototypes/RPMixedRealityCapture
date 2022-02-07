#ifndef VIDEOINPUTMNGR_H
#define VIDEOINPUTMNGR_H

#include <opencv2/opencv.hpp>
#include <thread>

class VideoInputMngr
{
public:
    volatile bool closed;
    volatile bool hasNewImg;
    cv::Mat img;
    uint64_t timestamp;
    std::mutex mutex;
    std::thread *videoThread;

    VideoInputMngr();
    ~VideoInputMngr();
    cv::Mat getImgCopy(uint64_t *timestamp = NULL);
    void setImg(const cv::Mat& img, uint64_t timestamp);
};

#endif // VIDEOINPUTMNGR_H
