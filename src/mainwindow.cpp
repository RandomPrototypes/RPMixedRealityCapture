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
#include <libQuestMR/QuestCalibData.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include <RPCameraInterface/VideoEncoder.h>
#include <RPCameraInterface/OpenCVConverter.h>

#include "FirstMenuPage.h"
#include "CalibrateCameraPosePage.h"
#include "CalibrateWithChessboardPage.h"
#include "CalibrateCameraQuestMenuPage.h"
#include "CalibrationOptionPage.h"
#include "CameraPreviewPage.h"
#include "CameraSelectPage.h"
#include "CheckCalibrationPage.h"
#include "ConnectQuestPage.h"
#include "PostProcessingOptionPage.h"
#include "PostProcessingPage.h"
#include "RecordMixedRealityPage.h"

using namespace RPCameraInterface;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    mainWidget = new QWidget();

    videoInput = new VideoInputMngr();
    questInput = new VideoInputMngr();

    questVideoMngr = libQuestMR::createQuestVideoMngr();

    questCom = libQuestMR::createQuestCommunicator();
    questComThreadData = NULL;
    questCommunicatorThread = NULL;

    camPreviewWidget = NULL;

    recording = false;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    setFrontPage();

    setCentralWidget(mainWidget);

    firstMenuPage = new FirstMenuPage(this);
    calibrateWithChessboardPage = new CalibrateWithChessboardPage(this);
    calibrateCameraPosePage = new CalibrateCameraPosePage(this);
    calibrateCameraQuestMenuPage = new CalibrateCameraQuestMenuPage(this);
    calibrationOptionPage = new CalibrationOptionPage(this);
    cameraSelectPage = new CameraSelectPage(this);
    cameraPreviewPage = new CameraPreviewPage(this);
    checkCalibrationPage = new CheckCalibrationPage(this);
    connectQuestPage = new ConnectQuestPage(this);
    recordMixedRealityPage = new RecordMixedRealityPage(this);
    postProcessingOptionPage = new PostProcessingOptionPage(this);
    postProcessingPage = new PostProcessingPage(this);


    timer->start();

}

MainWindow::~MainWindow()
{
    delete videoInput;
    delete questInput;
    stopQuestCommunicator();
}

void MainWindow::setFrontPage()
{
    currentPageName = PageName::frontPage;
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

void MainWindow::questCommunicatorThreadFunc()
{
    if(!questCom->connect(questIpAddress.c_str(), 25671))
    {
        questConnectionStatus = QuestConnectionStatus::ConnectionFailed;
        return ;
    }
    questConnectionStatus = QuestConnectionStatus::Connected;
    questComThreadData = libQuestMR::createQuestCommunicatorThreadData(questCom);
    libQuestMR::QuestCommunicatorThreadFunc(questComThreadData.get());
}

void MainWindow::videoThreadFunc(std::string cameraId)
{
    videoInput->closed = false;
    recording_finished = false;
    /*cv::VideoCapture cap(cameraId);

    //cap.set(cv::CAP_PROP_FRAME_WIDTH,1920);
    //cap.set(cv::CAP_PROP_FRAME_HEIGHT,1080);

    cap.set(cv::CAP_PROP_FRAME_WIDTH,1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,720);*/

    std::shared_ptr<CameraInterface> cam = getCameraInterface(listCameraEnumerator[currentCameraEnumId]->getBackend());
    if(!cam->open(cameraId.c_str()))
    {
        qDebug() << cam->getErrorMsg();
        return ;
    }
    if(currentCameraFormatId >= 0 && currentCameraFormatId < listCameraFormats.size())
        cam->selectFormat(currentCameraFormatId);


    VideoContainerType videoContainerType = VideoContainerType::NONE;
    std::vector<VideoContainerType> listContainers = cam->getListAvailableVideoContainer();
    if(listContainers.size() > 0) {
        videoContainerType = listContainers[0];
        cam->selectVideoContainer(videoContainerType);
    }
    qDebug() << "start capturing";
    if(!cam->startCapturing())
    {
        qDebug() << cam->getErrorMsg();
        return ;
    }

    bool inDeviceRecording = cam->hasRecordingCapability();
    bool recordingStarted = false;

    std::shared_ptr<RPCameraInterface::VideoEncoder> videoEncoder;

    FILE *timestampFile = NULL;

    ImageFormat dstFormat;
    dstFormat.width = 1280;
    dstFormat.height = 720;
    dstFormat.type = ImageType::BGR24;
    ImageFormatConverter converter(listCameraFormats[currentCameraFormatId], dstFormat);

    std::shared_ptr<ImageData> resultImg = RPCameraInterface::createImageData();

    while(!videoInput->closed)
    {
        std::shared_ptr<ImageData> camImg = cam->getNewFrame(true);
        uint64_t timestamp = camImg->getTimestamp();
        converter.convertImage(camImg, resultImg);
        cv::Mat img(resultImg->getImageFormat().height, resultImg->getImageFormat().width, CV_8UC3, resultImg->getDataPtr());
        //cap >> img;
        if(img.empty())
            break;
        qDebug() << "recv frame";
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
                    videoEncoder = RPCameraInterface::createVideoEncoder();
                    videoEncoder->open(recordedVideoFilename.c_str(), 720, 1280, 30);
                }
                recordingStarted = true;
            }
            if(videoEncoder != NULL){
                fprintf(timestampFile, "%llu\n", static_cast<unsigned long long>(timestamp));
                videoEncoder->write(RPCameraInterface::createImageDataFromMat(img, timestamp, false));
            }
        } else if(recordingStarted) {
            if(cam->hasRecordingCapability()) {
                recordedVideoFilename = record_folder+"/camVideo";
                recordedVideoTimestampFilename = recordedVideoFilename+"Timestamp.txt";
                if(videoContainerType == VideoContainerType::MP4)
                    recordedVideoFilename += ".mp4";
                qDebug() << "stop recording and save to file: " << recordedVideoFilename.c_str();
                cam->stopRecordingAndSaveToFile(recordedVideoFilename.c_str(), recordedVideoTimestampFilename.c_str());
            } else {
                fclose(timestampFile);
                timestampFile = NULL;
                videoEncoder->release();
                videoEncoder = NULL;
            }
            recording_finished = true;
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
    if(!cam->stopCapturing())
    {
        qDebug() << cam->getErrorMsg();
        return ;
    }
    qDebug() << "stopped capturing";
    if(!cam->close())
    {
        qDebug() << cam->getErrorMsg();
        return ;
    }

    qDebug() << "closed";
}

void MainWindow::questThreadFunc()
{
    std::shared_ptr<libQuestMR::QuestVideoSourceBufferedSocket> videoSrc = libQuestMR::createQuestVideoSourceBufferedSocket();
    videoSrc->Connect(questIpAddress.c_str());
    questVideoMngr->attachSource(videoSrc);
    bool was_recording = recording;
    if(recording)
        questVideoMngr->setRecording(record_folder.c_str(), "questVid");
    while(!questInput->closed)
    {
        if(was_recording != recording)
        {
            questVideoMngr->detachSource();
            videoSrc->Disconnect();
            videoSrc->Connect(questIpAddress.c_str());
            questVideoMngr->attachSource(videoSrc);
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
    videoSrc->Disconnect();
}

void MainWindow::onTimer()
{
    if(questComThreadData != NULL && questComThreadData->hasNewFrameData())
    {
        while(questComThreadData->hasNewFrameData())
            questComThreadData->getFrameData(&lastFrameData);
    }

    if(currentPageName == PageName::recalibratePose)
        calibrateCameraPosePage->onTimer();
    else if(currentPageName == PageName::calibrateWithChessboard)
        calibrateWithChessboardPage->onTimer();
    else if(currentPageName == PageName::checkCalibration)
        checkCalibrationPage->onTimer();
    else if(currentPageName == PageName::connectToQuest)
        connectQuestPage->onTimer();
    else if(currentPageName == PageName::recordMixedReality)
        recordMixedRealityPage->onTimer();
    else if(currentPageName == PageName::postProcessing)
        postProcessingPage->onTimer();
    else if(currentPageName == PageName::postProcessingOption)
        postProcessingOptionPage->onTimer();

    if(questInput->hasNewImg && questPreviewWidget != NULL)
    {
        cv::Mat img = questInput->getImgCopy();
        questPreviewWidget->setImg(img);
    }
}

void MainWindow::onClickStartButton()
{
    firstMenuPage->setPage();
}

void MainWindow::onClickPreviewWidget()
{
    if(currentPageName == PageName::recalibratePose)
        calibrateCameraPosePage->onClickPreviewWidget();
    else if(currentPageName == PageName::calibrateWithChessboard)
        calibrateWithChessboardPage->onClickPreviewWidget();
}

void MainWindow::startCamera()
{
    if(videoInput->videoThread == NULL) {
        videoInput->videoThread = new std::thread([&]()
            {
                videoThreadFunc(cameraId);
            }
        );
    }
}

void MainWindow::startQuestCommunicator()
{
    if(questCommunicatorThread != NULL) {
        if(questConnectionStatus == QuestConnectionStatus::Connected)
            return ;
        stopQuestCommunicator();
    }
    questCommunicatorThread = new std::thread([&]()
        {
            questCommunicatorThreadFunc();
        }
    );
}

void MainWindow::stopCamera()
{
    if(videoInput->videoThread != NULL) {
        videoInput->closed = true;
        videoInput->videoThread->join();
        videoInput->videoThread = NULL;
    }
}

void MainWindow::stopQuestCommunicator()
{
    if(questCommunicatorThread != NULL) {
        if(questComThreadData != NULL)
            questComThreadData->setFinishedVal(true);
        questCommunicatorThread->join();
        delete questCommunicatorThread;
        questCommunicatorThread = NULL;
        questComThreadData = NULL;
    }
    questConnectionStatus = QuestConnectionStatus::NotConnected;
}
