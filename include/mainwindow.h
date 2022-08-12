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
#include <libQuestMR/BackgroundSubtractor.h>
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
class CalibrateCameraQuestMenuPage;
class CalibrationOptionPage;
class CameraSelectPage;
class CameraPreviewPage;
class CheckCalibrationPage;
class ConnectQuestPage;
class RecordMixedRealityPage;
class PostProcessingOptionPage;
class PostProcessingPage;

class MixedRealityCompositorConfig
{
public:
    std::shared_ptr<libQuestMR::BackgroundSubtractor> camBackgroundSubtractor;//camera background subtractor
    std::shared_ptr<libQuestMR::BackgroundSubtractor> questBackgroundSubtractor;//quest background subtractor (to overlay VR on top of the environment)
    cv::Rect playAreaROI;
    cv::Mat playAreaMask;
    cv::Size videoSize;
    bool useQuestImg;
    bool useCamImg;
    bool useMatteImg;
    bool useGreenBackground;
    bool useBlackBackground;
    bool showForeground;
    int camDelayMs;
    cv::Point camTranslation;
    int camSubsampling;
    int questSubsampling;
    int camErosion;
    int questErosion;
};

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

    void startCamera();
    void startQuestRecorder();
    void startQuestCommunicator();
    void stopCamera();
    void stopQuestRecorder();
    void stopQuestCommunicator();
    cv::Mat alphaBlendingMat(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& alphaMask);
    cv::Mat composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, cv::Point camTranslation, const std::shared_ptr<libQuestMR::BackgroundSubtractor>& camBackgroundSubtractor, int camSubsampling, int camErosion, bool showForeground, const std::shared_ptr<libQuestMR::BackgroundSubtractor>& questBackgroundSubtractor, int questSubsampling, int questErosion, cv::Rect playAreaROI, cv::Mat playAreaMask, cv::Size videoSize, bool useQuestImg = true, bool useCamImg = true, bool useMatteImg = false, bool useGreenBackground = false, bool useBlackBackground = false);
    cv::Mat composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, const MixedRealityCompositorConfig& config);
    cv::Rect adjustROIWithSubsampling(cv::Rect ROI, int subsampling);

private slots:
    void onClickStartButton();
    void onClickPreviewWidget();
    void onTimer();
protected:
    void closeEvent(QCloseEvent *event);

    friend class FirstMenuPage;
    friend class CalibrateWithChessboardPage;
    friend class CalibrateCameraPosePage;
    friend class CalibrateCameraQuestMenuPage;
    friend class CalibrationOptionPage;
    friend class CameraSelectPage;
    friend class CameraPreviewPage;
    friend class CheckCalibrationPage;
    friend class ConnectQuestPage;
    friend class RecordMixedRealityPage;
    friend class PostProcessingOptionPage;
    friend class PostProcessingPage;
    QWidget *mainWidget;
    OpenCVWidget *camPreviewWidget;
    OpenCVWidget *questPreviewWidget;
    OpenCVWidget *postProcessPreviewWidget;
    QTimer *timer;
    VideoInputMngr *videoInput;
    VideoInputMngr *questInput;
    std::string cameraId;

    std::string questIpAddress = "192.168.10.106";

    //QHBoxLayout *hlayout;

    QLabel *instructionLabel;

    std::shared_ptr<libQuestMR::QuestVideoMngr> questVideoMngr;

    std::string record_folder, record_name;

    int currentCameraEnumId = 0, currentCameraFormatId = 0;
    std::vector<std::shared_ptr<RPCameraInterface::CameraEnumerator> > listCameraEnumerator;
    std::vector<RPCameraInterface::ImageFormat> listCameraFormats;


    volatile bool recording, recording_finished_camera, recording_finished_quest;
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
        calibrateCameraQuestMenu,
        calibrateWithChessboard,
        checkCalibration,
        connectToQuest,
        cameraPreview,
        cameraSelect,
        postProcessingOption,
        postProcessing,
        recordMixedReality,
        calibrationOption
    };
    PageName currentPageName;

    bool isCalibrationSection;

    FirstMenuPage *firstMenuPage;
    CalibrateWithChessboardPage *calibrateWithChessboardPage;
    CalibrateCameraPosePage *calibrateCameraPosePage;
    CalibrateCameraQuestMenuPage *calibrateCameraQuestMenuPage;
    CalibrationOptionPage *calibrationOptionPage;
    CameraSelectPage *cameraSelectPage;
    CameraPreviewPage *cameraPreviewPage;
    CheckCalibrationPage *checkCalibrationPage;
    ConnectQuestPage *connectQuestPage;
    RecordMixedRealityPage *recordMixedRealityPage;
    PostProcessingOptionPage *postProcessingOptionPage;
    PostProcessingPage *postProcessingPage;

    MixedRealityCompositorConfig previewCompositorConfig, recordingCompositorConfig;
};

void clearLayout(QLayout *layout);

inline bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

#endif // MAINWINDOW_H
