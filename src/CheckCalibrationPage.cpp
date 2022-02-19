#include "mainwindow.h"
#include "QuestCalibData.h"
#include "CheckCalibrationPage.h"

CheckCalibrationPage::CheckCalibrationPage(MainWindow *win)
    :win(win)
{
}

void CheckCalibrationPage::setPage()
{
    win->currentPageName = MainWindow::PageName::checkCalibration;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("check calibration: ");

    win->camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(calibrationLabel);
    layout->addWidget(win->camPreviewWidget);

    win->mainWidget->setLayout(layout);
}

void CheckCalibrationPage::onTimer()
{
    if(win->videoInput->hasNewImg && win->camPreviewWidget != NULL)
    {
        cv::Mat img = win->videoInput->getImgCopy();

        std::string calibDataStr = win->questComThreadData->getCalibData();
        if(!calibDataStr.empty())
        {
            libQuestMR::QuestCalibData calibData;
            calibData.loadXMLString(calibDataStr.c_str());
            libQuestMR::QuestFrameData frameData = win->lastFrameData;

            if(frameData.isHeadValid())
            {
                cv::Point3d headPos(frameData.head_pos[0], frameData.head_pos[1], frameData.head_pos[2]);
                cv::Point2d headPos2d = calibData.projectToCam(headPos);
                cv::circle(img, headPos2d, 10, cv::Scalar(0,0,255), 5);
            }

            if(frameData.isLeftHandValid())
            {
                cv::Point3d leftHandPos(frameData.left_hand_pos[0], frameData.left_hand_pos[1], frameData.left_hand_pos[2]);
                cv::Point2d leftHandPos2d = calibData.projectToCam(leftHandPos);
                cv::circle(img, leftHandPos2d, 10, cv::Scalar(255,0,0), 5);
            }

            if(frameData.isRightHandValid())
            {
                cv::Point3d rightHandPos(frameData.right_hand_pos[0], frameData.right_hand_pos[1], frameData.right_hand_pos[2]);
                cv::Point2d rightHandPos2d = calibData.projectToCam(rightHandPos);
                cv::circle(img, rightHandPos2d, 10, cv::Scalar(0,255,0), 5);
            }
        }
        win->camPreviewWidget->setImg(img);
    }
}
