#include "CameraInterfaceLibWebcam.h"
#include <libwebcam/webcam.h>
#include <QDebug>

CameraEnumeratorLibWebcam::CameraEnumeratorLibWebcam()
{
    cameraType = "USB camera";
}

CameraEnumeratorLibWebcam::~CameraEnumeratorLibWebcam()
{
}

bool CameraEnumeratorLibWebcam::detectCameras()
{
    const webcam::device_info_enumeration & enumeration = webcam::enumerator::enumerate();
    const size_t count = enumeration.count();

    qDebug() << count;

    for (size_t device_index = 0; device_index < count; device_index++){

        const webcam::device_info & device_info = enumeration.get(device_index);

        //print name
        qDebug()<<"Webcam no. "<<(device_index+1)<<":";
        const webcam::model_info & model_info = device_info.get_model_info();
        qDebug()<<"name: "<< model_info.get_name().c_str();
        qDebug()<<"friendly_name: "<< model_info.get_friendly_name().c_str();

        //print vendor info
        const webcam::vendor_info & vendor_info = model_info.get_vendor_info();
        qDebug()<<"vendor: "<<vendor_info.get_name().c_str();

        //print image details
        const webcam::video_info_enumeration & video_info_enumeration = device_info.get_video_info_enumeration();
        size_t video_count = video_info_enumeration.count();
        for (size_t video_index = 0; video_index < video_count; video_index++){
            const webcam::video_info & video_info = video_info_enumeration.get(video_index);

            qDebug() << "----------------------------";
            qDebug() << "\tformat: " << video_info.get_format().get_name().c_str();
            const webcam::resolution & resolution = video_info.get_resolution();
            qDebug() << "\twidth: " << resolution.get_width() << ", height: " << resolution.get_height();
        }
    }

    listCameras.clear();
    int lastId = -1;
    for(size_t i = 0; i < count; i++)
    {
        const webcam::device_info & device_info = enumeration.get(i);
        const webcam::video_info_enumeration & video_info_enumeration = device_info.get_video_info_enumeration();

        CameraInfo cam;
        auto modelInfo = device_info.get_model_info();
        int id = modelInfo.get_id();
        cam.id = std::to_string(i);
        cam.name = modelInfo.get_name();
        cam.description = modelInfo.get_desc();

        if(id == lastId)
            continue;
        lastId = id;

        listCameras.push_back(cam);
    }
    return true;
}

CameraInterfaceLibWebcam::CameraInterfaceLibWebcam()
{
    listLibWebcamFormats = std::vector<webcam::format*>(static_cast<int>(ImageType::NB_FORMAT_TYPE), NULL);
    device = NULL;
}

CameraInterfaceLibWebcam::~CameraInterfaceLibWebcam()
{
    for(webcam::format* format : listLibWebcamFormats)
        if(format != NULL)
            delete format;
}

ImageType CameraInterfaceLibWebcam::toImageType(const webcam::format& format) const
{
    if(format.get_name() == "MJPG")
        return ImageType::MJPG;
    else if(format.get_name() == "YUYV")
        return ImageType::YUYV422;
    return ImageType::UNKNOWN;
}

bool CameraInterfaceLibWebcam::open(std::string params)
{
    try {
        int enumId = std::stoi(params);

        const webcam::device_info_enumeration & enumeration = webcam::enumerator::enumerate();
        const webcam::device_info & device_info = enumeration.get(enumId);
        deviceId = device_info.get_model_info().get_id();
        qDebug() << "device: " << deviceId;
        const webcam::video_info_enumeration & video_info_enumeration = device_info.get_video_info_enumeration();
        size_t video_count = video_info_enumeration.count();
        for (size_t video_index = 0; video_index < video_count; video_index++){
            const webcam::video_info & video_info = video_info_enumeration.get(video_index);

            ImageFormat imgFormat;

            imgFormat.type = toImageType(video_info.get_format());
            if(imgFormat.type == ImageType::UNKNOWN)
                continue;

            int typeIndex = static_cast<int>(imgFormat.type);
            if(listLibWebcamFormats[typeIndex] == NULL)
                listLibWebcamFormats[typeIndex] = video_info.get_format().clone();

            const webcam::resolution& resolution = video_info.get_resolution();
            imgFormat.width = resolution.get_width();
            imgFormat.height = resolution.get_height();
            qDebug() << imgFormat.toString().c_str();
            listFormats.push_back(imgFormat);
        }
    } catch (std::invalid_argument const& ex) {
        errorMsg = std::string("can not convert to number: ")+params;
        return false;
    } catch(const webcam::webcam_exception & exc_) {
        errorMsg = std::string("open : ")+exc_.what();
        return false;
    }
    return true;
}
bool CameraInterfaceLibWebcam::close()
{
    return true;
}
std::vector<ImageFormat> CameraInterfaceLibWebcam::getAvailableFormats()
{
    return listFormats;
}
void CameraInterfaceLibWebcam::selectFormat(int formatId)
{
    imageFormat = listFormats[formatId];
}
void CameraInterfaceLibWebcam::selectFormat(ImageFormat format)
{
    imageFormat = format;
}
std::shared_ptr<ImageData> CameraInterfaceLibWebcam::getNewFrame(bool skipOldFrames)
{
    if(device != NULL)
    {
        webcam::image* image = device->read();
        const webcam::video_settings& videoSettings = image->get_video_settings();
        std::shared_ptr<ImageData> data = std::make_shared<ImageData>();
        data->releaseDataWhenDestroy = false;
        data->imageFormat.type = toImageType(videoSettings.get_format());
        data->imageFormat.width = videoSettings.get_width();
        data->imageFormat.height = videoSettings.get_height();
        data->data = const_cast<unsigned char*>(image->get_data());
        data->dataSize = image->get_data_lenght();
        return data;
    }
    return std::shared_ptr<ImageData>();
}
std::string CameraInterfaceLibWebcam::getErrorMsg()
{
    return errorMsg;
}

bool CameraInterfaceLibWebcam::startCapturing()
{
    try
    {
        int typeIndex = static_cast<int>(imageFormat.type);
        if(listLibWebcamFormats[typeIndex] == NULL)
        {
            errorMsg = std::string("startCapturing : unknown format");
            return false;
        }
        webcam::video_settings video_settings;
        video_settings.set_format(*listLibWebcamFormats[typeIndex]);
        video_settings.set_resolution(webcam::resolution(imageFormat.width, imageFormat.height));
        video_settings.set_fps(30);
        device = new webcam::device(deviceId, video_settings);
        device->open();
        return true;
    } catch(const webcam::webcam_exception & exc_) {
        errorMsg = std::string("startCapturing : ")+exc_.what();
        return false;
    }
    return true;
}
bool CameraInterfaceLibWebcam::stopCapturing()
{
    try
    {
        if(device != NULL)
        {
            device->close();
            delete device;
            device = NULL;
            return true;
        }
    } catch(const webcam::webcam_exception & exc_) {
        errorMsg = std::string("startCapturing : ")+exc_.what();
        return false;
    }

    return false;
}

CameraInterfaceFactoryLibWebcam::CameraInterfaceFactoryLibWebcam()
{
}

CameraInterfaceFactoryLibWebcam::~CameraInterfaceFactoryLibWebcam()
{
}

CameraInterface *CameraInterfaceFactoryLibWebcam::createInterface()
{
    return new CameraInterfaceLibWebcam();
}
