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
    void onClickImportExportCalibrationFileButton();
    void onClickCheckCurrentCalibrationButton();
    void onClickCalibrateCameraQuestButton();
    void onClickBackToMenuButton();
private:
    MainWindow *win;
};

#endif // CALIBRATIONOPTIONPAGE_H
