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
    void refreshCameraResolutionComboBox();
    void refreshCameraEncodingComboBox();
    void setCameraParamBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator);
    int findCameraFormatId(int width, int height, RPCameraInterface::ImageType encoding);
public slots:
    void onClickSelectCameraButton();
    void onClickCameraButton(int i);
    void onSelectCameraCombo(int i);
    void onSelectResolutionCombo(int i);
    void onSelectEncodingCombo(int i);
    void onClickBackToMenuButton();

private:
    MainWindow *win;
    QComboBox *listCameraCombo;
    QComboBox *listCameraResolutionCombo;
    QComboBox *listCameraEncodingCombo;
    QVBoxLayout *cameraParamLayout;
    std::vector<std::string> listCameraIds;
    std::vector<RPCameraInterface::ImageFormat> listResolution;
    std::vector<RPCameraInterface::ImageType> listEncoding;
    int currentCameraId, currentCameraResolutionId, currentCameraEncodingId;
    bool ignoreIndexChangeSignal;
};

#endif // CAMERASELECTPAGE_H
