#include "OpenCVWidget.h"

OpenCVWidget::OpenCVWidget(cv::Size viewSize)
    :viewSize(viewSize)
{
    drawCursor = false;

    aspectRatio = (float)viewSize.width / (float)viewSize.height;

    setMinimumSize(viewSize.width/4, viewSize.height/4);

    setMouseTracking(true);
}

void OpenCVWidget::setImg(const cv::Mat& img)
{
    this->img = img;
    updateImg();
}

void OpenCVWidget::updateViewSize()
{
    int w = this->contentsRect().width();
    int h = this->contentsRect().height();
    w = std::min(w, (int)(aspectRatio * h));
    h = (int)(w / aspectRatio);
    viewSize = cv::Size(w, h);//cv::Size(this->width()-this->contentsMargins().left()-this->contentsMargins().right()-2, this->height()-this->contentsMargins().top()-this->contentsMargins().bottom()-2);
    /*qDebug() << "contentsRect" << this->contentsRect().x() << " " << this->contentsRect().y() << " " << this->contentsRect().width() << " " << this->contentsRect().height();
    qDebug() << "contentsMargins" << this->contentsMargins().left() << " " << this->contentsMargins().top() << " " << this->contentsMargins().right() << " " << this->contentsMargins().bottom();
    qDebug() << "margin" << this->margin();
    qDebug() << "width" << this->width() << " height " << this->height();*/
}

void OpenCVWidget::updateImg()
{
    updateViewSize();
    cv::Mat resizedImg;
    if(img.empty())
        resizedImg = cv::Mat::zeros(viewSize, CV_8UC3);
    else
    {
        cv::resize(img, resizedImg, viewSize);
        if(drawCursor)
        {
            cv::Point2d p = imgToLocalPos(mousePos);
            cv::circle(resizedImg, p, 3, cv::Scalar(0,0,255), 2);
        }
    }
    if(!resizedImg.empty())
        cv::cvtColor(resizedImg, resizedImg, cv::COLOR_BGR2RGB);
    QImage imdisplay((uchar*)resizedImg.data, resizedImg.cols, resizedImg.rows, resizedImg.step, QImage::Format_RGB888);
    setPixmap(QPixmap::fromImage(imdisplay).scaled(this->width(), this->height(), Qt::KeepAspectRatio));
}

cv::Point2d OpenCVWidget::localToImgPos(cv::Point2d p)
{
    p.x = (p.x) * img.cols / viewSize.width;
    p.y = (p.y) * img.rows / viewSize.height;
    return p;
}

cv::Point2d OpenCVWidget::imgToLocalPos(cv::Point2d p)
{
    p.x = (p.x) * viewSize.width / img.cols;
    p.y = (p.y) * viewSize.height / img.rows;
    return p;
}

void OpenCVWidget::mouseMoveEvent(QMouseEvent *ev)
{
    updateViewSize();
    const QPoint pos = ev->pos();
    cv::Point2d p = localToImgPos(cv::Point2d(pos.x(), pos.y()));
    mousePos = p;
    //qDebug() << "(" << pos.x() << "," << pos.y() << "), size ("  << viewSize.width << "," << viewSize.height << ")";
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
