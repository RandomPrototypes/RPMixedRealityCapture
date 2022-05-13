#ifndef CALIBRATEWITHCHESSBOARDPAGE_H
#define CALIBRATEWITHCHESSBOARDPAGE_H

#include "mainwindow.h"
#include <QSpinBox>

class CalibrateWithChessboardPage : public QObject
{
    Q_OBJECT
public:
    CalibrateWithChessboardPage(MainWindow *win);

    void setPage();
    void captureFrame();
    void detectChessboardsAndCalibrate();
    void onTimer();

public slots:
    void onClickStartCalibration();
    void onClickCaptureFrame();
    void onClickStopCalibration();
    void onClickPreviewWidget();

private:

    MainWindow *win;
    QSpinBox *rowsSpinBox;
    QSpinBox *columnsSpinBox;
    QSpinBox *checkerWidthSpinBox;
    cv::Size boardSize;
    float checkerWidth;

    enum class CalibState
    {
        capture,
        calibrate,
        verify
    };

    CalibState state;
    uint32_t currentTriggerCount = 0;
};

#endif // CALIBRATEWITHCHESSBOARDPAGE_H
