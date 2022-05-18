#ifndef CALIBRATECAMERAQUESTMENUPAGE_H
#define CALIBRATECAMERAQUESTMENUPAGE_H

#include "mainwindow.h"


class CalibrateCameraQuestMenuPage : public QObject
{
    Q_OBJECT
public:
    CalibrateCameraQuestMenuPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickCalibrateCameraWithChessboardButton();
    void onClickCalibrateCameraPoseButton();
    void onClickCalibrateFullButton();
    void onClickBackToMenuButton();
private:
    MainWindow *win;
};

#endif // CALIBRATECAMERAQUESTMENUPAGE_H
