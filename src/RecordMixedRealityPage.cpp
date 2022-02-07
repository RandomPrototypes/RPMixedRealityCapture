#include "mainwindow.h"
#include <QDir>

void MainWindow::setRecordMixedRealityPage()
{
    currentPageName = "recordMixedReality";
    clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("record mixed reality: ");

    startRecordingButton = new QPushButton("start recording");
    startRecordingButton->setMaximumWidth(300);

    QTabWidget *tabWidget = new QTabWidget;

    camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    camPreviewWidget->setImg(cv::Mat());

    questPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    questPreviewWidget->setImg(cv::Mat());

    tabWidget->addTab(camPreviewWidget, tr("Camera"));
    tabWidget->addTab(questPreviewWidget, tr("Quest"));

    layout->addWidget(calibrationLabel);
    layout->addWidget(startRecordingButton);
    layout->addWidget(tabWidget);

    mainWidget->setLayout(layout);

    connect(startRecordingButton,SIGNAL(clicked()),this,SLOT(onClickStartRecordingButton()));
}

void MainWindow::onClickStartRecordingButton()
{
    if(!recording)
    {
        record_folder = "output/"+getCurrentDateTimeStr();
        QDir().mkdir(record_folder.c_str());
        recording = true;
        startRecordingButton->setText("stop recording");
    } else {
        recording = false;
        setPostProcessingPage();
        delete videoInput;
        delete questInput;
        camPreviewWidget = NULL;
        questPreviewWidget = NULL;
        postProcessingThread = new std::thread([&]()
        {
            postProcessThreadFunc();
        });
    }
}
