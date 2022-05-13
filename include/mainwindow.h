#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QProgressBar>
#include <QProcess>
#include <thread>
#include <opencv2/opencv.hpp>
#include "VideoInputMngr.h"
#include "OpenCVWidget.h"
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestVideoMngr.h>
#include <RPCameraInterface/CameraInterface.h>
#include "Util.hpp"

class CalibrationFrame
{
public:
    cv::Mat img;
    uint64_t imgTimestamp;
    libQuestMR::QuestFrameData frameData;
    cv::Point2d rightControllerImgPos;
    std::vector<cv::Point2f> corners;
};

class FirstMenuPage;
class CalibrateWithChessboardPage;
class CalibrateCameraPosePage;
class CalibrationOptionPage;
class CameraSelectPage;
class CameraPreviewPage;
class CheckCalibrationPage;
class ConnectQuestPage;
class RecordMixedRealityPage;
class PostProcessingPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setFrontPage();
    void setCameraParamBox(RPCameraInterface::CameraEnumerator *camEnumerator);
    void setUSBCameraParamBox();
    void clearMainWidget();
    void questCommunicatorThreadFunc();
    void videoThreadFunc(std::string cameraId);
    void questThreadFunc();
    void postProcessThreadFunc();
    bool calibratePose();
    void capturePoseCalibFrame();
    void captureChessboardCalibFrame();
    void refreshCameraComboBox(RPCameraInterface::CameraEnumerator *camEnumerator);

    void captureChessboardFrame();
    void detectChessboardsAndCalibrate();

private slots:
    void onClickStartButton();
    void onClickPreviewWidget();
    void onTimer();
protected:
    friend class FirstMenuPage;
    friend class CalibrateWithChessboardPage;
    friend class CalibrateCameraPosePage;
    friend class CalibrationOptionPage;
    friend class CameraSelectPage;
    friend class CameraPreviewPage;
    friend class CheckCalibrationPage;
    friend class ConnectQuestPage;
    friend class RecordMixedRealityPage;
    friend class PostProcessingPage;
    QWidget *mainWidget;
    OpenCVWidget *camPreviewWidget;
    OpenCVWidget *questPreviewWidget;
    QTimer *timer;
    VideoInputMngr *videoInput;
    VideoInputMngr *questInput;
    std::string cameraId;

    std::string questIpAddress;

    //QHBoxLayout *hlayout;

    QLabel *instructionLabel;

    std::shared_ptr<libQuestMR::QuestVideoMngr> questVideoMngr;

    std::string record_folder;

    int currentCameraEnumId = 0;
    std::vector<std::shared_ptr<RPCameraInterface::CameraEnumerator> > listCameraEnumerator;


    volatile bool recording, recording_finished;
    std::string recordedVideoFilename, recordedVideoTimestampFilename;

    QProcess *segmentationProcess = NULL;

    std::thread *questCommunicatorThread;
    std::thread *postProcessingThread = NULL;
    std::shared_ptr<libQuestMR::QuestCommunicator> questCom;
    std::shared_ptr<libQuestMR::QuestCommunicatorThreadData> questComThreadData;

    std::vector<CalibrationFrame> listCalibrationFrames;
    int currentCalibrationFrame;
    libQuestMR::QuestFrameData lastFrameData;

    enum class QuestConnectionStatus {
        NotConnected,
        Connecting,
        Connected,
        ConnectionFailed,
    };
    QuestConnectionStatus questConnectionStatus = QuestConnectionStatus::NotConnected;

    enum class PageName
    {
        frontPage,
        firstMenuPage,
        recalibratePose,
        calibrateWithChessboard,
        checkCalibration,
        connectToQuest,
        cameraPreview,
        cameraSelect,
        postProcessing,
        recordMixedReality,
        calibrationOption
    };
    PageName currentPageName;

    bool isCalibrationSection;

    FirstMenuPage *firstMenuPage;
    CalibrateWithChessboardPage *calibrateWithChessboardPage;
    CalibrateCameraPosePage *calibrateCameraPosePage;
    CalibrationOptionPage *calibrationOptionPage;
    CameraSelectPage *cameraSelectPage;
    CameraPreviewPage *cameraPreviewPage;
    CheckCalibrationPage *checkCalibrationPage;
    ConnectQuestPage *connectQuestPage;
    RecordMixedRealityPage *recordMixedRealityPage;
    PostProcessingPage *postProcessingPage;
};

void clearLayout(QLayout *layout);

#endif // MAINWINDOW_H
