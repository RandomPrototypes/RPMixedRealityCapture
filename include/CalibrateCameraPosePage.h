#ifndef CALIBRATECAMERAPOSEPAGE_H
#define CALIBRATECAMERAPOSEPAGE_H

#include "mainwindow.h"

class CalibrateCameraPosePage : public QObject
{
    Q_OBJECT
public:
    CalibrateCameraPosePage(MainWindow *win);

    void setPage();
    void onTimer();
    void capturePoseCalibFrame();
    bool calibratePose();
public slots:
    void onClickCaptureFrameButton();
    void onClickAnnotateCalibFrameButton();
    void onClickPreviewWidget();
private:

    MainWindow *win;
    QHBoxLayout *hlayout;

    enum class CalibState
    {
        capture,
        annotate
    };

    CalibState state;
    uint32_t currentTriggerCount = 0;
};

#endif // CALIBRATECAMERAPOSEPAGE_H
