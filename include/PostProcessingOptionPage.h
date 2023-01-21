#ifndef POSTPROCESSINGOPTIONPAGE_H
#define POSTPROCESSINGOPTIONPAGE_H

#include "mainwindow.h"
#include <libQuestMR/BackgroundSubtractor.h>
#include <RPCameraInterface/VideoEncoder.h>
#include <QCheckBox>
#include <QSlider>
#include <QProxyStyle>

class ClickSliderStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    virtual int styleHint(StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0) const{
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
        {
            return Qt::LeftButton;
        }
        else
        {
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
  }
};

class ClickSlider : public QSlider
{
    Q_OBJECT
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
public:
    explicit ClickSlider(Qt::Orientation orientation, QWidget *parent = 0);

signals:
    void clicked(int value) const;
};

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
    void onSelectCamBackgroundSubtractorCombo(int);
    void onClickQuestShowForegroundCheckbox();
    void onSelectQuestBackgroundSubtractorCombo(int);
    void onClickCamImgCheckbox();
    void onClickQuestImgCheckbox();
    void onClickMatteImgCheckbox();
    void onClickGreenBackgroundCheckbox();
    void onClickBlackBackgroundCheckbox();
    void onClickPlayButton();
    void onClickStopButton();
    void onClickStartTrimButton();
    void onClickEndTrimButton();
    void onClickSelectPlayAreaButton();
    void onClickPreviewWidget();
    void onClickSavePreviewSettingButton();
    void onClickBackToMenuButton();
    void onVideoSliderMoved(int pos);
private:
    bool loadCameraTimestamps(std::string filename);
    void readCameraFrame(uint64_t timestamp);
    void encodingThreadFunc();
    void updateDurationLabel();
    void refreshBackgroundSubtractorOption(bool questBackground);
    bool loadCameraRecordingFile();
    bool loadQuestRecordingFile();
    bool updateRecordingFile(bool resetTrimPos);
    void updatePreviewImg();
    void updatePlayArea();
    int getClosestCameraTimestamp(uint64_t timestamp);
    void seekCameraTo(int frame_id);
    MixedRealityCompositorConfig& getCompositorConfig();

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
    QComboBox *listCamBackgroundSubtractorCombo;
    QComboBox *listQuestBackgroundSubtractorCombo;
    QGridLayout *layout;
    QGridLayout *camBackgroundSubtractorOptionLayout;
    QGridLayout *questBackgroundSubtractorOptionLayout;
    QCheckBox *questShowForegroundCheckBox;

    QSlider *videoSlider;
    QToolButton *playButton;
    QLabel *durationLabel;
    QPushButton *startEncodingButton;
    QPushButton *selectPlayAreaButton;
    QPushButton *startTrimButton;
    QPushButton *endTrimButton;

    QCheckBox *camImgCheckbox;
    QCheckBox *questImgCheckbox;
    QCheckBox *matteImgCheckbox;
    QCheckBox *greenBackgroundCheckbox;
    QCheckBox *blackBackgroundCheckbox;
    cv::Mat currentFrameCam;
    cv::Mat currentFrameQuest;

    std::shared_ptr<libQuestMR::QuestVideoMngr> questVideoMngr;
    std::shared_ptr<libQuestMR::QuestVideoSourceFile> questVideoSrc;
    std::shared_ptr<cv::VideoCapture> capCameraVid;
    std::vector<uint64_t> listCameraTimestamp;
    int cameraFrameId, questFrameId;

    std::shared_ptr<RPCameraInterface::VideoEncoder> videoEncoder;
    std::string encodingFilename;
    std::thread *encodingThread;
    cv::Mat encodedFrame;
    std::mutex encodingMutex;

    std::vector<cv::Point2d> playAreaShape;

    volatile PostProcessingState state;
    volatile SelectShapeState selectShapeState;
    uint64_t startPlayTimestamp;

    bool isLivePreview;
    bool useGreenBackground, useBlackBackground, useQuestImg, useCamImg, useMatteImg;
    int seekToTime;

    int startTrimFrame, endTrimFrame;
};


#endif // POSTPROCESSINGPAGE_H
