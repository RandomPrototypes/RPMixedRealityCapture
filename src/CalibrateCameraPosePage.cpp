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
    state = CalibState::captureCamOrig;
    capturedCamOrig = false;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,30);
    //layout->setAlignment(Qt::AlignTop);

    win->instructionLabel = new QLabel;
    win->instructionLabel->setText("Put the right controller as close as possible to the camera and press the trigger button.\nFor best result, stay still for one second before pressing the trigger.");
    win->instructionLabel->setMaximumHeight(100);
    //hlayout = new QHBoxLayout();
    //QPushButton *captureFrameButton = new QPushButton("capture frame");

    nextButton = new QPushButton("Skip");

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

void CalibrateCameraPosePage::onClickAnnotateCalibFrameButton()
{
    if(state == CalibState::captureCamOrig) {
        capturedCamOrig = false;
        win->instructionLabel->setText("Capture a few calibration frames by moving the right controller in the scene and pressing the trigger button.\nFor best result, stay still for one second before pressing the trigger.");
        nextButton->setText("Calibrate");
        state = CalibState::capture;
    } else {
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
}

void CalibrateCameraPosePage::onTimer()
{
    if(state == CalibState::captureCamOrig || state == CalibState::capture) {
        if(win->videoInput->hasNewImg)
            win->camPreviewWidget->setImg(win->videoInput->getImgCopy());

        if(win->questComThreadData != NULL && win->questComThreadData->getTriggerCount() > currentTriggerCount)
        {
            if(state == CalibState::captureCamOrig){
                if(win->lastFrameData.isRightHandValid())
                {
                    const double *pos = win->lastFrameData.right_hand_pos;
                    camOrig = cv::Point3d(pos[0], pos[1], pos[2]);
                    capturedCamOrig = true;
                    win->instructionLabel->setText("Capture a few calibration frames by moving the right controller in the scene and pressing the trigger button.\nFor best result, stay still for one second before pressing the trigger.");
                    nextButton->setText("Calibrate");
                    state = CalibState::capture;
                }
            } else {
                capturePoseCalibFrame();
            }
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
        if(win->questComThreadData->isCalibDataUploaded()) {
            qDebug() << "calibration upload success!!";
            win->checkCalibrationPage->setPage();
        }
    }
}

std::string removeSpecialCharacters(std::string str)
{
   std::string result;
   for(size_t i = 0; i < str.size(); i++){
       if((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z') || (str[i] >= '0' && str[i] <= '9')) {
           result += str[i];
       } else {
           result += ' ';
       }
   }
   return result;
}

void CalibrateCameraPosePage::onClickPreviewWidget()
{
    if(state == CalibState::annotate)
    {
        win->listCalibrationFrames[win->currentCalibrationFrame].rightControllerImgPos = win->camPreviewWidget->mousePos;
        if(win->currentCalibrationFrame + 1 == win->listCalibrationFrames.size())
        {
            auto calibDataStr = win->questComThreadData->getCalibData();
            libQuestMR::QuestCalibData calibData;
            if(calibDataStr.size() > 0)
                calibData.loadXMLString(calibDataStr.c_str());
            calibData.setCameraName(removeSpecialCharacters(win->cameraName).c_str());
            calibData.setImageSize(win->listCalibrationFrames[0].img.size());
            for(int i = 0; i < 3; i++)
                calibData.raw_translation[i] = win->listCalibrationFrames[0].frameData.raw_pos[i];
            for(int i = 0; i < 4; i++)
                calibData.raw_rotation[i] = win->listCalibrationFrames[0].frameData.raw_rot[i];
            std::vector<cv::Point3d> listHand3D;
            std::vector<cv::Point2d> listHand2D;
            for(int i = 0; i < win->listCalibrationFrames.size(); i++)
            {
                listHand2D.push_back(win->listCalibrationFrames[i].rightControllerImgPos);
                listHand3D.push_back(win->listCalibrationFrames[i].frameData.getRightHandPos());
            }
            if(estimateIntrinsic) {
                if(capturedCamOrig) {
                    calibData.calibrateCamIntrinsicAndPose(camOrig, listHand3D, listHand2D);
                } else {
                    calibData.calibrateCamIntrinsicAndPose(listHand3D, listHand2D);
                }
            } else {
                if(capturedCamOrig) {
                    calibData.calibrateCamPose(camOrig, listHand3D, listHand2D);
                } else {
                    calibData.calibrateCamPose(listHand3D, listHand2D);
                }
            }
            calibDataStr = calibData.generateXMLString();
            win->questComThreadData->sendCalibDataToQuest(calibDataStr);
            state = CalibState::waitingCalibrationUpload;
            qDebug() << calibDataStr.c_str();
            qDebug() << "waitingCalibrationUpload";
        }
        else win->currentCalibrationFrame = (win->currentCalibrationFrame+1) % win->listCalibrationFrames.size();
    }
}

void CalibrateCameraPosePage::onClickBackToMenuButton()
{
    //win->stopCamera();
    win->calibrationOptionPage->setPage();
}

