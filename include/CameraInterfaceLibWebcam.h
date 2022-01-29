#ifndef CAMERAINTERFACELIBWEBCAM_H
#define CAMERAINTERFACELIBWEBCAM_H

#include "CameraInterface.h"
#include <sstream>
#include <libwebcam/webcam.h>

class CameraEnumeratorLibWebcam : public CameraEnumerator
{
public:
    CameraEnumeratorLibWebcam();
    virtual ~CameraEnumeratorLibWebcam();

    virtual bool detectCameras();
};

class CameraInterfaceLibWebcam : public CameraInterface
{
public:
    CameraInterfaceLibWebcam();
    ~CameraInterfaceLibWebcam();
    virtual bool open(std::string params);
    virtual bool close();
    virtual std::vector<ImageFormat> getAvailableFormats();
    virtual void selectFormat(int formatId);
    virtual void selectFormat(ImageFormat format);
    virtual std::shared_ptr<ImageData> getNewFrame(bool skipOldFrames);
    virtual std::string getErrorMsg();

    virtual bool startCapturing();
    virtual bool stopCapturing();
private:
    ImageType toImageType(const webcam::format &format) const;

    int deviceId;
    std::vector<ImageFormat> listFormats;
    std::vector<webcam::format*> listLibWebcamFormats;
    ImageFormat imageFormat;
    webcam::device *device;
    std::string errorMsg;
};

class CameraInterfaceFactoryLibWebcam : public CameraInterfaceFactory
{
public:
    CameraInterfaceFactoryLibWebcam();
    virtual ~CameraInterfaceFactoryLibWebcam();
    virtual CameraInterface *createInterface();
};

#endif // CAMERAINTERFACELIBWEBCAM_H
