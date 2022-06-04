#ifndef POSTPROCESSINGOPTIONPAGE_H
#define POSTPROCESSINGOPTIONPAGE_H

#include "mainwindow.h"
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/VideoEncoder.h>
#include <QCheckBox>

class PostProcessingOptionPage : public QObject
{
    Q_OBJECT
public:
    PostProcessingOptionPage(MainWindow *win);

    void setPage(bool isLivePreview = false);
    void onTimer();
    void setQuestRecordingFilename(std::string filename);
public slots:
    void onClickQuestRecordingFileBrowseButton();
    void onClickCamRecordingFileBrowseButton();
    void onClickStartEncodingButton();
    void onSelectBackgroundSubtractorCombo(int);
    void onClickCamImgCheckbox();
    void onClickQuestImgCheckbox();
    void onClickMatteImgCheckbox();
    void onClickGreenBackgroundCheckbox();
    void onClickBlackBackgroundCheckbox();
    void onClickPlayButton();
    void onClickStopButton();
    void onClickSelectPlayAreaButton();
    void onClickPreviewWidget();
    void onClickSavePreviewSettingButton();
private:
    bool loadCameraTimestamps(std::string filename);
    void readCameraFrame(uint64_t timestamp);
    void encodingThreadFunc();
    void updateDurationLabel();
    void refreshBackgroundSubtractorOption();
    bool loadCameraRecordingFile();
    bool loadQuestRecordingFile();
    void updateRecordingFile();
    void updatePreviewImg();
    void updatePlayArea();

    enum class SelectShapeState
    {
        noSelection,
        selecting,
        selectionFinished,
    };

    enum class PostProcessingState
    {
        previewPlay,
        previewPause,
        encodingStarted,
        encodingPaused,
        encodingStopped,
        encodingFinished,
    };

    MainWindow *win;
    QLineEdit *questRecordingFileEdit;
    QLineEdit *camRecordingFileEdit;
    QComboBox *listBackgroundSubtractorCombo;
    QGridLayout *layout;
    QGridLayout *backgroundSubtractorOptionLayout;

    QToolButton *playButton;
    QLabel *durationLabel;
    QPushButton *startEncodingButton;
    QPushButton *selectPlayAreaButton;

    QCheckBox *camImgCheckbox;
    QCheckBox *questImgCheckbox;
    QCheckBox *matteImgCheckbox;
    QCheckBox *greenBackgroundCheckbox;
    QCheckBox *blackBackgroundCheckbox;
    cv::Mat currentFrameCam;
    cv::Mat currentFrameQuest;

    std::shared_ptr<libQuestMR::QuestVideoMngr> questVideoMngr;
    std::shared_ptr<libQuestMR::QuestVideoSourceFile> questVideoSrc;
    std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSubtractor;
    std::shared_ptr<cv::VideoCapture> capCameraVid;
    std::vector<uint64_t> listCameraTimestamp;
    int cameraFrameId, questFrameId;

    std::shared_ptr<RPCameraInterface::VideoEncoder> videoEncoder;
    std::string encodingFilename;
    std::thread *encodingThread;
    cv::Mat encodedFrame;
    std::mutex encodingMutex;
    cv::Size videoSize;

    std::vector<cv::Point2d> playAreaShape;
    cv::Rect playAreaROI;
    cv::Mat playAreaMask;

    volatile PostProcessingState state;
    volatile SelectShapeState selectShapeState;
    uint64_t startPlayTimestamp;

    bool isLivePreview;
};


#endif // POSTPROCESSINGPAGE_H
