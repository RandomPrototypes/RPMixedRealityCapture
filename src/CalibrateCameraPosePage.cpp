#include "mainwindow.h"
#include <QMessageBox>
#include <QDebug>

#include "CalibrateCameraPosePage.h"
#include "CheckCalibrationPage.h"
#include "CalibrationOptionPage.h"

#include <libQuestMR/QuestCalibData.h>

CalibrateCameraPosePage::CalibrateCameraPosePage(MainWindow *win)
    :win(win)
{
    estimateIntrinsic = false;
}


void CalibrateCameraPosePage::setPage()
{
    win->listCalibrationFrames.clear();
    win->currentPageName = MainWindow::PageName::recalibratePose;
    state = CalibState::capture;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,30);
    //layout->setAlignment(Qt::AlignTop);

    win->instructionLabel = new QLabel;
    win->instructionLabel->setText("Capture a few calibration frames by moving the right controller in the scene and pressing the trigger button.\nFor best result, stay still for one second before pressing the trigger.");
    win->instructionLabel->setMaximumHeight(100);
    //hlayout = new QHBoxLayout();
    //QPushButton *captureFrameButton = new QPushButton("capture frame");

    nextButton = new QPushButton("next");

    //hlayout->addWidget(captureFrameButton);
    //hlayout->addWidget(nextButton);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    win->camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(win->instructionLabel);
    //layout->addLayout(hlayout);
    layout->addWidget(nextButton);
    layout->addWidget(win->camPreviewWidget);
    layout->addWidget(backToMenuButton);

    win->mainWidget->setLayout(layout);

    win->startCamera();

    //connect(captureFrameButton,SIGNAL(clicked()),this,SLOT(onClickCaptureFrameButton()));
    connect(nextButton,SIGNAL(clicked()),this,SLOT(onClickAnnotateCalibFrameButton()));
    connect(win->camPreviewWidget,SIGNAL(clicked()),this,SLOT(onClickPreviewWidget()));
    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));


    if(win->questComThreadData != NULL)
        currentTriggerCount = win->questComThreadData->getTriggerCount();
}

void CalibrateCameraPosePage::setEstimateIntrinsic(bool val)
{
    estimateIntrinsic = val;
}

void CalibrateCameraPosePage::capturePoseCalibFrame()
{
    if(win->lastFrameData.isRightHandValid())
    {
        CalibrationFrame frame;
        frame.img = win->videoInput->getImgCopy();
        frame.frameData = win->lastFrameData;
        win->listCalibrationFrames.push_back(frame);
    }
}

bool CalibrateCameraPosePage::calibratePose()
{
    std::vector<cv::Point3d> listPoint3d;
    std::vector<cv::Point2d> listPoint2d;
    for(size_t i = 0; i < win->listCalibrationFrames.size(); i++)
    {
        listPoint3d.push_back(win->listCalibrationFrames[i].frameData.getRightHandPos());
        listPoint2d.push_back(win->listCalibrationFrames[i].rightControllerImgPos);
    }
    //return calibData.calibrateCamPose(listPoints3D, listPoints2D);;
    return true;
}

void CalibrateCameraPosePage::onClickCaptureFrameButton()
{
    capturePoseCalibFrame();
}

void CalibrateCameraPosePage::onClickAnnotateCalibFrameButton()
{
    if(!estimateIntrinsic && win->listCalibrationFrames.size() < 4) {
        QMessageBox msgBox;
        msgBox.setText("You must capture at least 4 frames for calibration!!!");
        msgBox.exec();
    } else if(estimateIntrinsic && win->listCalibrationFrames.size() < 6) {
        QMessageBox msgBox;
        msgBox.setText("You must capture at least 6 frames for calibration!!!");
        msgBox.exec();
    } else {
        win->currentCalibrationFrame = 0;
        state = CalibState::annotate;
        win->camPreviewWidget->drawCursor = true;
        win->instructionLabel->setText("click on the center of the right controller on each frame\n");
        //clearLayout(hlayout);
        nextButton->setEnabled(false);
    }
}

void CalibrateCameraPosePage::onTimer()
{
    if(state == CalibState::capture) {
        if(win->videoInput->hasNewImg)
            win->camPreviewWidget->setImg(win->videoInput->getImgCopy());

        if(win->questComThreadData != NULL && win->questComThreadData->getTriggerCount() > currentTriggerCount)
        {
            qDebug() << "trigger";
            capturePoseCalibFrame();
            currentTriggerCount = win->questComThreadData->getTriggerCount();
        }
    } else if(state == CalibState::annotate) {
        if(win->currentCalibrationFrame < win->listCalibrationFrames.size())
        {
            cv::Mat img = win->listCalibrationFrames[win->currentCalibrationFrame].img.clone();
            if(win->listCalibrationFrames[win->currentCalibrationFrame].rightControllerImgPos.x >= 0)
                cv::circle(img, win->listCalibrationFrames[win->currentCalibrationFrame].rightControllerImgPos, 3, cv::Scalar(0,255,0), 2);
            win->camPreviewWidget->setImg(img);
        }
    } else if(state == CalibState::waitingCalibrationUpload) {
        qDebug() << "waitingCalibrationUpload";
        if(win->questComThreadData->isCalibDataUploaded())
            win->checkCalibrationPage->setPage();
    }
}

void CalibrateCameraPosePage::onClickPreviewWidget()
{
    if(state == CalibState::annotate)
    {
        win->listCalibrationFrames[win->currentCalibrationFrame].rightControllerImgPos = win->camPreviewWidget->localToImgPos(win->camPreviewWidget->mousePos);
        if(win->currentCalibrationFrame + 1 == win->listCalibrationFrames.size())
        {
            auto calibDataStr = win->questComThreadData->getCalibData();
            if(calibDataStr.size() > 0)
            {
                libQuestMR::QuestCalibData calibData;
                calibData.loadXMLString(calibDataStr.c_str());
                std::vector<cv::Point3d> listHand3D;
                std::vector<cv::Point2d> listHand2D;
                for(int i = 0; i < win->listCalibrationFrames.size(); i++)
                {
                    listHand2D.push_back(win->listCalibrationFrames[i].rightControllerImgPos);
                    listHand3D.push_back(win->listCalibrationFrames[i].frameData.getRightHandPos());
                }
                if(estimateIntrinsic) {
                    calibData.calibrateCamIntrinsicAndPose(listHand3D, listHand2D, win->listCalibrationFrames[0].img.size());
                } else {
                    calibData.calibrateCamPose(listHand3D, listHand2D);
                }
                calibDataStr = calibData.generateXMLString();
                win->questComThreadData->sendCalibDataToQuest(calibDataStr);
                state = CalibState::waitingCalibrationUpload;
                qDebug() << calibDataStr.c_str();
            }
        }
        else win->currentCalibrationFrame = (win->currentCalibrationFrame+1) % win->listCalibrationFrames.size();
    }
}

void CalibrateCameraPosePage::onClickBackToMenuButton()
{
    //win->stopCamera();
    win->calibrationOptionPage->setPage();
}

