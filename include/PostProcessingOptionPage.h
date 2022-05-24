#ifndef POSTPROCESSINGOPTIONPAGE_H
#define POSTPROCESSINGOPTIONPAGE_H

#include "mainwindow.h"
#include <libQuestMR/BackgroundSubtractor.h>
#include <QCheckBox>

class PostProcessingOptionPage : public QObject
{
    Q_OBJECT
public:
    PostProcessingOptionPage(MainWindow *win);

    void setPage();
    void onTimer();
    void refreshBackgroundSubtractorOption();
    void updateRecordingFile();
    void updatePreviewImg();
public slots:
    void onClickQuestRecordingFileBrowseButton();
    void onClickCamRecordingFileBrowseButton();
    void onSelectBackgroundSubtractorCombo(int);
    void onClickCamImgCheckbox();
    void onClickQuestImgCheckbox();
    void onClickMatteImgCheckbox();
    void onClickGreenBackgroundCheckbox();
    void onClickBlackBackgroundCheckbox();
    void onClickPlayButton();
    void onClickStopButton();
private:
    bool loadCameraTimestamps(std::string filename);
    void readCameraFrame(uint64_t timestamp);

    enum class PostProcessingState
    {
        previewPlay,
        previewPause,
    };

    MainWindow *win;
    QLineEdit *questRecordingFileEdit;
    QLineEdit *camRecordingFileEdit;
    QComboBox *listBackgroundSubtractorCombo;
    QGridLayout *layout;
    QGridLayout *backgroundSubtractorOptionLayout;

    QToolButton *playButton;
    QLabel *durationLabel;

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

    PostProcessingState state;
    uint64_t startPlayTimestamp;
};


#endif // POSTPROCESSINGPAGE_H
