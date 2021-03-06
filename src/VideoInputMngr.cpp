#include "VideoInputMngr.h"

VideoInputMngr::VideoInputMngr()
{
    closed = false;
    hasNewImg = false;
    videoThread = NULL;
}

VideoInputMngr::~VideoInputMngr()
{
    closed = true;
    if(videoThread != NULL)
    {
        videoThread->join();
        delete videoThread;
    }
}

cv::Mat VideoInputMngr::getImgCopy(uint64_t *timestamp)
{
    cv::Mat imgCopy;
    mutex.lock();
    imgCopy = img.clone();
    if(timestamp != NULL)
        *timestamp = this->timestamp;
    hasNewImg = false;
    mutex.unlock();
    return imgCopy;
}

void VideoInputMngr::setImg(const cv::Mat& img, uint64_t timestamp)
{
    mutex.lock();
    this->img = img.clone();
    this->timestamp = timestamp;
    hasNewImg = true;
    mutex.unlock();
}
