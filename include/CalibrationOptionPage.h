#ifndef CALIBRATIONOPTIONPAGE_H
#define CALIBRATIONOPTIONPAGE_H

#include "mainwindow.h"


class CalibrationOptionPage : public QObject
{
    Q_OBJECT
public:
    CalibrationOptionPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickCalibrateWithChessboardButton();
    void onClickLoadCalibrationFileButton();
    void onClickCheckCurrentCalibrationButton();
    void onClickRecalibratePoseButton();
    void onClickRecordMixedRealityButton();
private:
    MainWindow *win;
};

#endif // CALIBRATIONOPTIONPAGE_H
