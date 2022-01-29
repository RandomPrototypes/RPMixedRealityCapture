#ifndef CAMERAINTERFACEV4L2_H
#define CAMERAINTERFACEV4L2_H

#include "CameraInterface.h"
#include <sstream>

enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct V4L2_buffer {
    void   *start;
    size_t  length;
};

class CameraInterfaceV4L2 : public CameraInterface
{
public:
    CameraInterfaceV4L2();
    ~CameraInterfaceV4L2();
    virtual bool open(std::string params);
    virtual bool close();
    virtual std::vector<ImageFormat> getAvailableFormats();
    virtual void selectFormat(int formatId);
    virtual void selectPreviewFormat(int formatId);
    virtual void selectFormat(ImageFormat format);
    virtual void selectPreviewFormat(ImageFormat format);
    virtual std::shared_ptr<ImageData> getNewFrame(bool skipOldFrames);
    virtual std::string getErrorMsg();

    virtual bool startCapturing();
    virtual bool stopCapturing();
private:
    bool initDevice();
    bool uninitDevice();
    bool init_read(unsigned int buffer_size);
    bool init_mmap();
    bool init_userp(unsigned int buffer_size);
    int read_frame();
    void requestImageFormatList();

    void process_image(const void *p, int size);

    int fd;
    std::string dev_name;
    std::ostringstream errorMsg;

    int selectedFormat, selectedPreviewFormat;

    ImageFormat imageFormat;

    struct V4L2_buffer *buffers;

    std::vector<ImageFormat> listImageFormats;

    io_method io = IO_METHOD_MMAP;

    unsigned char *lastFrameData;
    int lastFrameDataLength;
    int lastFrameDataAllocatedSize;

    unsigned int n_buffers;
};

class CameraInterfaceFactoryV4L2 : public CameraInterfaceFactory
{
public:
    CameraInterfaceFactoryV4L2();
    virtual ~CameraInterfaceFactoryV4L2();
    virtual CameraInterface *createInterface();
};

#endif // CAMERAINTERFACEV4L2_H
