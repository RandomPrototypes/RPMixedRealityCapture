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
    void updateCamRecordingFile();
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
private:
    MainWindow *win;
    QLineEdit *questRecordingFileEdit;
    QLineEdit *camRecordingFileEdit;
    QComboBox *listBackgroundSubtractorCombo;
    QGridLayout *layout;
    QGridLayout *backgroundSubtractorOptionLayout;

    QCheckBox *camImgCheckbox;
    QCheckBox *questImgCheckbox;
    QCheckBox *matteImgCheckbox;
    QCheckBox *greenBackgroundCheckbox;
    QCheckBox *blackBackgroundCheckbox;
    cv::Mat firstFrameCam;

    std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSubtractor;
};


#endif // POSTPROCESSINGPAGE_H
