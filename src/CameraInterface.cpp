#include "CameraInterface.h"
#include <sstream>
#include <qdebug.h>

CameraMngr* CameraMngr::instance = NULL;

uint64_t getTimestampMs()
{
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
}

AVPixelFormat ImageTypeToAVPixelFormat(ImageType type)
{
    if(type == ImageType::RGB)
        return AV_PIX_FMT_RGB24;
    else if(type == ImageType::BGR)
        return AV_PIX_FMT_BGR24;
    else if(type == ImageType::YUYV422)
        return AV_PIX_FMT_YUYV422;
    else return AV_PIX_FMT_NONE;
}

std::string toString(ImageType type)
{
    switch(type)
    {
        case ImageType::UNKNOWN:
            return "unknown";
        case ImageType::YUYV422:
            return "yuyv422";
        case ImageType::RGB:
            return "rgb";
        case ImageType::BGR:
            return "bgr";
        case ImageType::MJPG:
            return "mjpg";
    }
    return "unknown";
}

ImageFormat::ImageFormat()
{
    type = ImageType::UNKNOWN;
    width = 0;
    height = 0;
    fps = 0;
}

std::string ImageFormat::toString()
{
    std::ostringstream str;
    str << ::toString(type) << ", " << width << "x" << height << ", " << fps << "fps";
    return str.str();
}

ImageFormatConverter::ImageFormatConverter(ImageFormat srcFormat, ImageFormat dstFormat)
{
    init(srcFormat, dstFormat);
}

ImageFormatConverter::~ImageFormatConverter()
{
    if(swsContext != nullptr)
        sws_freeContext(swsContext);
}

void ImageFormatConverter::init(ImageFormat srcFormat, ImageFormat dstFormat)
{
    this->srcFormat = srcFormat;
    this->dstFormat = dstFormat;

    srcPixelFormat = ImageTypeToAVPixelFormat(srcFormat.type);
    dstPixelFormat = ImageTypeToAVPixelFormat(dstFormat.type);
    //https://stackoverflow.com/questions/23443322/decoding-mjpeg-with-libavcodec
    if(srcFormat.type == ImageType::MJPG)
    {
        srcPixelFormat = AV_PIX_FMT_BGR24;//AV_PIX_FMT_YUVJ422P;
        codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
        codecContext = avcodec_alloc_context3(codec);
        avcodec_get_context_defaults3(codecContext, codec);
        avcodec_open2(codecContext, codec, nullptr);

        pkt = av_packet_alloc();
        if (!pkt) {
            qDebug() << ("Could not allocate packet\n");
            return ;
        }

        frame = av_frame_alloc();
        if (!frame) {
            qDebug() << ("Could not allocate video frame\n");
            return ;
        }
        /*frame->format = srcPixelFormat;//codecContext->pix_fmt;
        frame->width  = codecContext->width;
        frame->height = codecContext->height;*/
        //frame->channels = 3;
        /* the image can be allocated by any means and av_image_alloc() is
         * just the most convenient way if av_malloc() is to be used */
        /*int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            qDebug() << ("Could not allocate the video frame data\n");
            return ;
        }*/
        swsContext = NULL;
    }
    else
    {
        swsContext = sws_getContext(
                            srcFormat.width,
                            srcFormat.height,
                            srcPixelFormat,
                            dstFormat.width,
                            dstFormat.height,
                            dstPixelFormat,
                            SWS_POINT,
                            nullptr, nullptr, nullptr);
    }

    av_image_fill_linesizes(src_linesize, srcPixelFormat, srcFormat.width);
    av_image_fill_linesizes(dst_linesize, dstPixelFormat, dstFormat.width);
    qDebug() << "src_linesize" << src_linesize[0] << src_linesize[1] << src_linesize[2] << src_linesize[3];
    qDebug() << "dst_linesize" << dst_linesize[0] << dst_linesize[1] << dst_linesize[2] << dst_linesize[3];
}

bool ImageFormatConverter::convertImage(const std::shared_ptr<ImageData>& srcImg, std::shared_ptr<ImageData> dstImg)
{
    int total_size = dst_linesize[0] + dst_linesize[1] + dst_linesize[2] + dst_linesize[3];
    total_size *= dstFormat.height;

    if(dstImg->dataSize != total_size)
    {
        if(dstImg->data != NULL && dstImg->releaseDataWhenDestroy)
            dstImg->freeData();
        dstImg->allocData(total_size);
        dstImg->releaseDataWhenDestroy = true;
    }

    dstImg->imageFormat = dstFormat;
    dstImg->timestamp = srcImg->timestamp;

    if(srcFormat.type == ImageType::MJPG)
    {
        pkt->size = srcImg->dataSize;
        pkt->data = srcImg->data;

        int got_picture;

        // decode
        //avcodec_decode_video2(codecContext, _outputFrame, &got_picture, &_inputPacket);

        int ret = avcodec_send_packet(codecContext, pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret == AVERROR_EOF) {
            printf("Error sending a frame for decoding\n");
            return false;
        }

        pkt->size = 0;
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret < 0)
            return false;

        //memcpy(dstImg->data, frame->data[0], total_size);

        qDebug() << "frame->linesize" << frame->linesize[0] << frame->linesize[1] << frame->linesize[2] << frame->linesize[3];

        if(swsContext == NULL) {
            swsContext = sws_getContext(
                                frame->width,
                                frame->height,
                                (AVPixelFormat)frame->format,
                                dstFormat.width,
                                dstFormat.height,
                                dstPixelFormat,
                                SWS_POINT,
                                nullptr, nullptr, nullptr);

        }

        sws_scale(swsContext,
                  (const uint8_t * const *)&(frame->data[0]),
                  frame->linesize,
                  0,
                  srcImg->imageFormat.height,
                  (uint8_t* const*)&(dstImg->data),
                  dst_linesize);
    } else {
        sws_scale(swsContext,
                  (const uint8_t * const *)&(srcImg->data),
                  src_linesize,
                  0,
                  srcImg->imageFormat.height,
                  (uint8_t* const*)&(dstImg->data),
                  dst_linesize);
    }
    return true;
}

ImageData::ImageData(ImageFormat imageFormat, unsigned char *data, int dataSize, unsigned long long timestamp)
    :imageFormat(imageFormat), data(data), dataSize(dataSize), timestamp(timestamp)
{
    releaseDataWhenDestroy = true;
}

ImageData::~ImageData()
{
    if(releaseDataWhenDestroy && data != NULL)
        delete [] data;
}

void ImageData::allocData(int size)
{
    data = new unsigned char[size];
    dataSize = size;
}

void ImageData::freeData()
{
    delete [] data;
    data = NULL;
    dataSize = 0;
}

CameraInterface::CameraInterface()
{

}

CameraInterface::~CameraInterface()
{

}

std::vector<ImageFormat> CameraInterface::getAvailableFormats()
{
    return std::vector<ImageFormat>();
}

std::vector<VideoContainerType> CameraInterface::getAvailableVideoContainer()
{
    return std::vector<VideoContainerType>();
}

std::vector<VideoCodecType> CameraInterface::getAvailableVideoCodec()
{
    return std::vector<VideoCodecType>();
}

void CameraInterface::selectFormat(int formatId)
{
    std::vector<ImageFormat> listFormats = getAvailableFormats();
    if(formatId >= 0 && formatId < listFormats.size())
        selectFormat(listFormats[formatId]);
}

void CameraInterface::selectVideoContainer(int containerId)
{
    std::vector<VideoContainerType> listContainers = getAvailableVideoContainer();
    if(containerId >= 0 && containerId < listContainers.size())
        selectVideoContainer(listContainers[containerId]);
}


void CameraInterface::selectVideoCodec(int codecId)
{
    std::vector<VideoCodecType> listCodecs = getAvailableVideoCodec();
    if(codecId >= 0 && codecId < listCodecs.size())
        selectVideoCodec(listCodecs[codecId]);
}

void CameraInterface::selectFormat(ImageFormat format)
{

}

void CameraInterface::selectVideoContainer(VideoContainerType container)
{

}

void CameraInterface::selectVideoCodec(VideoCodecType codec)
{

}

bool CameraInterface::hasRecordingCapability()
{
    return false;
}
bool CameraInterface::startRecording()
{
    return false;
}
bool CameraInterface::stopRecordingAndSaveToFile(std::string filename)
{
    return false;
}

std::string CameraInterface::getErrorMsg()
{
    return "";
}

CameraInterfaceFactory::CameraInterfaceFactory()
{
}

CameraInterfaceFactory::~CameraInterfaceFactory()
{
}

CameraEnumeratorField::CameraEnumeratorField(std::string name, std::string type, std::string text, std::string value)
    :name(name), type(type), text(text), value(value)
{

}

CameraEnumerator::CameraEnumerator()
{
}

CameraEnumerator::~CameraEnumerator()
{
}

std::string CameraEnumerator::getCameraId(int id)
{
    if(id >= 0 && id < listCameras.size())
        return listCameras[id].id;
    return "unknown";
}

std::string CameraEnumerator::getCameraName(int id)
{
    if(id >= 0 && id < listCameras.size())
        return listCameras[id].name;
    return "unknown";
}

std::string CameraEnumerator::getCameraDescription(int id)
{
    if(id >= 0 && id < listCameras.size())
        return listCameras[id].description;
    return "unknown";
}

int CameraEnumerator::count()
{
    return listCameras.size();
}

int CameraEnumerator::getCameraIndex(const std::string& id)
{
    for(size_t i = 0; i < listCameras.size(); i++)
        if(listCameras[i].id == id)
            return i;
    return -1;
}

std::string CameraEnumerator::getCameraName(const std::string& id)
{
    int index = getCameraIndex(id);
    if(index >= 0)
        return getCameraName(index);
    return "unknown";
}
std::string CameraEnumerator::getCameraDescription(const std::string& id)
{
    int index = getCameraIndex(id);
    if(index >= 0)
        return getCameraDescription(index);
    return "unknown";
}

CameraEnumAndFactory::CameraEnumAndFactory(CameraEnumerator* enumerator, CameraInterfaceFactory*  interfaceFactory)
    :enumerator(enumerator), interfaceFactory(interfaceFactory)
{

}

CameraMngr::CameraMngr()
{

}

CameraMngr::~CameraMngr()
{
    for(size_t i = 0; i < listCameraEnumAndFactory.size(); i++)
    {
        delete listCameraEnumAndFactory[i].enumerator;
        delete listCameraEnumAndFactory[i].interfaceFactory;
    }
}

void CameraMngr::registerEnumAndFactory(CameraEnumerator* enumerator, CameraInterfaceFactory* factory)
{
    listCameraEnumAndFactory.push_back(CameraEnumAndFactory(enumerator, factory));
}

CameraMngr *CameraMngr::getInstance()
{
    if(instance == NULL)
        instance = new CameraMngr();
    return instance;
}
