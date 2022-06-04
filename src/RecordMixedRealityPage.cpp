#include "mainwindow.h"
#include "RecordMixedRealityPage.h"
#include "PostProcessingPage.h"
#include "PostProcessingOptionPage.h"
#include <QDir>
#include <QFileDialog>

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

    win->startCamera();
    win->startQuestRecorder();
}

void RecordMixedRealityPage::onClickStartRecordingButton()
{
    if(!win->recording)
    {
        QFileDialog dialog(NULL);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setNameFilter(tr("quest recording file (*.questMRVideo)"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);

        if (dialog.exec())
        {
            QStringList filenames = dialog.selectedFiles();
            if(filenames.size() == 1)
            {
                std::string filename = filenames[0].toStdString();
                if(!endsWith(filename, ".questMRVideo"))
                    filename += ".questMRVideo";
                QFileInfo fi(filename.c_str());
                qDebug() << fi.absolutePath() << " fn=" << fi.fileName();
                win->record_folder = fi.absolutePath().toStdString();
                filename = fi.fileName().toStdString();
                win->record_name = filename.substr(0, filename.size() - 13);
                qDebug() << win->record_name.c_str();
                //win->record_folder = "output/"+getCurrentDateTimeStr();
                //QDir().mkdir(win->record_folder.c_str());
                win->recording = true;
                startRecordingButton->setText("stop recording");
            }
        }
    } else {
        win->recording = false;
        while(!win->recording_finished)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        win->stopCamera();
        win->stopQuestRecorder();

        win->postProcessingOptionPage->setPage();
        win->postProcessingOptionPage->setQuestRecordingFilename(win->record_folder+"/"+win->record_name+".questMRVideo");
    }
}

void RecordMixedRealityPage::onTimer()
{
    if(win->videoInput->hasNewImg)
    {
        cv::Mat img = win->videoInput->getImgCopy();
        qDebug() << "cam " << img.cols << " " << img.rows;
        win->camPreviewWidget->setImg(img);
    }
    if(win->questInput->hasNewImg && win->questPreviewWidget != NULL)
    {
        cv::Mat img = win->questInput->getImgCopy();
        qDebug() << "quest " << img.cols << " " << img.rows;
        win->questPreviewWidget->setImg(img(cv::Rect(0,0,img.cols/2,img.rows)));
    }
}

