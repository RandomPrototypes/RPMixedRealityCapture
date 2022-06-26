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
    void setEstimateIntrinsic(bool val);
public slots:
    void onClickCaptureFrameButton();
    void onClickAnnotateCalibFrameButton();
    void onClickPreviewWidget();
    void onClickBackToMenuButton();
private:

    MainWindow *win;
    QHBoxLayout *hlayout;
    QPushButton *nextButton;
    bool estimateIntrinsic;

    enum class CalibState
    {
        capture,
        annotate,
        waitingCalibrationUpload
    };

    CalibState state;
    uint32_t currentTriggerCount = 0;
};

#endif // CALIBRATECAMERAPOSEPAGE_H
