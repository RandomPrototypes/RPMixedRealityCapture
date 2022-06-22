#include "mainwindow.h"

#include <BufferedSocket/BufferedSocket.h>

#include <QApplication>

enum AndroidCameraCmd
{
    EXIT = 100,
    LIST_CAMERAS =200,
    TIMESTAMP = 300,
    START_RECORDING = 400,
    STOP_RECORDING = 500,
    CAPTURE_IMG = 600
};


int main(int argc, char *argv[])
{
    libQuestMR::setBackgroundSubtractorResourceFolder("resources/backgroundSub_data");
    /*BufferedSocket bufferedSock;

    if (!bufferedSock.connect("192.168.10.101", 25600))
    {
        qDebug() << ("Failed to connect.");
        return 0;
    }

    //while(true)
    {
        std::string format = "mjpg";
        bufferedSock.sendInt32(CAPTURE_IMG);
        DataPacket packet;
        packet.putInt32(720);
        packet.putInt32(480);
        packet.putInt32(format.size());
        packet.putNBytes(format.c_str(), format.size());
        bufferedSock.sendInt64(packet.size());
        bufferedSock.sendNBytes(packet.getRawPtr(), packet.size());
        char buf[255];

        if(bufferedSock.readInt32() != CAPTURE_IMG)
        {
            printf("protocol error\n");
            return 0;
        }
        int64_t timestamp = bufferedSock.readInt64();
        int64_t size = bufferedSock.readInt64();
        qDebug() << size << " timestamp " << timestamp;

        std::shared_ptr<ImageData> img = RPCameraInterface::createImageData();
        img->imageFormat.type = ImageType::MJPG;
        img->imageFormat.width = 720;
        img->imageFormat.height = 480;
        img->allocData(size);

        bufferedSock.readNBytes((char*)img->data, size);

        ImageFormat dstFormat;
        dstFormat.width = 720;
        dstFormat.height = 480;
        dstFormat.type = ImageType::BGR;
        ImageFormatConverter converter(img->imageFormat, dstFormat);

        std::shared_ptr<ImageData> img2 = RPCameraInterface::createImageData();

        converter.convertImage(img, img2);
        cv::Mat resultImg2(img2->imageFormat.height, img2->imageFormat.width, CV_8UC3, img2->data);
        //cv::Mat resultImg2(img->imageFormat.height, img->imageFormat.width, CV_8UC1, img->data, 720);
        cv::imshow("img", resultImg2);
        cv::waitKey(0);
    }
    std::string cmd = "start_recording\n";
    bufferedSock.sendInt32(START_RECORDING);
    bufferedSock.sendInt64(0);
    if(bufferedSock.readInt32() != START_RECORDING)
    {
        printf("protocol error\n");
        return 0;
    }
    cv::waitKey(0);
    cmd = "stop_recording\n";
    bufferedSock.sendInt32(STOP_RECORDING);
    bufferedSock.sendInt64(0);
    if(bufferedSock.readInt32() != STOP_RECORDING)
    {
        printf("protocol error\n");
        return 0;
    }

    {
        char buf[255];

        int32_t size = bufferedSock.readInt64();
        qDebug() << size;

        FILE *file = fopen("test_android.mp4", "wb");
        int offset = 0;
        while(offset < size)
        {
            char buf[1024];
            int max_read_len = std::min((int)sizeof(buf), (int)size - offset);
            int len = bufferedSock.readNBytes(buf, max_read_len);
            if(len < 0)
                break;
            fwrite(buf, 1, len, file);
            offset += len;
        }
        fclose(file);
    }
    bufferedSock.disconnect();*/

    QApplication a(argc, argv);
    MainWindow w;
    w.resize(1000,800);
    w.showMaximized();
    return a.exec();

    /*CameraInterfaceV4L2 cam;
    if(!cam.open("/dev/video0"))
    {
        printf("%s\n", cam.getErrorMsg().c_str());
        return 0;
    }
    std::vector<ImageFormat> listFormats = cam.getAvailableFormats();
    int selectedFormat = -1;
    for(size_t i = 0; i < listFormats.size(); i++)
    {
        if(listFormats[i].type == ImageType::MJPG && listFormats[i].width == 1920 && listFormats[i].height == 1080)
        {
            selectedFormat = i;
            break;
        }
    }
    cam.selectFormat(selectedFormat);
    if(!cam.startCapturing())
    {
        printf("%s\n", cam.getErrorMsg().c_str());
        return 0;
    }

    ImageFormat dstFormat;
    dstFormat.width = 1920;
    dstFormat.height = 1080;
    dstFormat.type = ImageType::BGR;
    ImageFormatConverter converter(listFormats[selectedFormat], dstFormat);

    ImageData resultImg;
    while(true)
    {
        qDebug() << "getNewFrame...";
        std::shared_ptr<ImageData> img = cam.getNewFrame(true, false);
        converter.convertImage(*img, &resultImg);
        cv::Mat resultImg2(resultImg.imageFormat.height, resultImg.imageFormat.width, CV_8UC3, resultImg.data);
        cv::imshow("img", resultImg2);
        cv::waitKey(10);
    }
    resultImg.freeData();*/

}
