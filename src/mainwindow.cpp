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

void clearLayout(QLayout *layout) {
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while (auto item = layout->takeAt(0)) {
      delete item->widget();
      clearLayout(item->layout());
   }
}

std::string getCurrentDateTimeStr()
{
     std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
     time_t tt = std::chrono::system_clock::to_time_t(now);
     tm local_tm = *localtime(&tt);
     char buffer[255];
     snprintf(buffer, sizeof(buffer), "%04d%02d%02d%02d%02d%02d", local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
     return buffer;
}


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

    questVideoMngr = new QuestVideoMngr();

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

void MainWindow::processOutput()
{
    qDebug() << segmentationProcess->readAllStandardOutput();
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
    if(!cam->startCapturing())
    {
        qDebug() << cam->getErrorMsg().c_str();
        return ;
    }

    //cv::VideoWriter writer;
    //writer.open("test.mp4", writer.fourcc('H', '2', '6', '4'), 30, cv::Size(1280,720), true);
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
        qDebug() << "readImg";
        //cv::Mat img;
        //while(cap.grab())
            //;
        //cap.retrieve(img);
        std::shared_ptr<ImageData> camImg = cam->getNewFrame(true);
        converter.convertImage(camImg, resultImg);
        cv::Mat img(resultImg->imageFormat.height, resultImg->imageFormat.width, CV_8UC3, resultImg->data);
        //cap >> img;
        if(img.empty())
            break;
        if(recording)
        {
            if(videoEncoder == NULL)
            {
                videoEncoder = new VideoEncoder();
                videoEncoder->open((record_folder+"/camVideo.h264").c_str(), cv::Size(1280,720), 30);
                timestampFile = fopen((record_folder+"/camVideoTimestamp.txt").c_str(), "w");
            }
            auto current_time = std::chrono::system_clock::now();
            auto seconds_since_epoch = std::chrono::duration<double>(current_time.time_since_epoch()).count();
            unsigned long long timestampMs = (unsigned long long)(seconds_since_epoch*1000);
            fprintf(timestampFile, "%llu\n", timestampMs);
            videoEncoder->write(img);
        } else if(videoEncoder != NULL) {
            videoEncoder->release();
            fclose(timestampFile);
            videoEncoder = NULL;
            timestampFile = NULL;
        }
        videoInput->setImg(img);
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
    QuestVideoSourceSocket videoSrc;
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
        cv::Mat img = questVideoMngr->getMostRecentImg();
        if(!img.empty())
        {
            questInput->setImg(img);
        }
    }
    questVideoMngr->detachSource();
    videoSrc.Disconnect();
}

void MainWindow::postProcessThreadFunc()
{
    std::string timestampFilename = (record_folder+"/questVidTimestamp.txt");
    FILE *timestampFile = fopen(timestampFilename.c_str(), "r");
    unsigned long long firstTimestamp = 0, lastTimestamp = 0;
    printf("load timestamps\n");
    if(timestampFile)
    {
        fscanf(timestampFile, "%llu\n", &firstTimestamp);
        while(!feof(timestampFile))
            fscanf(timestampFile, "%llu\n", &lastTimestamp);
        fclose(timestampFile);
    }
    if(lastTimestamp > firstTimestamp)
    {
        printf("start processing\n");
        VideoEncoder *videoEncoder = NULL;
        timestampFile = NULL;
        QuestVideoMngr mngr;
        QuestVideoSourceFile videoSrc;
        videoSrc.open((record_folder+"/questVid.questMRVideo").c_str());
        mngr.attachSource(&videoSrc);
        mngr.setRecordedTimestampSource(timestampFilename.c_str());
        while(true)
        {
            if(!videoSrc.isValid())
                break;
            mngr.VideoTickImpl();
            unsigned long long timestamp;
            cv::Mat img = mngr.getMostRecentImg(&timestamp);
            printf("process %llu / %llu\n", timestamp - firstTimestamp, lastTimestamp - firstTimestamp);
            if(!img.empty())
            {
                img = img(cv::Rect(0,0,img.cols/2,img.rows)).clone();
                if(videoEncoder == NULL)
                {
                    videoEncoder = new VideoEncoder();
                    videoEncoder->open((record_folder+"/questVid_processed.h264").c_str(), img.size(), 30);
                    timestampFile = fopen((record_folder+"/questVid_processedTimestamp.txt").c_str(), "w");
                }
                postProcessVal = ((double)(timestamp - firstTimestamp)) / (lastTimestamp - firstTimestamp);
                fprintf(timestampFile, "%llu\n", timestamp);
                videoEncoder->write(img);
            }
        }
        if(videoEncoder != NULL)
        {
            videoEncoder->release();
            fclose(timestampFile);
        }
    }
}

void MainWindow::setConnectToQuestPage()
{
    currentPageName = "connectToQuest";
    clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(300, 0, 300, 200);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Enter the ip address of the quest...");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    QHBoxLayout *ipAddressLayout = new QHBoxLayout();
    ipAddressLayout->setContentsMargins(10,50,10,50);
    QLabel *ipAddressLabel = new QLabel;
    ipAddressLabel->setText("ip address: ");
    ipAddressField = new QLineEdit();
    ipAddressField->setFixedWidth(200);
    ipAddressField->setText("192.168.10.105");
    ipAddressLayout->addWidget(ipAddressLabel);
    ipAddressLayout->addWidget(ipAddressField);

    connectButton = new QPushButton("connect");

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(ipAddressLayout);
    layout->addWidget(connectButton,Qt::AlignCenter);

    connect(connectButton, &QPushButton::released, this, &MainWindow::onClickConnectToQuestButton);

    mainWidget->setLayout(layout);
}

void MainWindow::setCameraSelectPage()
{
    currentPageName = "cameraSelect";
    clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 10);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Select the type of camera...");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    hlayout = new QHBoxLayout();
    layout->setContentsMargins(50, 50, 50, 200);
    CameraMngr *cameraMngr = CameraMngr::getInstance();
    for(size_t i = 0; i < cameraMngr->listCameraEnumAndFactory.size(); i++)
    {
        std::string type = "&"+cameraMngr->listCameraEnumAndFactory[i].enumerator->cameraType;
        QRadioButton *cameraButton = new QRadioButton(tr(type.c_str()));
        hlayout->addWidget(cameraButton);
        connect(cameraButton,&QRadioButton::clicked,[=](){onClickCameraButton(i);});
    }

    cameraParamLayout = new QVBoxLayout();


    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(hlayout);
    layout->addLayout(cameraParamLayout);

    mainWidget->setLayout(layout);
}

void MainWindow::refreshCameraComboBox(CameraEnumerator *camEnumerator)
{
    listCameraCombo->clear();
    camEnumerator->detectCameras();

    listCameraIds.clear();
    for (size_t i = 0; i < camEnumerator->count(); i++) {
        listCameraIds.push_back(camEnumerator->getCameraId(i));
        listCameraCombo->addItem(QString(camEnumerator->getCameraName(i).c_str()));
    }

}

void MainWindow::setCameraParamBox(CameraEnumerator *camEnumerator)
{
    clearLayout(cameraParamLayout);

    QHBoxLayout *cameraSelectLayout = new QHBoxLayout();
    cameraSelectLayout->setContentsMargins(10,50,10,50);

    if(camEnumerator->listRequiredField.size() > 0)
    {
        QHBoxLayout *hLayout1 = new QHBoxLayout();
        QVBoxLayout *vLayout1 = new QVBoxLayout();
        for(size_t i = 0; i < camEnumerator->listRequiredField.size(); i++)
        {
            CameraEnumeratorField& field = camEnumerator->listRequiredField[i];
            QHBoxLayout *hLayout2 = new QHBoxLayout();
            QLabel *textLabel = new QLabel;
            textLabel->setText((field.text+": ").c_str());
            hLayout2->addWidget(textLabel);
            if(field.type == "text")
            {
                QLineEdit *lineEdit = new QLineEdit();
                lineEdit->setText(field.value.c_str());
                lineEdit->setContentsMargins(10,30,10,30);

                field.extra_param = lineEdit;

                hLayout2->addWidget(lineEdit);
            }
            vLayout1->addLayout(hLayout2);
        }
        hLayout1->addLayout(vLayout1);
        QPushButton *refreshButton = new QPushButton("refresh");
        connect(refreshButton,&QPushButton::clicked,[=]()
        {
            for(size_t i = 0; i < camEnumerator->listRequiredField.size(); i++)
            {
                CameraEnumeratorField& field = camEnumerator->listRequiredField[i];
                if(field.type == "text")
                    field.value = ((QLineEdit*)field.extra_param)->text().toStdString();
            }
            refreshCameraComboBox(camEnumerator);
        });
        hLayout1->addWidget(refreshButton);
        cameraParamLayout->addLayout(hLayout1);
    }

    QLabel *cameraLabel = new QLabel;
    cameraLabel->setText("camera: ");

    listCameraCombo = new QComboBox;

    refreshCameraComboBox(camEnumerator);

    QPushButton *selectButton = new QPushButton("select");

    cameraSelectLayout->addWidget(cameraLabel);
    cameraSelectLayout->addWidget(listCameraCombo);


    cameraParamLayout->addLayout(cameraSelectLayout);

    cameraParamLayout->addWidget(selectButton);

    connect(selectButton,SIGNAL(clicked()),this,SLOT(onClickSelectCameraButton()));
}

void MainWindow::setCalibrationOptionPage()
{
    currentPageName = "calibrationOption";
    clearMainWidget();

     QVBoxLayout *layout = new QVBoxLayout();

     QPushButton *checkCurrentCalibButton = new QPushButton("check current calibration");
     layout->addWidget(checkCurrentCalibButton);

     QPushButton *loadCalibrationFileButton = new QPushButton("load calibration file");
     layout->addWidget(loadCalibrationFileButton);

     QPushButton *saveCurrentCalibrationFileButton = new QPushButton("save calibration to file");
     layout->addWidget(saveCurrentCalibrationFileButton);

     QPushButton *calibrateWithChessboardButton = new QPushButton("calibrate camera with chessboard");
     layout->addWidget(calibrateWithChessboardButton);

     QPushButton *recalibrateCameraPoseButton = new QPushButton("recalibrate camera pose");
     layout->addWidget(recalibrateCameraPoseButton);

     QPushButton *recordMixedRealityButton = new QPushButton("record mixed reality");
     layout->addWidget(recordMixedRealityButton);

     mainWidget->setLayout(layout);

     connect(checkCurrentCalibButton,SIGNAL(clicked()),this,SLOT(onClickCheckCurrentCalibrationButton()));
     connect(loadCalibrationFileButton,SIGNAL(clicked()),this,SLOT(onClickLoadCalibrationFileButton()));
     connect(recalibrateCameraPoseButton,SIGNAL(clicked()),this,SLOT(onClickRecalibratePoseButton()));
     connect(recordMixedRealityButton,SIGNAL(clicked()),this,SLOT(onClickRecordMixedRealityButton()));
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

void MainWindow::setRecordMixedRealityPage()
{
    currentPageName = "recordMixedReality";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("record mixed reality: ");

    startRecordingButton = new QPushButton("start recording");
    startRecordingButton->setMaximumWidth(300);

    QTabWidget *tabWidget = new QTabWidget;

    camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    camPreviewWidget->setImg(cv::Mat());

    questPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    questPreviewWidget->setImg(cv::Mat());

    tabWidget->addTab(camPreviewWidget, tr("Camera"));
    tabWidget->addTab(questPreviewWidget, tr("Quest"));

    layout->addWidget(calibrationLabel);
    layout->addWidget(startRecordingButton);
    layout->addWidget(tabWidget);

    mainWidget->setLayout(layout);

    connect(startRecordingButton,SIGNAL(clicked()),this,SLOT(onClickStartRecordingButton()));
}

void MainWindow::setPostProcessingPage()
{
    currentPageName = "postProcessing";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(100,100,100,300);

    QLabel *postProcessingLabel = new QLabel;
    postProcessingLabel->setText("post processing: ");

    progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setMaximumWidth(500);


    layout->addWidget(postProcessingLabel);
    layout->addWidget(progressBar);
    mainWidget->setLayout(layout);

    segmentationProcess = new QProcess(this);
    segmentationProcess->setEnvironment( QProcess::systemEnvironment() );
    segmentationProcess->setProcessChannelMode( QProcess::MergedChannels );
    QString inputVideo = QString::fromStdString(record_folder + "/camVideo.h264");
    QString outputVideo = QString::fromStdString(record_folder + "/camVideo_mask.mp4");
    segmentationProcess->start("python3", QStringList() << "humanSegmentation.py" << inputVideo << outputVideo);
    //process->waitForStarted();
    connect (segmentationProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
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
        /*if(questConnectionStatus == QuestConnectionStatus::ConnectionFailed)
        {
            QMessageBox msgBox;
            msgBox.setText("Can not connect to the quest...");
            msgBox.exec();
            questCommunicatorThread->join();
            questCommunicatorThread = NULL;
            questConnectionStatus = QuestConnectionStatus::NotConnected;
            connectButton->setText("connect");
            connectButton->setEnabled(true);
        } else if(questConnectionStatus == QuestConnectionStatus::Connected) {*/
            setCameraSelectPage();
        //}
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

void MainWindow::onClickConnectToQuestButton()
{
    QString ipaddress = ipAddressField->text();
    connectButton->setText("connecting...");
    connectButton->setEnabled(false);
    questConnectionStatus = QuestConnectionStatus::Connecting;

    questCommunicatorThread = new std::thread([&]()
        {
            questCommunicatorThreadFunc();
        }
    );
}

void MainWindow::onClickCameraButton(int i)
{
    currentCameraEnumId = i;
    CameraMngr *cameraMngr = CameraMngr::getInstance();
    CameraEnumerator *camEnumerator = cameraMngr->listCameraEnumAndFactory[currentCameraEnumId].enumerator;
    setCameraParamBox(camEnumerator);
}

void MainWindow::onClickSelectCameraButton()
{
    cameraId = listCameraIds[listCameraCombo->currentIndex()];

    videoInput->videoThread = new std::thread([&]()
        {
            videoThreadFunc(cameraId);
        }
    );
    setCalibrationOptionPage();
    //setCameraPreview();
}

void MainWindow::onClickLoadCalibrationFileButton()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("calib file (*.xml *.json)"));
    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        if(filenames.size() == 1)
        {
            libQuestMR::QuestCalibData calibData;
            calibData.loadXMLFile(filenames[0].toStdString().c_str());

            printf("%s\n", calibData.generateXMLString().c_str());
        }
    }
}

void MainWindow::onClickCheckCurrentCalibrationButton()
{
    setCheckCalibrationPage();
}

void MainWindow::onClickRecalibratePoseButton()
{
    setRecalibratePosePage();
}


void MainWindow::onClickRecordMixedRealityButton()
{
    questInput->videoThread = new std::thread([&]()
        {
            questThreadFunc();
        }
    );
    setRecordMixedRealityPage();
}

void MainWindow::onClickStartRecordingButton()
{
    if(!recording)
    {
        record_folder = "output/"+getCurrentDateTimeStr();
        QDir().mkdir(record_folder.c_str());
        recording = true;
        startRecordingButton->setText("stop recording");
    } else {
        recording = false;
        setPostProcessingPage();
        delete videoInput;
        delete questInput;
        camPreviewWidget = NULL;
        questPreviewWidget = NULL;
        postProcessingThread = new std::thread([&]()
        {
            postProcessThreadFunc();
        });
    }
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
