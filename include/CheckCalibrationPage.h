#ifndef CHECKCALIBRATIONPAGE_H
#define CHECKCALIBRATIONPAGE_H

#include "mainwindow.h"


class CheckCalibrationPage : public QObject
{
    Q_OBJECT
public:
    CheckCalibrationPage(MainWindow *win);

    void setPage();
    void onTimer();

private:
    MainWindow *win;
};

#endif // CHECKCALIBRATIONPAGE_H
