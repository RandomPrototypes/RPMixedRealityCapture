#include "CameraEnumeratorQt.h"
#include <QCameraDevice>
#include <QMediaDevices>

CameraEnumeratorQt::CameraEnumeratorQt()
{
    cameraType = "USB camera";
}

CameraEnumeratorQt::~CameraEnumeratorQt()
{
}

bool CameraEnumeratorQt::detectCameras()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {
        CameraInfo camInfo;
        camInfo.id = QString(cameraDevice.id()).toStdString();
        camInfo.description = QString(cameraDevice.description()).toStdString();
        listCameras.push_back(camInfo);
    }
    return true;
}
