#ifndef OPENCVWIDGET_H
#define OPENCVWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <opencv2/opencv.hpp>

class OpenCVWidget : public QLabel
{
    Q_OBJECT

public:
    OpenCVWidget(cv::Size viewSize);
    void setImg(const cv::Mat& img);

    void mouseMoveEvent( QMouseEvent* ev );
    void mousePressEvent(QMouseEvent *eventPress);
    void mouseReleaseEvent(QMouseEvent *releaseEvent);

    cv::Point2d localToImgPos(cv::Point2d p);
    void updateImg();
    void updateViewSize();

    cv::Mat img;
    cv::Size viewSize;
    cv::Point2d mousePos;
    bool leftPressed;
    bool drawCursor;
signals:
    void clicked();
public slots:
};

#endif // OPENCVWIDGET_H
