#include "CameraInterfaceAndroid.h"
#include <QDebug>

enum AndroidCameraCmd
{
    EXIT = 100,
    LIST_CAMERAS =200,
    TIMESTAMP = 300,
    START_RECORDING = 400,
    STOP_RECORDING = 500,
    CAPTURE_IMG = 600
};

CameraEnumeratorAndroid::CameraEnumeratorAndroid()
{
    cameraType = "Android phone";
    listRequiredField.push_back(CameraEnumeratorField("ip_address", "text", "ip address", "192.168."));
    listRequiredField.push_back(CameraEnumeratorField("port", "text", "port", "25600"));
}

CameraEnumeratorAndroid::~CameraEnumeratorAndroid()
{
}

std::vector<std::string> splitString(const char* str, int length, char delim)
{
    std::vector<std::string> list;
    int start = 0;
    for(int i = 0; i <= length; i++)
    {
        if(i == length || str[i] == delim)
        {
            if(i > start)
                list.push_back(std::string(&str[start], &str[i]));
            start = i+1;
        }
    }
    return list;
}

bool CameraEnumeratorAndroid::detectCameras()
{
    qDebug() << "detectCameras";
    listCameras.clear();
    BufferedSocket bufferedSock;
    std::string ip_address = listRequiredField[0].value;
    int port = std::stoi(listRequiredField[1].value);
    if (!bufferedSock.connect(ip_address, port))
    {
        return false;
    }
    qDebug() << "connected";
    bufferedSock.sendInt32(AndroidCameraCmd::LIST_CAMERAS);
    bufferedSock.sendInt64(0);
    int ret = bufferedSock.readInt32();
    if(ret != AndroidCameraCmd::LIST_CAMERAS)
    {
        qDebug() << "bad reply";
        bufferedSock.disconnect();
        return false;
    }
    int nbCameras = bufferedSock.readInt32();
    qDebug() << "nbCameras : " << nbCameras;
    for(int i = 0; i < nbCameras; i++)
    {
        CameraInfo camInfo;
        int size = bufferedSock.readInt32();
        char *data = new char[size+1];
        bufferedSock.readNBytes(data, size);
        std::vector<std::string> lines = splitString(data, size, '\n');
        for(std::string& line : lines)
        {
            if(line.rfind("id:", 0) == 0)
                camInfo.id = ip_address+":"+std::to_string(port)+":"+line.substr(3);
            else if(line.rfind("name:", 0) == 0)
                camInfo.name = line.substr(5);
            else if(line.rfind("desc:", 0) == 0)
                camInfo.description = line.substr(5);
        }
        qDebug() << camInfo.id.c_str();
        listCameras.push_back(camInfo);
        delete [] data;
    }
    bufferedSock.disconnect();
    return true;
}






CameraInterfaceAndroid::CameraInterfaceAndroid()
{
}

CameraInterfaceAndroid::~CameraInterfaceAndroid()
{
}

bool CameraInterfaceAndroid::open(std::string params)
{
    qDebug() << "CameraInterfaceAndroid::open(\"" << params.c_str() << "\")";
    std::vector<std::string> list_params = splitString(params.c_str(), params.length(), ':');
    if(!bufferedSock.connect(list_params[0], std::stoi(list_params[1])))
    {
        return false;
    }
    qDebug() << "connection successful";
    if(!syncTimestamp())
        return false;
    return true;
}

bool CameraInterfaceAndroid::close()
{
    bufferedSock.disconnect();
    return true;
}

std::vector<ImageFormat> CameraInterfaceAndroid::getAvailableFormats()
{
    std::vector<ImageFormat> listFormats(1);
    listFormats[0].width = 640;
    listFormats[0].height = 480;
    /*listFormats[1].width = 1280;
    listFormats[1].height = 720;
    listFormats[2].width = 1920;
    listFormats[2].height = 1080;*/
    for(int i = 0; i < listFormats.size(); i++)
        listFormats[i].type = ImageType::MJPG;
    return listFormats;
}

void CameraInterfaceAndroid::selectFormat(ImageFormat format)
{
    imageFormat = format;
}

std::shared_ptr<ImageData> CameraInterfaceAndroid::getNewFrame(bool skipOldFrames)
{
    bufferedSock.sendInt32(CAPTURE_IMG);
    DataPacket packet;
    packet.putInt32(imageFormat.width);
    packet.putInt32(imageFormat.height);
    std::string format = "";
    if(imageFormat.type == ImageType::MJPG)
        format = "MJPG";
    packet.putInt32(format.size());
    packet.putNBytes(format.c_str(), format.size());
    bufferedSock.sendInt64(packet.size());
    bufferedSock.sendNBytes(packet.getRawPtr(), packet.size());

    if(bufferedSock.readInt32() != CAPTURE_IMG)
    {
        qDebug() << "protocol error";
        errorMsg = "protocol error\n";
        return std::shared_ptr<ImageData>();
    }

    int32_t frame_id = bufferedSock.readInt32();
    int64_t timestamp = bufferedSock.readInt64();
    int64_t size = bufferedSock.readInt64();
    std::shared_ptr<ImageData> img = std::make_shared<ImageData>();
    img->imageFormat.type = ImageType::MJPG;
    img->imageFormat.width = imageFormat.width;
    img->imageFormat.height = imageFormat.height;
    img->timestamp = timestamp + timestampOffsetMs;
    img->allocData(size);
    bufferedSock.readNBytes((char*)img->data, size);
    qDebug() << "new frame, id : " << frame_id << "timestamp : " << img->timestamp;
    return img;
}
std::string CameraInterfaceAndroid::getErrorMsg()
{
    return errorMsg;
}

bool CameraInterfaceAndroid::startCapturing()
{
    return true;
}

bool CameraInterfaceAndroid::stopCapturing()
{
    return true;
}

bool CameraInterfaceAndroid::hasRecordingCapability()
{
    return true;
}
bool CameraInterfaceAndroid::startRecording()
{
    bufferedSock.sendInt32(START_RECORDING);
    bufferedSock.sendInt64(0);
    if(bufferedSock.readInt32() != START_RECORDING)
    {
        qDebug() << "protocol error";
        errorMsg = "protocol error\n";
        return false;
    }
    return true;
}
bool CameraInterfaceAndroid::stopRecordingAndSaveToFile(std::string filename)
{
    bufferedSock.sendInt32(STOP_RECORDING);
    bufferedSock.sendInt64(0);
    if(bufferedSock.readInt32() != STOP_RECORDING)
    {
        qDebug() << "protocol error";
        errorMsg = "protocol error\n";
        return false;
    }
    int64_t startRecordTimestamp = bufferedSock.readInt64();
    int64_t size = bufferedSock.readInt64();
    FILE *file = fopen(filename.c_str(), "wb");
    int64_t offset = 0;
    while(offset < size)
    {
        char buf[1024];
        int max_read_len = (int)std::min((int64_t)sizeof(buf), size - offset);
        int len = bufferedSock.readNBytes(buf, max_read_len);
        if(len < 0)
            break;
        if(file != NULL)
            fwrite(buf, 1, len, file);
        offset += len;
    }
    if(!file)
    {
        qDebug() << "can not open file: " << filename.c_str();
        errorMsg = "can not open file: "+filename;
        return false;
    }
    fclose(file);
    return true;
}

bool CameraInterfaceAndroid::syncTimestamp()
{
    bufferedSock.sendInt32(TIMESTAMP);
    bufferedSock.sendInt64(0);
    uint64_t startTimestamp = getTimestampMs();
    if(bufferedSock.readInt32() != TIMESTAMP)
    {
        qDebug() << "protocol error";
        errorMsg = "protocol error\n";
        return false;
    }
    uint64_t endTimestamp = getTimestampMs();
    int64_t androidTimestampMs = bufferedSock.readInt64();
    qDebug() << "roundtrip time : " << (endTimestamp - startTimestamp) << "ms";
    timestampOffsetMs = ((int64_t)(startTimestamp+endTimestamp)/2) - androidTimestampMs;
    qDebug() << "offset : " << timestampOffsetMs << "ms";
    return true;
}





CameraInterfaceFactoryAndroid::CameraInterfaceFactoryAndroid()
{
}

CameraInterfaceFactoryAndroid::~CameraInterfaceFactoryAndroid()
{
}

CameraInterface *CameraInterfaceFactoryAndroid::createInterface()
{
    return new CameraInterfaceAndroid();
}
