#include "mainwindow.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QImage>
#include <QMessageBox>
#include <QFileDialog>
#include <thread>
#include <chrono>
#include <qprocess.h>
#include <QTextEdit>
#include <QDebug>

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
    closeQuestThreadAfterRecording = false;

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
    font.setPointSize(20);
    textLabel->setFont(font);

    QPushButton *startButton = new QPushButton("start");

    startButton->setStyleSheet("font-size: 20px;");


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
    recording_finished_camera = false;
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
        cam->selectFormat(listCameraFormats[currentCameraFormatId]);


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
        qDebug() << "recv frame " << camImg->getImageFormat().toString().c_str();
        if(recording)
        {
            if(!recordingStarted)
            {
                if(cam->hasRecordingCapability()) {
                    cam->startRecording();
                } else if(videoEncoder == NULL) {
                    recordedVideoTimestampFilename = record_folder+"/"+record_name+"_camTimestamp.txt";
                    recordedVideoFilename = record_folder+"/"+record_name+"_cam.mp4";
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
                recordedVideoFilename = record_folder+"/"+record_name+"_cam";
                recordedVideoTimestampFilename = record_folder+"/"+record_name+"_camTimestamp.txt";
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
            recording_finished_camera = true;
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
    recording_finished_quest = false;
    questVideoSrc = libQuestMR::createQuestVideoSourceBufferedSocket();
    //videoSrc->setUseThread(false, 0);
    questVideoSrc->Connect(questIpAddress.c_str());
    questVideoMngr->attachSource(questVideoSrc);
    bool was_recording = recording;
    if(recording)
        questVideoMngr->setRecording(record_folder.c_str(), record_name.c_str());
    int lastFrameId = -1;
    while(!questInput->closed)
    {
        if(was_recording != recording)
        {
            qDebug() << "videoSrc->Disconnect()";
            questVideoSrc->Disconnect();
            while(was_recording && questVideoSrc->getBufferedDataLength() > 0) {
                qDebug() << "remaining buffer data: " << questVideoSrc->getBufferedDataLength();
                questVideoMngr->VideoTickImpl(true);
            }
            recording_finished_quest = was_recording;
            qDebug() << "questVideoMngr->detachSource()";
            questVideoMngr->detachSource();
            if(was_recording && closeQuestThreadAfterRecording) {
                qDebug() << "questThreadFunc closed after recording";
                return ;
            }
            qDebug() << "videoSrc->Connect()";
            if(!questVideoSrc->Connect(questIpAddress.c_str())) {
                qDebug() << "Unable to reconnect to the quest";
                break;
            }
            qDebug() << "questVideoMngr->attachSource()";
            questVideoMngr->attachSource(questVideoSrc);
            if(recording)
                questVideoMngr->setRecording(record_folder.c_str(), record_name.c_str());
            was_recording = recording;
            lastFrameId = -1;
        }
        qDebug() << "buffered data: " << questVideoSrc->getBufferedDataLength();
        questVideoMngr->VideoTickImpl(true);
        uint64_t timestamp;
        int frameId;
        qDebug() << "questVideoMngr->getMostRecentImg()";
        cv::Mat img = questVideoMngr->getMostRecentImg(&timestamp, &frameId);
        if(frameId != lastFrameId && !img.empty())
        {
            questInput->setImg(img, timestamp);
            //qDebug() << "img timestamp " << timestamp;
        }
        lastFrameId = frameId;
    }
    qDebug() << "questVideoMngr->detachSource()";
    questVideoMngr->detachSource();
    qDebug() << "videoSrc->Disconnect()";
    questVideoSrc->Disconnect();
    qDebug() << "questThreadFunc closed";
    questVideoSrc = NULL;
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
        qDebug() << "start camera...\n";
        videoInput->videoThread = new std::thread([&]()
            {
                videoThreadFunc(cameraId);
            }
        );
    }
}

void MainWindow::startQuestRecorder()
{
    if(questInput->videoThread == NULL) {
        questInput->videoThread = new std::thread([&]()
            {
                questThreadFunc();
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
    if(videoInput->videoThread != NULL && !videoInput->closed) {
        qDebug() << "stop camera...\n";
        videoInput->closed = true;
        videoInput->videoThread->join();
        delete videoInput->videoThread;
        videoInput->videoThread = NULL;
        qDebug() << "camera stopped\n";
    }
}

void MainWindow::stopQuestRecorder()
{
    if(questInput->videoThread != NULL && !questInput->closed) {
        qDebug() << "stop quest recorder...\n";
        questInput->closed = true;

        questInput->videoThread->join();
        delete questInput->videoThread;
        qDebug() << "quest recorder stopped\n";
        questInput->videoThread = NULL;
        closeQuestThreadAfterRecording = false;
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

cv::Mat MainWindow::alphaBlendingMat(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& alphaMask)
{
    cv::Mat result = cv::Mat(img1.size(), CV_8UC3);
    for(int i = 0; i < result.rows; i++)
    {
        unsigned char *dst = result.ptr<unsigned char>(i);
        const unsigned char *src1 = img1.ptr<unsigned char>(i);
        const unsigned char *src2 = img2.ptr<unsigned char>(i);
        const unsigned char *alphaPtr = alphaMask.ptr<unsigned char>(i);
        for(int j = 0; j < result.cols; j++)
        {
            unsigned short alpha = *alphaPtr;
            unsigned short beta = 255 - alpha;
            for(int k = 3; k > 0; k--) {
                *dst++ = (alpha * (*src1++) + beta * (*src2++))/255;
            }
            alphaPtr++;
        }
    }
    return result;
}

cv::Mat MainWindow::composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, cv::Point camTranslation, const std::shared_ptr<libQuestMR::BackgroundSubtractor>& camBackgroundSubtractor, int camSubsampling, int camErosion, bool showForeground, const std::shared_ptr<libQuestMR::BackgroundSubtractor>& questBackgroundSubtractor, int questSubsampling, int questErosion, cv::Rect playAreaROI, cv::Mat playAreaMask, cv::Size videoSize, bool useQuestImg, bool useCamImg, bool useMatteImg, bool useGreenBackground, bool useBlackBackground)
{
    cv::Mat background;
    cv::Mat middleImg;
    if(useGreenBackground) {
        background = cv::Mat(videoSize, CV_8UC3);
        background.setTo(cv::Scalar(0,255,0));
    } else if(useBlackBackground) {
        background = cv::Mat(videoSize, CV_8UC3);
        background.setTo(cv::Scalar(0,0,0));
    } else if(useQuestImg && !questImg.empty()) {
        background = questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows));
        cv::resize(background, background, videoSize);
    }
    cv::Mat fgmask;
    if(useCamImg || useMatteImg) {
        if(!camImg.empty()) {
            cv::Mat camImg2;
            if(camTranslation.x == 0 && camTranslation.y == 0) {
                camImg2 = camImg;
            } else {
                camImg2 = cv::Mat::zeros(camImg.size(), camImg.type());
                cv::Rect srcRoi(std::max(-camTranslation.x, 0), std::max(-camTranslation.y, 0), camImg.cols - abs(camTranslation.x), camImg.rows - abs(camTranslation.y));
                cv::Rect dstRoi(std::max(camTranslation.x, 0), std::max(camTranslation.y, 0), camImg.cols - abs(camTranslation.x), camImg.rows - abs(camTranslation.y));
                camImg(srcRoi).copyTo(camImg2(dstRoi));
            }
            cv::Mat element = cv::getStructuringElement( cv::MORPH_CROSS, cv::Size(3,3), cv::Point(1,1));

            if(camBackgroundSubtractor != NULL && (!background.empty() || useMatteImg)) {
                camBackgroundSubtractor->setROI(adjustROIWithSubsampling(playAreaROI, camSubsampling));
                cv::Mat subCamImg;
                if(camSubsampling != 1) {
                    cv::resize(camImg2, subCamImg, cv::Size(camImg2.cols/camSubsampling, camImg2.rows/camSubsampling));
                } else {
                    subCamImg = camImg2;
                }
                camBackgroundSubtractor->apply(subCamImg, fgmask);
                if(camSubsampling != 1)
                    cv::resize(fgmask, fgmask, cv::Size(camImg2.cols, camImg2.rows));
                for(int i = 0; i < camErosion; i++)
                    cv::erode(fgmask, fgmask, element);
                for(int i = 0; i < -camErosion; i++)
                    cv::dilate(fgmask, fgmask, element);
                cv::bitwise_and(fgmask, playAreaMask, fgmask);
            } else {
                fgmask = playAreaMask.clone();
            }

            if(questBackgroundSubtractor != NULL && useQuestImg && !background.empty()) {
                cv::Rect roi(0,0,videoSize.width,videoSize.height);
                questBackgroundSubtractor->setROI(adjustROIWithSubsampling(roi, questSubsampling));
                cv::Mat subBackground;
                if(questSubsampling != 1) {
                    cv::resize(background, subBackground, cv::Size(background.cols/questSubsampling, background.rows/questSubsampling));
                } else {
                    subBackground = background;
                }
                cv::Mat bgmask;
                questBackgroundSubtractor->apply(subBackground, bgmask);
                if(questSubsampling != 1)
                    cv::resize(bgmask, bgmask, cv::Size(background.cols, background.rows));
                for(int i = 0; i < questErosion; i++)
                    cv::erode(bgmask, bgmask, element);
                for(int i = 0; i < -questErosion; i++)
                    cv::dilate(bgmask, bgmask, element);
                cv::bitwise_not(bgmask, bgmask);
                cv::bitwise_or(fgmask, bgmask, fgmask);
                //cv::cvtColor(fgmask, middleImg, cv::COLOR_GRAY2BGR);
                //background = cv::Mat();
            }

            if(useMatteImg) {
                cv::cvtColor(fgmask, middleImg, cv::COLOR_GRAY2BGR);
                background = cv::Mat();
            } else {
                middleImg = camImg2.clone();
            }
        }
    }
    cv::Mat result = background;
    if(!middleImg.empty()) {
        if(!fgmask.empty() && !background.empty())
            result = alphaBlendingMat(middleImg, background, fgmask);
        else result = middleImg;
    }
    if(useQuestImg && !questImg.empty() && showForeground)
    {
        cv::Mat fgImg, fgAlpha;
        cv::resize(questImg(cv::Rect(questImg.cols/2,0,questImg.cols/4,questImg.rows)), fgImg, result.size());
        cv::resize(questImg(cv::Rect(questImg.cols*3/4,0,questImg.cols/4,questImg.rows)), fgAlpha, result.size());
        cv::cvtColor(fgAlpha, fgAlpha, cv::COLOR_BGR2GRAY);
        result = alphaBlendingMat(fgImg, result, fgAlpha);
    }
    return result;
}

cv::Mat MainWindow::composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, const MixedRealityCompositorConfig& config)
{
    return composeMixedRealityImg(questImg, camImg, config.camTranslation, config.camBackgroundSubtractor, config.camSubsampling, config.camErosion, config.showForeground, config.questBackgroundSubtractor, config.questSubsampling, config.questErosion, config.playAreaROI, config.playAreaMask, config.videoSize, config.useQuestImg, config.useCamImg, config.useMatteImg, config.useGreenBackground, config.useBlackBackground);
}

cv::Rect MainWindow::adjustROIWithSubsampling(cv::Rect ROI, int subsampling)
{
    return cv::Rect(ROI.x/subsampling, ROI.y/subsampling, ROI.width/subsampling, ROI.height/subsampling);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopCamera();
    stopQuestCommunicator();
    stopQuestRecorder();
}
