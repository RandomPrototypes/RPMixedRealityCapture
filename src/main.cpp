#include "mainwindow.h"

#include <BufferedSocket/BufferedSocket.h>

#include <libQuestMR/QuestCalibData.h>

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

template<typename T>
std::string mat2str(const cv::Mat& mat)
{
    std::string str;
    for(int i = 0; i < mat.rows; i++) {
        for(int j = 0; j < mat.cols; j++) {
            str += std::to_string(mat.at<T>(i,j)) + ", ";
        }
        str += "\n";
    }
    return str;
}

void calibrateCamPose(libQuestMR::QuestCalibData &calibData, cv::Point3d camOrig, const std::vector<cv::Point3d>& listPoint3d, const std::vector<cv::Point2d>& listPoint2d)
{
    //z * (listPoint2d[i].x, listPoint2d[i].y, 1) = K * R * (listPoint3d[i] - camOrig)
    //K^-1 * z * (listPoint2d[i].x, listPoint2d[i].y, 1) = R * (listPoint3d[i] - camOrig)
    //normalize(K^-1 * z * (listPoint2d[i].x, listPoint2d[i].y, 1)) = R * normalize(listPoint3d[i] - camOrig)
    //normalize(K^-1 * (listPoint2d[i].x, listPoint2d[i].y, 1)) = R * normalize(listPoint3d[i] - camOrig)
    std::vector<cv::Point3d> ray(listPoint3d.size()), p3d(listPoint3d.size());
    cv::Mat K_inv = (calibData.getFlipXMat()*calibData.getCameraMatrix()).inv();
    //qDebug() << "K_inv:\n" << mat2str<double>(K_inv).c_str();
    const double *K_inv_ptr = K_inv.ptr<double>(0);
    for(size_t i = 0; i < listPoint2d.size(); i++) {
        cv::Point3d p(-listPoint2d[i].x, -listPoint2d[i].y, -1);
        cv::Point3d p1 = p;
        p1 /= p1.z;
        p1 /= sqrt(p1.dot(p1));
        cv::Point3d p2 = p;
        p2 /= sqrt(p2.dot(p2));
        //qDebug() << "("<<p1.x << ","<<p1.y<<","<<p1.z<<") == " << "("<<p2.x << ","<<p2.y<<","<<p2.z<<")";
        //p /= p.z;
        p /= sqrt(p.dot(p));
        ray[i].x = K_inv_ptr[0] * p.x + K_inv_ptr[1] * p.y + K_inv_ptr[2] * p.z;
        ray[i].y = K_inv_ptr[3] * p.x + K_inv_ptr[4] * p.y + K_inv_ptr[5] * p.z;
        ray[i].z = K_inv_ptr[6] * p.x + K_inv_ptr[7] * p.y + K_inv_ptr[8] * p.z;
        ray[i] /= sqrt(ray[i].dot(ray[i]));
    }
    for(size_t i = 0; i < listPoint3d.size(); i++) {
        p3d[i] = listPoint3d[i] - camOrig;
        p3d[i] /= sqrt(p3d[i].dot(p3d[i]));
    }
    cv::Mat R = libQuestMR::estimateRotation3D(p3d, ray);
    double *R_ptr = R.ptr<double>(0);
    for(size_t i = 0; i < p3d.size(); i++) {
        cv::Point3d p = p3d[i];//listPoint3d[i] - camOrig;
        cv::Point3d p2;
        p2.x = R_ptr[0] * p.x + R_ptr[1] * p.y + R_ptr[2] * p.z;
        p2.y = R_ptr[3] * p.x + R_ptr[4] * p.y + R_ptr[5] * p.z;
        p2.z = R_ptr[6] * p.x + R_ptr[7] * p.y + R_ptr[8] * p.z;
        cv::Point3d p3 = p2 / sqrt(p2.dot(p2));
        //qDebug() << "diff " << sqrt((ray[i]-p3).dot(ray[i]-p3));
    }
}

int main(int argc, char *argv[])
{
    /*cv::Vec3f eulerVal(1.2f, 0.4f, -1.2f);
    cv::Mat R = libQuestMR::eulerAnglesToRotationMatrix(eulerVal);
    cv::Point3d camOrig(1,5,3);
    libQuestMR::QuestCalibData calibData;
    calibData.setCameraFromSizeAndFOV(90*CV_PI/180, 1280, 720);

    cv::Mat flipMat = calibData.getFlipXMat();
    cv::Mat K = calibData.getCameraMatrix();

    cv::Mat KR = K*R;

    cv::Mat flipKR = flipMat * K * R;
    double *flipKR_ptr = flipKR.ptr<double>(0);

    std::vector<cv::Point3d> listPoint3d(9);
    std::vector<cv::Point3d> listRay(listPoint3d.size());
    std::vector<cv::Point2d> listPoint2d(listPoint3d.size());
    for(size_t i = 0; i < listPoint3d.size(); i++) {
        while(true) {
            listPoint3d[i] = camOrig + 0.1*cv::Point3d((rand()%1000) - 500, (rand()%1000) - 500, (rand()%1000) - 500);
            cv::Point3d p = listPoint3d[i] - camOrig;
            cv::Point3d ray;
            ray.x = flipKR_ptr[0] * p.x + flipKR_ptr[1] * p.y + flipKR_ptr[2] * p.z;
            ray.y = flipKR_ptr[3] * p.x + flipKR_ptr[4] * p.y + flipKR_ptr[5] * p.z;
            ray.z = flipKR_ptr[6] * p.x + flipKR_ptr[7] * p.y + flipKR_ptr[8] * p.z;
            ray /= sqrt(ray.dot(ray));

            listRay[i] = ray;//cv::Point3d(ray.x/ray.z, ray.y/ray.z, 1);
            listPoint2d[i] = cv::Point2d(ray.x/ray.z, ray.y/ray.z);
            if(ray.z < 0)
                break;
        }
    }
    calibrateCamPose(calibData, camOrig, listPoint3d, listPoint2d);

    calibData.calibrateCamPose(camOrig, listPoint3d, listPoint2d);
    cv::Mat R2 = libQuestMR::quaternion2rotationMat(calibData.getRotation()).inv();
    for(size_t i = 0; i < listPoint3d.size(); i++)
    {
        cv::Point2d p = calibData.projectToCam(listPoint3d[i]);
        double diff = sqrt((listPoint2d[i]-p).dot((listPoint2d[i]-p)));
        qDebug() << diff;
    }

    qDebug() << "R:\n" << mat2str<double>(R).c_str();
    qDebug() << "R2:\n" << mat2str<double>(R2).c_str();

    //qDebug() << libQuestMR::testRotationEstimation();
    //return 0;*/

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
