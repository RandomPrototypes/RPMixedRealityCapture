#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <string>
#include <vector>
#include <memory>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
}

uint64_t getTimestampMs();


enum class ImageType
{
    UNKNOWN,
    RGB,
    BGR,
    YUYV422,
    MJPG,
    NB_FORMAT_TYPE
};

enum class VideoCodecType
{
    UNKNOWN,
    H264,
};

enum class VideoContainerType
{
    UNKNOWN,
    MP4
};

std::string toString(ImageType type);

class ImageFormat
{
public:
    int width, height;
    int fps;
    ImageType type;

    ImageFormat();

    std::string toString();
};


class ImageData
{
public:
    ImageFormat imageFormat;
    unsigned long long timestamp;
    unsigned char *data;
    int dataSize;
    bool releaseDataWhenDestroy;

    ImageData(ImageFormat imageFormat = ImageFormat(), unsigned char *data = NULL, int dataSize = 0, unsigned long long timestamp = 0);
    ~ImageData();
    void allocData(int size);
    void freeData();
};

class ImageFormatConverter
{
public:
    ImageFormatConverter(ImageFormat srcFormat, ImageFormat dstFormat);
    ~ImageFormatConverter();

    void init(ImageFormat srcFormat, ImageFormat dstFormat);

    bool convertImage(const std::shared_ptr<ImageData>& srcImg, std::shared_ptr<ImageData> dstImg);


    SwsContext *swsContext = nullptr;
    AVCodec *codec = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVPacket *pkt  = nullptr;
    AVFrame *frame = nullptr;


    ImageFormat srcFormat;
    ImageFormat dstFormat;
    int src_linesize[4];
    int dst_linesize[4];
    AVPixelFormat srcPixelFormat, dstPixelFormat;
};

class CameraInfo
{
public:
    std::string id;
    std::string name;
    std::string description;
};

class CameraEnumeratorField
{
public:
    std::string name;
    std::string type;
    std::string text;
    std::string value;
    void *extra_param;

    CameraEnumeratorField(std::string name, std::string type, std::string text, std::string value = "");
};

class CameraEnumerator
{
public:
    CameraEnumerator();
    virtual ~CameraEnumerator();

    virtual bool detectCameras() = 0;
    virtual std::string getCameraId(int index);
    virtual std::string getCameraName(int index);
    virtual std::string getCameraDescription(int index);
    virtual int getCameraIndex(const std::string& id);
    virtual std::string getCameraName(const std::string& id);
    virtual std::string getCameraDescription(const std::string& id);
    virtual int count();

    std::string cameraType;
    std::vector<CameraInfo> listCameras;
    std::vector<CameraEnumeratorField> listRequiredField;//first : field name, second : field type ("text", "number")
};

class CameraInterface
{
public:
    CameraInterface();
    virtual ~CameraInterface();
    virtual bool open(std::string params) = 0;
    virtual bool close() = 0;
    virtual bool startCapturing() = 0;
    virtual bool stopCapturing() = 0;
    virtual bool hasRecordingCapability();
    virtual bool startRecording();
    virtual bool stopRecordingAndSaveToFile(std::string filename);
    virtual std::vector<ImageFormat> getAvailableFormats();
    virtual std::vector<VideoCodecType> getAvailableVideoCodec();
    virtual std::vector<VideoContainerType> getAvailableVideoContainer();
    virtual void selectFormat(int formatId);
    virtual void selectVideoContainer(int containerId);
    virtual void selectVideoCodec(int codecId);
    virtual void selectFormat(ImageFormat format);
    virtual void selectVideoContainer(VideoContainerType container);
    virtual void selectVideoCodec(VideoCodecType codec);
    virtual std::shared_ptr<ImageData> getNewFrame(bool skipOldFrames) = 0;
    virtual std::string getErrorMsg();
};

class CameraInterfaceFactory
{
public:
    CameraInterfaceFactory();
    virtual ~CameraInterfaceFactory();
    virtual CameraInterface *createInterface() = 0;
};

class CameraEnumAndFactory
{
public:
    CameraEnumerator* enumerator;
    CameraInterfaceFactory*  interfaceFactory;

    CameraEnumAndFactory(CameraEnumerator* enumerator, CameraInterfaceFactory*  interfaceFactory);
};

class CameraMngr
{
public:
    std::vector<CameraEnumAndFactory> listCameraEnumAndFactory;

    void registerEnumAndFactory(CameraEnumerator* enumerator, CameraInterfaceFactory* factory);

    static CameraMngr *getInstance();
private:
    static CameraMngr *instance;

    CameraMngr();
    ~CameraMngr();
};

#endif // CAMERAINTERFACE_H
