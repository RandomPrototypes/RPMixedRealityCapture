#ifndef CAMERASELECTPAGE_H
#define CAMERASELECTPAGE_H

#include "mainwindow.h"
#include <RPCameraInterface/CameraInterface.h>

class CameraSelectPage : public QObject
{
    Q_OBJECT
public:
    CameraSelectPage(MainWindow *win);

    void setPage();
    void refreshCameraComboBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator);
    void refreshCameraFormatComboBox();
    void setCameraParamBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator);
public slots:
    void onClickSelectCameraButton();
    void onClickCameraButton(int i);
    void onSelectCameraCombo(int i);

private:
    MainWindow *win;
    QComboBox *listCameraCombo;
    QComboBox *listCameraFormatCombo;
    QVBoxLayout *cameraParamLayout;
    std::vector<std::string> listCameraIds;
};

#endif // CAMERASELECTPAGE_H
