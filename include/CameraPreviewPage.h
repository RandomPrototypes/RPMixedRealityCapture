#ifndef CAMERAPREVIEWPAGE_H
#define CAMERAPREVIEWPAGE_H

#include "mainwindow.h"


class CameraPreviewPage : public QObject
{
    Q_OBJECT
public:
    CameraPreviewPage(MainWindow *win);

    void setPage();
    void onTimer();

private:
    MainWindow *win;
};

#endif // CAMERAPREVIEWPAGE_H
