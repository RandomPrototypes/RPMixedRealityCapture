#ifndef CAMERASELECTPAGE_H
#define CAMERASELECTPAGE_H

#include "mainwindow.h"

class CameraEnumerator;

class CameraSelectPage : public QObject
{
    Q_OBJECT
public:
    CameraSelectPage(MainWindow *win);

    void setPage();
    void refreshCameraComboBox(RPCameraInterface::CameraEnumerator *camEnumerator);
    void setCameraParamBox(RPCameraInterface::CameraEnumerator *camEnumerator);
public slots:
    void onClickSelectCameraButton();
    void onClickCameraButton(int i);

private:
    MainWindow *win;
    QComboBox *listCameraCombo;
    QVBoxLayout *cameraParamLayout;
    std::vector<std::string> listCameraIds;
};

#endif // CAMERASELECTPAGE_H
