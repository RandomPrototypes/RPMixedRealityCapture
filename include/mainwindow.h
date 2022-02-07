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
#include "VideoEncoder.h"
#include "QuestCommunicator.h"
#include "QuestVideoMngr.h"
#include "CameraInterface.h"
#include "Util.hpp"

class CalibrationFrame
{
public:
    cv::Mat img;
    libQuestMR::QuestFrameData frameData;
    cv::Point2d rightControllerImgPos;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setFrontPage();
    void setConnectToQuestPage();
    void setCameraSelectPage();
    void setCameraParamBox(RPCameraInterface::CameraEnumerator *camEnumerator);
    void setUSBCameraParamBox();
    void setCalibrationOptionPage();
    void setCheckCalibrationPage();
    void setRecalibratePosePage();
    void setRecordMixedRealityPage();
    void setPostProcessingPage();
    void clearMainWidget();
    void questCommunicatorThreadFunc();
    void videoThreadFunc(std::string cameraId);
    void questThreadFunc();
    void postProcessThreadFunc();
    bool calibratePose();
    void captureCalibFrame();
    void refreshCameraComboBox(RPCameraInterface::CameraEnumerator *camEnumerator);
private slots:
    void onClickStartButton();
    void onClickConnectToQuestButton();
    void onClickCameraButton(int i);
    void onClickSelectCameraButton();
    void onClickLoadCalibrationFileButton();
    void onClickCheckCurrentCalibrationButton();
    void onClickRecordMixedRealityButton();
    void onClickRecalibratePoseButton();
    void onClickStartRecordingButton();
    void onClickCaptureFrameButton();
    void onClickAnnotateCalibFrameButton();
    void onClickPreviewWidget();
    void setCameraPreview();
    void onTimer();
    void processOutput();
private:
    QWidget *mainWidget;
    QVBoxLayout *cameraParamLayout;
    QComboBox *listCameraCombo;
    QLineEdit *ipAddressField;
    QPushButton *connectButton;
    QPushButton *startRecordingButton;
    QProgressBar *progressBar;
    OpenCVWidget *camPreviewWidget;
    OpenCVWidget *questPreviewWidget;
    std::vector<std::string> listCameraIds;
    QTimer *timer;
    VideoInputMngr *videoInput;
    VideoInputMngr *questInput;
    std::string cameraId;

    QHBoxLayout *hlayout;

    QLabel *instructionLabel;

    libQuestMR::QuestVideoMngr *questVideoMngr;

    std::string currentPageName;

    std::string record_folder;

    int currentCameraEnumId = 0;

    double postProcessVal = 0;

    volatile bool recording;
    std::string recordedVideoFilename, recordedVideoTimestampFilename;

    QProcess *segmentationProcess = NULL;

    std::thread *questCommunicatorThread;
    std::thread *postProcessingThread = NULL;
    libQuestMR::QuestCommunicator questCom;
    libQuestMR::QuestCommunicatorThreadData *questComThreadData;

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
};

void clearLayout(QLayout *layout);

#endif // MAINWINDOW_H
