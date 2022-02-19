#include "mainwindow.h"
#include "CameraPreviewPage.h"

CameraPreviewPage::CameraPreviewPage(MainWindow *win)
    :win(win)
{

}

void CameraPreviewPage::setPage()
{
    win->currentPageName = MainWindow::PageName::cameraPreview;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("calibration: ");

    win->camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(calibrationLabel);
    layout->addWidget(win->camPreviewWidget);

    win->mainWidget->setLayout(layout);
}
