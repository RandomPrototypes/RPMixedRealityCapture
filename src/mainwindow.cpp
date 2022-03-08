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

#include "CalibrateCameraPosePage.h"
#include "CalibrateWithChessboardPage.h"
#include "CalibrationOptionPage.h"
#include "CameraPreviewPage.h"
#include "CameraSelectPage.h"
#include "CheckCalibrationPage.h"
#include "ConnectQuestPage.h"
#include "PostProcessingPage.h"
#include "RecordMixedRealityPage.h"

using namespace RPCameraInterface;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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

    calibrateWithChessboardPage = new CalibrateWithChessboardPage(this);
    calibrateCameraPosePage = new CalibrateCameraPosePage(this);
    calibrationOptionPage = new CalibrationOptionPage(this);
    cameraSelectPage = new CameraSelectPage(this);
    cameraPreviewPage = new CameraPreviewPage(this);
    checkCalibrationPage = new CheckCalibrationPage(this);
    connectQuestPage = new ConnectQuestPage(this);
    recordMixedRealityPage = new RecordMixedRealityPage(this);
    postProcessingPage = new PostProcessingPage(this);


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
    recording_finished = false;
    /*cv::VideoCapture cap(cameraId);

    //cap.set(cv::CAP_PROP_FRAME_WIDTH,1920);
    //cap.set(cv::CAP_PROP_FRAME_HEIGHT,1080);

    cap.set(cv::CAP_PROP_FRAME_WIDTH,1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,720);*/

    std::shared_ptr<CameraInterface> cam = getCameraInterface(listCameraEnumerator[currentCameraEnumId]->backend);
    if(!cam->open(cameraId))
    {
        qDebug() << cam->getErrorMsg().c_str();
        return ;
    }
    std::vector<ImageFormat> listFormats = cam->getAvailableFormats();
    int selectedFormat = -1;
    for(size_t i = 0; i < listFormats.size(); i++)
    {
        if(listFormats[i].type == ImageType::JPG && listFormats[i].width == 1920 && listFormats[i].height == 1080)
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
    dstFormat.type = ImageType::BGR24;
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
}

void MainWindow::questThreadFunc()
{
    libQuestMR::QuestVideoSourceBufferedSocket videoSrc;
    videoSrc.Connect(questIpAddress);
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
            videoSrc.Connect(questIpAddress);
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
    else if(currentPageName == PageName::postProcessing)
        postProcessingPage->onTimer();

    if(questInput->hasNewImg && questPreviewWidget != NULL)
    {
        cv::Mat img = questInput->getImgCopy();
        questPreviewWidget->setImg(img);
    }
}

void MainWindow::onClickStartButton()
{
    connectQuestPage->setPage();
}

void MainWindow::onClickPreviewWidget()
{
    if(currentPageName == PageName::recalibratePose)
        calibrateCameraPosePage->onClickPreviewWidget();
    else if(currentPageName == PageName::calibrateWithChessboard)
        calibrateWithChessboardPage->onClickPreviewWidget();
}
