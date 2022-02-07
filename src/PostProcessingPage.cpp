#include "mainwindow.h"

void MainWindow::setPostProcessingPage()
{
    currentPageName = "postProcessing";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(100,100,100,300);

    QLabel *postProcessingLabel = new QLabel;
    postProcessingLabel->setText("post processing: ");

    progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setMaximumWidth(500);


    layout->addWidget(postProcessingLabel);
    layout->addWidget(progressBar);
    mainWidget->setLayout(layout);

    segmentationProcess = new QProcess(this);
    segmentationProcess->setEnvironment( QProcess::systemEnvironment() );
    segmentationProcess->setProcessChannelMode( QProcess::MergedChannels );
    QString inputVideo = QString::fromStdString(recordedVideoFilename);
    QString outputVideo = QString::fromStdString(record_folder + "/camVideo_mask.mp4");
    segmentationProcess->start("python3", QStringList() << "humanSegmentation.py" << inputVideo << outputVideo);
    //process->waitForStarted();
    connect (segmentationProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
}

void MainWindow::processOutput()
{
    qDebug() << segmentationProcess->readAllStandardOutput();
}


void MainWindow::postProcessThreadFunc()
{
    std::string timestampFilename = (record_folder+"/questVidTimestamp.txt");
    FILE *timestampFile = fopen(timestampFilename.c_str(), "r");
    uint64_t firstTimestamp = 0, lastTimestamp = 0;
    printf("load timestamps\n");
    if(timestampFile)
    {
        unsigned long long tmp;
        fscanf(timestampFile, "%llu\n", &tmp);
        firstTimestamp = static_cast<uint64_t>(tmp);
        while(!feof(timestampFile)) {
            fscanf(timestampFile, "%llu\n", &tmp);
            lastTimestamp = static_cast<uint64_t>(tmp);
        }
        fclose(timestampFile);
    }
    if(lastTimestamp > firstTimestamp)
    {
        printf("start processing\n");
        VideoEncoder *videoEncoder = NULL;
        timestampFile = NULL;
        libQuestMR::QuestVideoMngr mngr;
        libQuestMR::QuestVideoSourceFile videoSrc;
        videoSrc.open((record_folder+"/questVid.questMRVideo").c_str());
        mngr.attachSource(&videoSrc);
        mngr.setRecordedTimestampSource(timestampFilename.c_str());
        while(true)
        {
            if(!videoSrc.isValid())
                break;
            mngr.VideoTickImpl();
            uint64_t timestamp;
            cv::Mat img = mngr.getMostRecentImg(&timestamp);
            printf("process %lu / %lu\n", timestamp - firstTimestamp, lastTimestamp - firstTimestamp);
            if(!img.empty())
            {
                img = img(cv::Rect(0,0,img.cols/2,img.rows)).clone();
                if(videoEncoder == NULL)
                {
                    videoEncoder = new VideoEncoder();
                    videoEncoder->open((record_folder+"/questVid_processed.h264").c_str(), img.size(), 30);
                    timestampFile = fopen((record_folder+"/questVid_processedTimestamp.txt").c_str(), "w");
                }
                postProcessVal = ((double)(timestamp - firstTimestamp)) / (lastTimestamp - firstTimestamp);
                fprintf(timestampFile, "%llu\n", static_cast<unsigned long long>(timestamp));
                videoEncoder->write(img);
            }
        }
        if(videoEncoder != NULL)
        {
            videoEncoder->release();
            fclose(timestampFile);
        }
    }
}
