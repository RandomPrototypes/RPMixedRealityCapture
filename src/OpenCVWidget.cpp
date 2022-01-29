#include "OpenCVWidget.h"

OpenCVWidget::OpenCVWidget(cv::Size viewSize)
    :viewSize(viewSize)
{
    drawCursor = false;

    setMouseTracking(true);
}

void OpenCVWidget::setImg(const cv::Mat& img)
{
    this->img = img;
    updateImg();
}

void OpenCVWidget::updateImg()
{
    cv::Mat resizedImg;
    if(img.empty())
        resizedImg = cv::Mat::zeros(viewSize, CV_8UC3);
    else
    {
        cv::resize(img, resizedImg, viewSize);
        if(drawCursor)
        {
            cv::Point2d p = localToImgPos(mousePos);
            p.x *= viewSize.width / img.cols;
            p.y *= viewSize.height / img.rows;
            cv::circle(resizedImg, localToImgPos(mousePos), 3, cv::Scalar(0,0,255), 2);
        }
    }
    //cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    QImage imdisplay((uchar*)resizedImg.data, resizedImg.cols, resizedImg.rows, resizedImg.step, QImage::Format_BGR888);
    setPixmap(QPixmap::fromImage(imdisplay));
}

cv::Point2d OpenCVWidget::localToImgPos(cv::Point2d p)
{
    p.x = (p.x) * img.cols / viewSize.width;
    p.y = (p.y) * img.rows / viewSize.height;
    return p;
}

void OpenCVWidget::mouseMoveEvent(QMouseEvent *ev)
{
    const QPoint pos = ev->pos();
    cv::Point2d p = localToImgPos(cv::Point2d(pos.x(), pos.y()));
    mousePos = p;
    updateImg();
    QWidget::mouseMoveEvent(ev);
}

void OpenCVWidget::mousePressEvent(QMouseEvent *ev)
{
    const QPoint pos = ev->pos();
    cv::Point2d p = localToImgPos(cv::Point2d(pos.x(), pos.y()));
    mousePos = p;
    leftPressed = true;
    updateImg();
    QWidget::mousePressEvent(ev);
}

void OpenCVWidget::mouseReleaseEvent(QMouseEvent *ev)
{
    const QPoint pos = ev->pos();
    cv::Point2d p = localToImgPos(cv::Point2d(pos.x(), pos.y()));
    mousePos = p;
    leftPressed = false;
    updateImg();
    QWidget::mouseReleaseEvent(ev);
    emit clicked();
}
