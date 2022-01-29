#include "CameraInterfaceV4l2.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

//based on https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/capture.c.html


#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define errno_exit(str) \
    { \
    errorMsg.clear(); \
    errorMsg << str << " error " << errno << ", " << strerror(errno); \
    return false; \
    }

unsigned int ImageTypeToV4L2(ImageType type)
{
    if(type == ImageType::YUYV422)
        return V4L2_PIX_FMT_YUYV;
    else if(type == ImageType::MJPG)
        return V4L2_PIX_FMT_MJPEG;
    else return -1;
}

ImageType V4L2ToImageType(unsigned int format)
{
    if(format == V4L2_PIX_FMT_YUYV)
        return ImageType::YUYV422;
    else if(format == V4L2_PIX_FMT_MJPEG)
        return ImageType::MJPG;
    else return ImageType::UNKNOWN;
}

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}


CameraInterfaceV4L2::CameraInterfaceV4L2()
{
    fd = -1;
    selectedFormat = -1;
    selectedPreviewFormat = -1;
    lastFrameData = NULL;
    lastFrameDataLength = 0;
    lastFrameDataAllocatedSize = 0;
}

CameraInterfaceV4L2::~CameraInterfaceV4L2()
{

}


void CameraInterfaceV4L2::process_image(const void *p, int size)
{
    switch (io) {
    case IO_METHOD_READ:
        lastFrameData = (unsigned char*)p;
        lastFrameDataLength = size;
        lastFrameDataAllocatedSize = 0;
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if(lastFrameData == NULL || lastFrameDataAllocatedSize < size)
        {
            if(lastFrameData != NULL)
                free(lastFrameData);
            lastFrameData = (unsigned char*)malloc(size);
            lastFrameDataAllocatedSize = size;
        }
        memcpy(lastFrameData, p, size);
        lastFrameDataLength = size;
        break;
    }
}

bool CameraInterfaceV4L2::open(std::string params)
{
    dev_name = params;
    struct stat st;

    if (-1 == stat(dev_name.c_str(), &st)) {
        errorMsg.clear();
        errorMsg << "Cannot identify '" << dev_name << "': " << errno << ", " << strerror(errno);
        return false;
    }

    if (!S_ISCHR(st.st_mode)) {
        errorMsg.clear();
        errorMsg << dev_name << "is no device";
        return false;
    }

    fd = ::open(dev_name.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        errorMsg.clear();
        errorMsg << "Cannot open '" << dev_name << "': " << errno << ", " << strerror(errno);
        return false;
    }

    requestImageFormatList();

    return true;
}

bool CameraInterfaceV4L2::initDevice()
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        errorMsg.clear();
        if (EINVAL == errno)
            errorMsg << dev_name << "is no V4L2 device";
        else errno_exit("VIDIOC_QUERYCAP");
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        errorMsg.clear();
        errorMsg << "is no video capture device";
        return false;
    }

    switch (io) {
    case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                errorMsg.clear();
                errorMsg << dev_name << " does not support read i/o";
                return false;
            }
            break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                errorMsg.clear();
                errorMsg << dev_name << " does not support streaming i/o";
                return false;
            }
            break;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                    /* Cropping not supported. */
                    break;
            default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
            /* Errors ignored. */
    }


    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (selectedFormat != -1) {
        ImageFormat& imgFormat = listImageFormats[selectedFormat];
        fmt.fmt.pix.width       = imgFormat.width;
        fmt.fmt.pix.height      = imgFormat.height;
        fmt.fmt.pix.pixelformat = ImageTypeToV4L2(imgFormat.type);
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
            errorMsg << "VIDIOC_S_FMT error " << errno << ", " << strerror(errno);
            return false;
        }

        /* Note VIDIOC_S_FMT may change width and height. */
    } else {
            /* Preserve original settings as set by v4l2-ctl for example */
            if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
                errorMsg << "VIDIOC_G_FMT error " << errno << ", " << strerror(errno);
                return false;
            }
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;


    switch (io) {
    case IO_METHOD_READ:
        return init_read(fmt.fmt.pix.sizeimage);

    case IO_METHOD_MMAP:
        return init_mmap();

    case IO_METHOD_USERPTR:
        return init_userp(fmt.fmt.pix.sizeimage);
    }
    return true;
}

bool CameraInterfaceV4L2::uninitDevice()
{
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        free(buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(buffers[i].start, buffers[i].length))
                errno_exit("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free(buffers[i].start);
        break;
    }

    free(buffers);
    return true;
}


bool CameraInterfaceV4L2::init_read(unsigned int buffer_size)
{
    printf("init_read\n");
    buffers = (struct V4L2_buffer*)calloc(1, sizeof(*buffers));

    if (!buffers) {
        errorMsg << "Out of memory";
        return false;
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc(buffer_size);

    if (!buffers[0].start) {
        errorMsg << "Out of memory";
        return false;
    }
    return true;
}

bool CameraInterfaceV4L2::init_mmap()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            errorMsg.clear();
            errorMsg << dev_name << " does not support memory mapping";
            return false;
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        errorMsg.clear();
        errorMsg << "Insufficient buffer memory on " << dev_name;
        return false;
    }

    buffers = (struct V4L2_buffer*)calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        errorMsg.clear();
        errorMsg << "Out of memory";
        return false;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
           errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
        mmap(NULL /* start anywhere */,
              buf.length,
              PROT_READ | PROT_WRITE /* required */,
              MAP_SHARED /* recommended */,
              fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }

    return true;
}

bool CameraInterfaceV4L2::init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            errorMsg.clear();
            errorMsg << dev_name << " does not support user pointer i/o";
            return false;
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    buffers = (struct V4L2_buffer*)calloc(4, sizeof(*buffers));

    if (!buffers) {
        errorMsg.clear();
        errorMsg << "Out of memory";
        return false;
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = malloc(buffer_size);

        if (!buffers[n_buffers].start) {
            errorMsg.clear();
            errorMsg << "Out of memory";
            return false;
        }
    }
    return true;
}

bool CameraInterfaceV4L2::startCapturing()
{
    unsigned int i;
    enum v4l2_buf_type type;

    if(!initDevice())
        return false;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)buffers[i].start;
            buf.length = buffers[i].length;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;
    }
    return true;
}

bool CameraInterfaceV4L2::stopCapturing()
{
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");
        break;
    }
    return uninitDevice();
}

bool CameraInterfaceV4L2::close()
{
    if (-1 == ::close(fd)) {
        errorMsg.clear();
        errorMsg << "close";
        return false;
    }
    fd = -1;
    return true;
}

std::vector<ImageFormat> CameraInterfaceV4L2::getAvailableFormats()
{   
    return listImageFormats;
}

void CameraInterfaceV4L2::requestImageFormatList()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_fmtdesc fmt;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;

    fmt.index = 0;
    fmt.type = type;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
        frmsize.pixel_format = fmt.pixelformat;
        frmsize.index = 0;
        char *fourc_fmt = *(char**)&(fmt.pixelformat);
        //printf("format %c%c%c%c\n", fourc_fmt[0], fourc_fmt[1], fourc_fmt[2], fourc_fmt[3]);
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                printf("discrete %dx%d\n",
                                  frmsize.discrete.width,
                                  frmsize.discrete.height);
                ImageFormat imgFormat;
                imgFormat.type = V4L2ToImageType(fmt.pixelformat);

                if(imgFormat.type != ImageType::UNKNOWN)
                {
                    imgFormat.width = frmsize.discrete.width;
                    imgFormat.height = frmsize.discrete.height;

                    printf("%s\n", imgFormat.toString().c_str());

                    listImageFormats.push_back(imgFormat);
                }
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                printf("stepwise %dx%d\n",
                                  frmsize.stepwise.max_width,
                                  frmsize.stepwise.max_height);
            }
                frmsize.index++;
            }
            fmt.index++;
    }
}

void CameraInterfaceV4L2::selectFormat(int formatId)
{
    selectedFormat = formatId;
    imageFormat = listImageFormats[formatId];
}

void CameraInterfaceV4L2::selectPreviewFormat(int formatId)
{
    selectedPreviewFormat = formatId;
}

void CameraInterfaceV4L2::selectFormat(ImageFormat format)
{
    selectedFormat = -1;
    for(size_t i = 0; i < listImageFormats.size(); i++)
    {
        if (listImageFormats[i].width == format.width
         && listImageFormats[i].height == format.height
         && listImageFormats[i].type == format.type)
        {
            selectedFormat = i;
            break;
        }
    }
    imageFormat = format;
}
void CameraInterfaceV4L2::selectPreviewFormat(ImageFormat format)
{

}

int CameraInterfaceV4L2::read_frame()
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */
            default:
                errorMsg.clear();
                errorMsg << "read error";
                return -1;
            }
        }
        process_image(buffers[0].start, buffers[0].length);
        break;
    case IO_METHOD_MMAP:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                    return 0;

            case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */
            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        assert(buf.index < n_buffers);

        process_image(buffers[buf.index].start, buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
        break;

    case IO_METHOD_USERPTR:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                    return 0;

            case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

            default:
                    errno_exit("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long)buffers[i].start
                && buf.length == buffers[i].length)
                    break;

        assert(i < n_buffers);

        process_image((void *)buf.m.userptr, buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
        break;
    }

    return 1;
}

std::shared_ptr<ImageData> CameraInterfaceV4L2::getNewFrame(bool skipOldFrames)
{
    while(true)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            errorMsg.clear();
            return std::shared_ptr<ImageData>();
        }

        if (0 == r) {
            errorMsg.clear();
            errorMsg << "select timeout";
            return std::shared_ptr<ImageData>();
        }

        int val = read_frame();
        if(val == 0)
            continue;
        if(val < 0)
            return std::shared_ptr<ImageData>();
        else break;
    }
    std::shared_ptr<ImageData> data = std::make_shared<ImageData>(imageFormat, (unsigned char*)lastFrameData, lastFrameDataLength, 0);
    data->releaseDataWhenDestroy = false;
    return data;
}

std::string CameraInterfaceV4L2::getErrorMsg()
{
    return errorMsg.str();
}



CameraInterfaceFactoryV4L2::CameraInterfaceFactoryV4L2()
{
}

CameraInterfaceFactoryV4L2::~CameraInterfaceFactoryV4L2()
{
}

CameraInterface *CameraInterfaceFactoryV4L2::createInterface()
{
    return new CameraInterfaceV4L2();
}
