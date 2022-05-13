#ifndef FIRSTMENUPAGE_H
#define FIRSTMENUPAGE_H

#include "mainwindow.h"


class FirstMenuPage : public QObject
{
    Q_OBJECT
public:
    FirstMenuPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickCalibrationButton();
    void onClickRecordingButton();
private:
    MainWindow *win;
};

#endif // FIRSTMENUPAGE_H
