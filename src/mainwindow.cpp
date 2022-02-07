#include "mainwindow.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QComboBox>
#include <QImage>
#include <QMessageBox>
#include <QFileDialog>
#include <thread>
#include <chrono>
#include <qprocess.h>
#include <QTextEdit>

#include <opencv2/opencv.hpp>
#include "VideoInputMngr.h"
#include "QuestCalibData.h"
#include "CameraEnumeratorQt.h"
#include "CameraInterfaceV4l2.h"
#include "CameraInterfaceLibWebcam.h"
#include "CameraInterfaceAndroid.h"
#include "ImageFormatConverter.h"

using namespace RPCameraInterface;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    CameraMngr *cameraMngr = CameraMngr::getInstance();
    //cameraMngr->registerEnumAndFactory(new CameraEnumeratorQt(), new CameraInterfaceFactoryV4L2());
    cameraMngr->registerEnumAndFactory(new CameraEnumeratorLibWebcam(), new CameraInterfaceFactoryLibWebcam());
    cameraMngr->registerEnumAndFactory(new CameraEnumeratorAndroid(), new CameraInterfaceFactoryAndroid());
    mainWidget = new QWidget();

    videoInput = new VideoInputMngr();
    questInput = new VideoInputMngr();

    questVideoMngr = new libQuestMR::QuestVideoMngr();

    questComThreadData = NULL;

    camPreviewWidget = NULL;

    recording = false;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    setFrontPage();

    setCentralWidget(mainWidget);


    timer->start();

}

MainWindow::~MainWindow()
{
    delete videoInput;
    delete questInput;
    if(questComThreadData != NULL)
    {
        questComThreadData->setFinishedVal(true);
        questCommunicatorThread->join();
        delete questComThreadData;
        delete questCommunicatorThread;
    }
}

void MainWindow::setFrontPage()
{
    currentPageName = "frontPage";
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 10);

    QLabel *imageLabel = new QLabel;
    QLabel *textLabel = new QLabel;

    QPixmap pixmap("resources/logo.png");
    imageLabel->setPixmap(pixmap);
    textLabel->setText("Welcome to RandomPrototypes' mixed reality capture (RPMRC)");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    QPushButton *startButton = new QPushButton("start");


    layout->setAlignment(Qt::AlignCenter);

    imageLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    layout->addWidget(imageLabel,Qt::AlignCenter);
    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addWidget(startButton,Qt::AlignCenter);

    connect(startButton, &QPushButton::released, this, &MainWindow::onClickStartButton);

    mainWidget->setLayout(layout);
}

void MainWindow::clearMainWidget()
{
    QLayout* layout = mainWidget->layout();
    if (layout != 0)
    {
        clearLayout(layout);
        //qDeleteAll(mainWidget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
        delete layout;
    }
}

bool MainWindow::calibratePose()
{
    std::vector<cv::Point3d> listPoint3d;
    std::vector<cv::Point2d> listPoint2d;
    for(size_t i = 0; i < listCalibrationFrames.size(); i++)
    {
        listPoint3d.push_back(listCalibrationFrames[i].frameData.getRightHandPos());
        listPoint2d.push_back(listCalibrationFrames[i].rightControllerImgPos);
    }
    //return calibData.calibrateCamPose(listPoints3D, listPoints2D);;
    return true;
}

void MainWindow::questCommunicatorThreadFunc()
{
    if(!questCom.connect("192.168.10.105", 25671))
    {
        questConnectionStatus = QuestConnectionStatus::ConnectionFailed;
        return ;
    }
    questConnectionStatus = QuestConnectionStatus::Connected;
    questComThreadData = new libQuestMR::QuestCommunicatorThreadData(&questCom);
    libQuestMR::QuestCommunicatorThreadFunc(questComThreadData);
}

void MainWindow::videoThreadFunc(std::string cameraId)
{
    /*cv::VideoCapture cap(cameraId);

    //cap.set(cv::CAP_PROP_FRAME_WIDTH,1920);
    //cap.set(cv::CAP_PROP_FRAME_HEIGHT,1080);

    cap.set(cv::CAP_PROP_FRAME_WIDTH,1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,720);*/

    CameraInterface *cam = CameraMngr::getInstance()->listCameraEnumAndFactory[currentCameraEnumId].interfaceFactory->createInterface();
    if(!cam->open(cameraId))
    {
        qDebug() << cam->getErrorMsg().c_str();
        return ;
    }
    std::vector<ImageFormat> listFormats = cam->getAvailableFormats();
    int selectedFormat = -1;
    for(size_t i = 0; i < listFormats.size(); i++)
    {
        if(listFormats[i].type == ImageType::MJPG && listFormats[i].width == 1920 && listFormats[i].height == 1080)
        {
            selectedFormat = i;
            break;
        }
    }
    if(selectedFormat < 0 && listFormats.size() > 0)
    {
        selectedFormat = 0;
    }
    cam->selectFormat(selectedFormat);


    VideoContainerType videoContainerType = VideoContainerType::NONE;
    std::vector<VideoContainerType> listContainers = cam->getAvailableVideoContainer();
    if(listContainers.size() > 0) {
        videoContainerType = listContainers[0];
        cam->selectVideoContainer(videoContainerType);
    }
    if(!cam->startCapturing())
    {
        qDebug() << cam->getErrorMsg().c_str();
        return ;
    }

    bool inDeviceRecording = cam->hasRecordingCapability();
    bool recordingStarted = false;

    VideoEncoder *videoEncoder = NULL;

    FILE *timestampFile = NULL;

    ImageFormat dstFormat;
    dstFormat.width = 1280;
    dstFormat.height = 720;
    dstFormat.type = ImageType::BGR;
    ImageFormatConverter converter(listFormats[selectedFormat], dstFormat);

    std::shared_ptr<ImageData> resultImg = std::make_shared<ImageData>();

    while(!videoInput->closed)
    {
        std::shared_ptr<ImageData> camImg = cam->getNewFrame(true);
        uint64_t timestamp = camImg->timestamp;
        converter.convertImage(camImg, resultImg);
        cv::Mat img(resultImg->imageFormat.height, resultImg->imageFormat.width, CV_8UC3, resultImg->data);
        //cap >> img;
        if(img.empty())
            break;
        if(recording)
        {
            if(!recordingStarted)
            {
                if(cam->hasRecordingCapability()) {
                    cam->startRecording();
                } else if(videoEncoder == NULL) {
                    recordedVideoTimestampFilename = record_folder+"/camVideoTimestamp.txt";
                    recordedVideoFilename = record_folder+"/camVideo.h264";
                    timestampFile = fopen(recordedVideoTimestampFilename.c_str(), "w");
                    videoEncoder = new VideoEncoder();
                    videoEncoder->open(recordedVideoFilename.c_str(), cv::Size(1280,720), 30);
                }
                recordingStarted = true;
            }
            if(videoEncoder != NULL){
                fprintf(timestampFile, "%llu\n", static_cast<unsigned long long>(timestamp));
                videoEncoder->write(img);
            }
        } else if(recordingStarted) {
            if(cam->hasRecordingCapability()) {
                recordedVideoFilename = record_folder+"/camVideo";
                recordedVideoTimestampFilename = recordedVideoFilename+"Timestamp.txt";
                if(videoContainerType == VideoContainerType::MP4)
                    recordedVideoFilename += ".mp4";
                qDebug() << "stop recording and save to file: " << recordedVideoFilename.c_str();
                cam->stopRecordingAndSaveToFile(recordedVideoFilename, recordedVideoTimestampFilename);
            } else {
                fclose(timestampFile);
                timestampFile = NULL;
                videoEncoder->release();
                videoEncoder = NULL;
            }
            recordingStarted = false;
        }
        videoInput->setImg(img, timestamp);
        //cv::imshow("img", img);
        //cv::waitKey(100);
    }
    if(videoEncoder != NULL)
    {
        videoEncoder->release();
        fclose(timestampFile);
    }
}

void MainWindow::questThreadFunc()
{
    libQuestMR::QuestVideoSourceSocket videoSrc;
    videoSrc.Connect();
    questVideoMngr->attachSource(&videoSrc);
    bool was_recording = recording;
    if(recording)
        questVideoMngr->setRecording(record_folder.c_str(), "questVid");
    while(!questInput->closed)
    {
        if(was_recording != recording)
        {
            questVideoMngr->detachSource();
            videoSrc.Disconnect();
            videoSrc.Connect();
            questVideoMngr->attachSource(&videoSrc);
            if(recording)
                questVideoMngr->setRecording(record_folder.c_str(), "questVid");
            was_recording = recording;
        }
        questVideoMngr->VideoTickImpl(true);
        uint64_t timestamp;
        cv::Mat img = questVideoMngr->getMostRecentImg(&timestamp);
        if(!img.empty())
        {
            questInput->setImg(img, timestamp);
        }
    }
    questVideoMngr->detachSource();
    videoSrc.Disconnect();
}

void MainWindow::setCheckCalibrationPage()
{
    currentPageName = "checkCalibration";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("check calibration: ");

    camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(calibrationLabel);
    layout->addWidget(camPreviewWidget);

    mainWidget->setLayout(layout);
}

void MainWindow::setCameraPreview()
{
    currentPageName = "cameraPreview";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("calibration: ");

    camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(calibrationLabel);
    layout->addWidget(camPreviewWidget);

    mainWidget->setLayout(layout);
}

void MainWindow::setRecalibratePosePage()
{
    currentPageName = "recalibratePose";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    instructionLabel = new QLabel;
    instructionLabel->setText("Capture a few calibration frames by moving the right controller in the scene and pressing the trigger button.\nFor best result, stay still for one second before pressing the trigger.");

    hlayout = new QHBoxLayout();
    QPushButton *captureFrameButton = new QPushButton("capture frame");
    QPushButton *nextButton = new QPushButton("next");

    hlayout->addWidget(captureFrameButton);
    hlayout->addWidget(nextButton);

    camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    camPreviewWidget->setImg(cv::Mat());
    layout->addWidget(instructionLabel);
    layout->addLayout(hlayout);
    layout->addWidget(camPreviewWidget);

    mainWidget->setLayout(layout);

    connect(captureFrameButton,SIGNAL(clicked()),this,SLOT(onClickCaptureFrameButton()));
    connect(nextButton,SIGNAL(clicked()),this,SLOT(onClickAnnotateCalibFrameButton()));
    connect(camPreviewWidget,SIGNAL(clicked()),this,SLOT(onClickPreviewWidget()));
}

void MainWindow::captureCalibFrame()
{
    if(lastFrameData.isRightHandValid())
    {
        CalibrationFrame frame;
        frame.img = videoInput->getImgCopy();
        frame.frameData = lastFrameData;
        listCalibrationFrames.push_back(frame);
    }
}


void MainWindow::onTimer()
{
    if(questComThreadData != NULL && questComThreadData->hasNewFrameData())
    {
        while(questComThreadData->hasNewFrameData())
            questComThreadData->getFrameData(&lastFrameData);
    }

    if(currentPageName == "recalibratePose")
    {
        if(questComThreadData->getTriggerVal())
        {
            qDebug() << "trigger";
            captureCalibFrame();
            questComThreadData->setTriggerVal(false);
        }
    }

    if(currentPageName == "annotateCalibrationFrames")
    {
        if(currentCalibrationFrame < listCalibrationFrames.size())
        {
            cv::Mat img = listCalibrationFrames[currentCalibrationFrame].img.clone();
            if(listCalibrationFrames[currentCalibrationFrame].rightControllerImgPos.x >= 0)
                cv::circle(img, listCalibrationFrames[currentCalibrationFrame].rightControllerImgPos, 3, cv::Scalar(0,255,0), 2);
            camPreviewWidget->setImg(img);
        }
    }
    else if(videoInput->hasNewImg && camPreviewWidget != NULL)
    {
        cv::Mat img = videoInput->getImgCopy();
        if(currentPageName == "checkCalibration")
        {
            std::string calibDataStr = questComThreadData->getCalibData();
            if(!calibDataStr.empty())
            {
                libQuestMR::QuestCalibData calibData;
                calibData.loadXMLString(calibDataStr.c_str());
                libQuestMR::QuestFrameData frameData = lastFrameData;

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
        }
        camPreviewWidget->setImg(img);
    }

    if(questInput->hasNewImg && questPreviewWidget != NULL)
    {
        cv::Mat img = questInput->getImgCopy();
        questPreviewWidget->setImg(img);
    }

    if(currentPageName == "connectToQuest")
    {
        if(questConnectionStatus == QuestConnectionStatus::ConnectionFailed)
        {
            QMessageBox msgBox;
            msgBox.setText("Can not connect to the quest...");
            msgBox.exec();
            setCameraSelectPage();
            /*questCommunicatorThread->join();
            questCommunicatorThread = NULL;
            questConnectionStatus = QuestConnectionStatus::NotConnected;
            connectButton->setText("connect");
            connectButton->setEnabled(true);*/
        } else if(questConnectionStatus == QuestConnectionStatus::Connected) {
            setCameraSelectPage();
        }
    }

    if(currentPageName == "postProcessing")
    {
        progressBar->setValue((int)(100*postProcessVal));
    }
}

void MainWindow::onClickStartButton()
{
    setConnectToQuestPage();
}

void MainWindow::onClickCaptureFrameButton()
{
    captureCalibFrame();
}


void MainWindow::onClickAnnotateCalibFrameButton()
{
    if(listCalibrationFrames.size() < 4) {
        QMessageBox msgBox;
        msgBox.setText("You must capture at least 4 frames for calibration!!!");
        msgBox.exec();
    } else {
        currentCalibrationFrame = 0;
        currentPageName = "annotateCalibrationFrames";
        camPreviewWidget->drawCursor = true;
        instructionLabel->setText("click on the center of the right controller on each frame\n");
        clearLayout(hlayout);
    }
}

void MainWindow::onClickPreviewWidget()
{
    if(currentPageName == "annotateCalibrationFrames")
    {
        listCalibrationFrames[currentCalibrationFrame].rightControllerImgPos = camPreviewWidget->localToImgPos(camPreviewWidget->mousePos);
        if(currentCalibrationFrame + 1 == listCalibrationFrames.size())
        {
            std::string calibDataStr = questComThreadData->getCalibData();
            if(!calibDataStr.empty())
            {
                libQuestMR::QuestCalibData calibData;
                calibData.loadXMLString(calibDataStr.c_str());
                std::vector<cv::Point3d> listHand3D;
                std::vector<cv::Point2d> listHand2D;
                for(int i = 0; i < listCalibrationFrames.size(); i++)
                {
                    listHand2D.push_back(listCalibrationFrames[i].rightControllerImgPos);
                    listHand3D.push_back(listCalibrationFrames[i].frameData.getRightHandPos());
                }
                calibData.calibrateCamPose(listHand3D, listHand2D);
                calibDataStr = calibData.generateXMLString();
                questComThreadData->sendCalibDataToQuest(calibDataStr);
                qDebug() << calibDataStr.c_str();
                //exit(0);
            }

            setCheckCalibrationPage();
        }
        else currentCalibrationFrame = (currentCalibrationFrame+1) % listCalibrationFrames.size();
    }
}
