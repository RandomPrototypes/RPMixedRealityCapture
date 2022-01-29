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

cv::Mat VideoInputMngr::getImgCopy()
{
    cv::Mat imgCopy;
    mutex.lock();
    imgCopy = img.clone();
    hasNewImg = false;
    mutex.unlock();
    return imgCopy;
}

void VideoInputMngr::setImg(const cv::Mat& img)
{
    mutex.lock();
    this->img = img.clone();
    hasNewImg = true;
    mutex.unlock();
}
