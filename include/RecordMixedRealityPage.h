#ifndef RECORDMIXEDREALITYPAGE_H
#define RECORDMIXEDREALITYPAGE_H

#include "mainwindow.h"

class RecordMixedRealityPage : public QObject
{
    Q_OBJECT
public:
    RecordMixedRealityPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickStartRecordingButton();
    void onClickBackToMenuButton();
private:
    MainWindow *win;
    QPushButton *startRecordingButton;
    OpenCVWidget *mixedRealityPreviewWidget;

    cv::Mat currentQuestImg, currentCamImg;
};

#endif // RECORDMIXEDREALITYPAGE_H
