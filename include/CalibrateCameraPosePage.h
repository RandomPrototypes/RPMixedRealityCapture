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
        captureCamOrig,
        capture,
        annotate,
        waitingCalibrationUpload
    };

    CalibState state;
    cv::Point3d camOrig;
    bool capturedCamOrig;
    uint32_t currentTriggerCount = 0;
};

#endif // CALIBRATECAMERAPOSEPAGE_H
