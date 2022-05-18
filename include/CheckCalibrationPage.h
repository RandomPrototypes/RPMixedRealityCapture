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

public slots:
    void onClickBackToMenuButton();

private:
    MainWindow *win;
    QHBoxLayout *hlayout;
};

#endif // CHECKCALIBRATIONPAGE_H
