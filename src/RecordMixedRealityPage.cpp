#include "mainwindow.h"
#include "RecordMixedRealityPage.h"
#include "PostProcessingPage.h"
#include <QDir>

RecordMixedRealityPage::RecordMixedRealityPage(MainWindow *win)
    :win(win)
{
}

void RecordMixedRealityPage::setPage()
{
    win->currentPageName = MainWindow::PageName::recordMixedReality;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("record mixed reality: ");

    startRecordingButton = new QPushButton("start recording");
    startRecordingButton->setMaximumWidth(300);

    QTabWidget *tabWidget = new QTabWidget;

    win->camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->camPreviewWidget->setImg(cv::Mat());

    win->questPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->questPreviewWidget->setImg(cv::Mat());

    tabWidget->addTab(win->camPreviewWidget, tr("Camera"));
    tabWidget->addTab(win->questPreviewWidget, tr("Quest"));

    layout->addWidget(calibrationLabel);
    layout->addWidget(startRecordingButton);
    layout->addWidget(tabWidget);

    win->mainWidget->setLayout(layout);

    connect(startRecordingButton,SIGNAL(clicked()),this,SLOT(onClickStartRecordingButton()));

    win->videoInput->videoThread = new std::thread([&]()
        {
            win->videoThreadFunc(win->cameraId);
        }
    );

    win->questInput->videoThread = new std::thread([&]()
        {
            win->questThreadFunc();
        }
    );
}

void RecordMixedRealityPage::onClickStartRecordingButton()
{
    if(!win->recording)
    {
        win->record_folder = "output/"+getCurrentDateTimeStr();
        QDir().mkdir(win->record_folder.c_str());
        win->recording = true;
        startRecordingButton->setText("stop recording");
    } else {
        win->recording = false;
        win->postProcessingPage->setPage();
        delete win->videoInput;
        delete win->questInput;
        win->camPreviewWidget = NULL;
        win->questPreviewWidget = NULL;
        win->postProcessingThread = new std::thread([&]()
        {
            win->postProcessingPage->postProcessThreadFunc();
        });
    }
}

void RecordMixedRealityPage::onTimer()
{
    if(win->videoInput->hasNewImg)
    {
        cv::Mat img = win->videoInput->getImgCopy();
        win->camPreviewWidget->setImg(img);
    }
}

