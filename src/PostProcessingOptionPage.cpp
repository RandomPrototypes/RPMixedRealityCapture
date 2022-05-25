#include "mainwindow.h"
#include "PostProcessingOptionPage.h"
#include <RPCameraInterface/VideoEncoder.h>
#include <RPCameraInterface/OpenCVConverter.h>
#include <QFileDialog>
#include <libQuestMR/BackgroundSubtractor.h>
#include <QSpinBox>
#include <QColorDialog>
#include <QCheckBox>
#include <QMessageBox>
#include <QStyle>
#include <QToolButton>
#include <fstream>
#include <QDebug>

PostProcessingOptionPage::PostProcessingOptionPage(MainWindow *win)
    :win(win)
{
}

void PostProcessingOptionPage::setPage()
{
    win->currentPageName = MainWindow::PageName::postProcessingOption;
    win->clearMainWidget();

    state = PostProcessingState::previewPause;

    layout = new QGridLayout();
    layout->setContentsMargins(10,50,10,50);

    QLabel *postProcessingLabel = new QLabel;
    postProcessingLabel->setText("Post processing: ");

    QLabel *questRecordingFileLabel = new QLabel;
    questRecordingFileLabel->setText("Quest recording file: ");

    questRecordingFileEdit = new QLineEdit();

    QPushButton *questRecordingFileBrowseButton = new QPushButton();
    questRecordingFileBrowseButton->setText("browse");

    QLabel *camRecordingFileLabel = new QLabel;
    camRecordingFileLabel->setText("Camera recording file: ");

    camRecordingFileEdit = new QLineEdit();


    QPushButton *camRecordingFileBrowseButton = new QPushButton();
    camRecordingFileBrowseButton->setText("browse");

    QLabel *backgroundSubtractorLabel = new QLabel;
    backgroundSubtractorLabel->setText("background subtractor method:");
    listBackgroundSubtractorCombo = new QComboBox;
    for(int i = 0; i < libQuestMR::getBackgroundSubtractorCount(); i++)
        listBackgroundSubtractorCombo->addItem(QString(libQuestMR::getBackgroundSubtractorName(i).c_str()));

    QHBoxLayout *hlayout1 = new QHBoxLayout();

    QPushButton *selectPlayAreaButton = new QPushButton();
    selectPlayAreaButton->setText("select play area");

    //QPushButton *playButton = new QPushButton();
    //playButton->setText("play");

    playButton = new QToolButton();
    playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));

    QToolButton *stopButton = new QToolButton();
    stopButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaStop));

    durationLabel = new QLabel();
    durationLabel->setText("00:00 / 00:00");
    durationLabel->setMaximumWidth(100);

    QPushButton *startEncodingButton = new QPushButton();
    startEncodingButton->setText("start encoding");

    hlayout1->addWidget(selectPlayAreaButton);
    hlayout1->addWidget(startEncodingButton);

    backgroundSubtractorOptionLayout = new QGridLayout();

    QLabel *previewLabel = new QLabel;
    previewLabel->setText("preview");

    QHBoxLayout *preview_checkbox_layout = new QHBoxLayout();
    camImgCheckbox = new QCheckBox;
    camImgCheckbox->setText("camera img");
    camImgCheckbox->setChecked(true);

    questImgCheckbox = new QCheckBox;
    questImgCheckbox->setText("quest img");

    matteImgCheckbox = new QCheckBox;
    matteImgCheckbox->setText("matte");

    greenBackgroundCheckbox = new QCheckBox;
    greenBackgroundCheckbox->setText("green background");

    blackBackgroundCheckbox = new QCheckBox;
    blackBackgroundCheckbox->setText("black background");

    preview_checkbox_layout->addWidget(camImgCheckbox);
    preview_checkbox_layout->addWidget(questImgCheckbox);
    preview_checkbox_layout->addWidget(matteImgCheckbox);
    preview_checkbox_layout->addWidget(greenBackgroundCheckbox);
    preview_checkbox_layout->addWidget(blackBackgroundCheckbox);


    win->postProcessPreviewWidget = new OpenCVWidget(cv::Size(960, 540));

    win->postProcessPreviewWidget->setImg(cv::Mat());

    QHBoxLayout *bottom_hlayout = new QHBoxLayout();
    bottom_hlayout->addWidget(playButton);
    bottom_hlayout->addWidget(stopButton);
    bottom_hlayout->addWidget(durationLabel);

    layout->addWidget(postProcessingLabel, 0, 0);

    layout->addWidget(questRecordingFileLabel, 1, 0);
    layout->addWidget(questRecordingFileEdit, 1, 1);
    layout->addWidget(questRecordingFileBrowseButton, 1, 2);

    layout->addWidget(camRecordingFileLabel, 2, 0);
    layout->addWidget(camRecordingFileEdit, 2, 1);
    layout->addWidget(camRecordingFileBrowseButton, 2, 2);

    layout->addWidget(backgroundSubtractorLabel, 3, 0);
    layout->addWidget(listBackgroundSubtractorCombo, 3, 1);

    layout->addLayout(hlayout1, 4, 0, 1, 3);

    layout->addWidget(previewLabel, 5, 0);
    layout->addLayout(preview_checkbox_layout, 5, 1);

    layout->addWidget(win->postProcessPreviewWidget, 6, 0, 3, 3);

    layout->addLayout(backgroundSubtractorOptionLayout, 6, 3, 3, 3);

    layout->addLayout(bottom_hlayout, 10, 0, 1, 3);
    win->mainWidget->setLayout(layout);

    connect(questRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickQuestRecordingFileBrowseButton()));
    connect(camRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickCamRecordingFileBrowseButton()));
    connect(listBackgroundSubtractorCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectBackgroundSubtractorCombo(int)));

    connect(camImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickCamImgCheckbox()));
    connect(questImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickQuestImgCheckbox()));
    connect(matteImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickMatteImgCheckbox()));
    connect(greenBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickGreenBackgroundCheckbox()));
    connect(blackBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickBlackBackgroundCheckbox()));

    connect(playButton,SIGNAL(clicked()),this,SLOT(onClickPlayButton()));
    connect(stopButton,SIGNAL(clicked()),this,SLOT(onClickStopButton()));

    refreshBackgroundSubtractorOption();
}

void PostProcessingOptionPage::refreshBackgroundSubtractorOption()
{
    clearLayout(backgroundSubtractorOptionLayout);
    int bgMethodId = listBackgroundSubtractorCombo->currentIndex();
    if(bgMethodId < 0)
        return ;
    backgroundSubtractor = libQuestMR::createBackgroundSubtractor(bgMethodId);
    for(int i = 0; i < backgroundSubtractor->getParameterCount(); i++) {
        if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeInt) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QSpinBox *spin = new QSpinBox();
            spin->setValue(backgroundSubtractor->getParameterValAsInt(i));
            backgroundSubtractorOptionLayout->addWidget(label, i, 0);
            backgroundSubtractorOptionLayout->addWidget(spin, i, 1);
        } else if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeBool) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setChecked(backgroundSubtractor->getParameterValAsBool(i));
            backgroundSubtractorOptionLayout->addWidget(label, i, 0);
            backgroundSubtractorOptionLayout->addWidget(checkbox, i, 1);
        } else if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeColor) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QHBoxLayout *hlayout = new QHBoxLayout();
            QLineEdit *lineEdit = new QLineEdit();
            lineEdit->setText(QString(backgroundSubtractor->getParameterVal(i).str().c_str()));
            hlayout->addWidget(lineEdit);
            QPushButton *button = new QPushButton;
            button->setText("select color");
            hlayout->addWidget(button);
            backgroundSubtractorOptionLayout->addWidget(label, i, 0);
            backgroundSubtractorOptionLayout->addLayout(hlayout, i, 1);
            connect(button,&QPushButton::clicked,[=](){
                unsigned char r,g,b;
                backgroundSubtractor->getParameterValAsRGB(i, &r, &g, &b);
                QColor newColor = QColorDialog::getColor(QColor(r,g,b), NULL, QString(), QColorDialog::DontUseNativeDialog);
                int r2, g2, b2;
                newColor.getRgb(&r2, &g2, &b2);
                backgroundSubtractor->setParameterValRGB(i, r2, g2, b2);
                lineEdit->setText(QString(backgroundSubtractor->getParameterVal(i).str().c_str()));
            });
        }
    }
}



void PostProcessingOptionPage::onTimer()
{
    if(state == PostProcessingState::previewPlay)
    {
        uint64_t timestamp = libQuestMR::getTimestampMs() - startPlayTimestamp + listCameraTimestamp[0];
        readCameraFrame(timestamp);
        updatePreviewImg();
    }
}

uint64_t absdiff(uint64_t t1, uint64_t t2)
{
    return t2 > t1 ? t2-t1 : t1-t2;
}

void PostProcessingOptionPage::readCameraFrame(uint64_t timestamp)
{
    while(cameraFrameId+1 < listCameraTimestamp.size() && absdiff(timestamp, listCameraTimestamp[cameraFrameId+1]) < absdiff(timestamp, listCameraTimestamp[cameraFrameId]))
    {
        (*capCameraVid) >> currentFrameCam;
        cameraFrameId++;
    }
    if(questVideoMngr != NULL && questVideoSrc->isValid()) {
        qDebug() << "videoTickImpl";
        uint64_t quest_timestamp;
        currentFrameQuest = questVideoMngr->getMostRecentImg(&quest_timestamp);
        while(questVideoSrc->isValid() && quest_timestamp < listCameraTimestamp[cameraFrameId]) {
            questVideoMngr->VideoTickImpl();
            currentFrameQuest = questVideoMngr->getMostRecentImg(&quest_timestamp);
        }
    }
    int currentTime = (int)(listCameraTimestamp[cameraFrameId] - listCameraTimestamp[0])/1000;
    int duration = (int)(listCameraTimestamp[listCameraTimestamp.size()-1] - listCameraTimestamp[0])/1000;
    char txt[255];
    sprintf(txt, "%02d:%02d / %02d:%02d", currentTime/60, currentTime%60, duration/60, duration%60);
    durationLabel->setText(txt);
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool PostProcessingOptionPage::loadCameraTimestamps(std::string filename)
{
    listCameraTimestamp.clear();
    std::ifstream timestampFile(filename);
    std::string timestampStr;

    if(!timestampFile.good())
        return false;

    while(std::getline(timestampFile, timestampStr))
    {
        uint64_t timestamp;
        std::istringstream iss(timestampStr);
        iss >> timestamp;
        listCameraTimestamp.push_back(timestamp);
    }
    return true;
}


void PostProcessingOptionPage::updateRecordingFile()
{
    if(capCameraVid != NULL) {
        capCameraVid->release();
        capCameraVid = NULL;
    }
    std::string camFilename = camRecordingFileEdit->text().toStdString();
    std::string questFilename = questRecordingFileEdit->text().toStdString();
    if(!endsWith(camFilename, "_cam.mp4")) {
        if(camFilename.size() > 0) {
            QMessageBox msgBox;
            msgBox.setText("camera files should ends with _cam.mp4");
            msgBox.exec();
        }
        return ;
    }
    if(questFilename.size() > 0 && !endsWith(questFilename, "_quest.questMRVideo"))
    {
        QMessageBox msgBox;
        msgBox.setText("quest files should ends with _quest.questMRVideo");
        msgBox.exec();
        questFilename = "";
        questRecordingFileEdit->setText("");
    }
    std::string camTimestampFilename = camFilename.substr(0, camFilename.size()-7)+"timestamp.txt";
    if(!loadCameraTimestamps(camTimestampFilename)) {
        QMessageBox msgBox;
        msgBox.setText(("can not load camera timestamps '"+camTimestampFilename+"'").c_str());
        msgBox.exec();
        return ;
    }

    capCameraVid = std::shared_ptr<cv::VideoCapture>(new cv::VideoCapture(camFilename));
    if(!capCameraVid->isOpened()) {
        QMessageBox msgBox;
        msgBox.setText("Can not connect to the quest...");
        msgBox.exec();
        capCameraVid = NULL;
        return ;
    }
    if(questFilename.size() > 0) {
        if(questVideoMngr != NULL) {
            questVideoMngr->detachSource();
            questVideoSrc->close();
            questVideoMngr = NULL;
            questVideoSrc = NULL;
        }
        questVideoMngr = libQuestMR::createQuestVideoMngr();
        questVideoSrc = libQuestMR::createQuestVideoSourceFile();
        questVideoSrc->open(questFilename.c_str());
        questVideoMngr->attachSource(questVideoSrc);
        std::string questTimestampFilename = questFilename.substr(0, questFilename.size()-19)+"_questTimestamp.txt";
        qDebug() << questTimestampFilename.c_str();
        questVideoMngr->setRecordedTimestampFile(questTimestampFilename.c_str());
        currentFrameQuest = cv::Mat();
        while(questVideoSrc->isValid() && currentFrameQuest.cols < 100) {
            qDebug() << "videoTickImpl";
            questVideoMngr->VideoTickImpl();
            uint64_t quest_timestamp;
            currentFrameQuest = questVideoMngr->getMostRecentImg(&quest_timestamp);
        }
    }
    cameraFrameId = 0;
    questFrameId = 0;
    (*capCameraVid) >> currentFrameCam;
    startPlayTimestamp = libQuestMR::getTimestampMs();
    readCameraFrame(listCameraTimestamp[0]);
    updatePreviewImg();
}


void PostProcessingOptionPage::onClickQuestRecordingFileBrowseButton()
{
    QFileDialog dialog(NULL);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("quest recording file (*.questMRVideo)"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        if(filenames.size() == 1)
        {
            questRecordingFileEdit->setText(filenames[0]);

            std::string questFilename = filenames[0].toStdString();
            if(camRecordingFileEdit->text().size() == 0 && endsWith(questFilename, "_quest.questMRVideo")) {
                std::string camFilename = questFilename.substr(0, questFilename.size() - 19) + "_cam.mp4";
                if(std::ifstream(camFilename).good())
                    camRecordingFileEdit->setText(camFilename.c_str());
            }
            updateRecordingFile();
        }
    }
}

void PostProcessingOptionPage::onClickCamRecordingFileBrowseButton()
{
    QFileDialog dialog(NULL);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("camera recording file (*.mp4)"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        if(filenames.size() == 1)
        {
            camRecordingFileEdit->setText(filenames[0]);

            std::string camFilename = filenames[0].toStdString();
            if(questRecordingFileEdit->text().size() == 0 && endsWith(camFilename, "_cam.mp4")) {
                std::string questFilename = camFilename.substr(0, camFilename.size() - 8) + "_quest.questMRVideo";
                if(std::ifstream(questFilename).good())
                    questRecordingFileEdit->setText(questFilename.c_str());
            }
            updateRecordingFile();
        }
    }
}

cv::Mat alphaBlendingMat(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& alphaMask)
{
    cv::Mat result = cv::Mat(img1.size(), CV_8UC3);
    for(int i = 0; i < result.rows; i++)
    {
        unsigned char *dst = result.ptr<unsigned char>(i);
        const unsigned char *src1 = img1.ptr<unsigned char>(i);
        const unsigned char *src2 = img2.ptr<unsigned char>(i);
        const unsigned char *alphaPtr = alphaMask.ptr<unsigned char>(i);
        for(int j = 0; j < result.cols; j++)
        {
            unsigned short alpha = *alphaPtr;
            unsigned short beta = 255 - alpha;
            for(int k = 3; k > 0; k--) {
                *dst++ = (alpha * (*src1++) + beta * (*src2++))/255;
            }
            alphaPtr++;
        }
    }
    return result;
}

void PostProcessingOptionPage::updatePreviewImg()
{
    if(cameraFrameId == 0 && backgroundSubtractor != NULL)
        backgroundSubtractor->restart();
    cv::Size size(1280,720);
    if(!currentFrameCam.empty())
        size = currentFrameCam.size();
    cv::Mat background;
    cv::Mat middleImg;
    if(greenBackgroundCheckbox->isChecked()) {
        background = cv::Mat(size, CV_8UC3);
        background.setTo(cv::Scalar(0,255,0));
    } else if(blackBackgroundCheckbox->isChecked()) {
        background = cv::Mat(size, CV_8UC3);
        background.setTo(cv::Scalar(0,0,0));
    } else if(questImgCheckbox->isChecked()) {
        background = currentFrameQuest(cv::Rect(0,0,currentFrameQuest.cols/2,currentFrameQuest.rows));
        cv::resize(background, background, size);
    }
    cv::Mat fgmask;
    if(camImgCheckbox->isChecked() || matteImgCheckbox->isChecked()) {
        if(!currentFrameCam.empty()) {
            if(!background.empty() || matteImgCheckbox->isChecked())
                backgroundSubtractor->apply(currentFrameCam, fgmask);

            if(matteImgCheckbox->isChecked()) {
                cv::cvtColor(fgmask, middleImg, cv::COLOR_GRAY2BGR);
                background = cv::Mat();
            } else {
                middleImg = currentFrameCam.clone();
            }
        }
    }
    cv::Mat result = background;
    if(!middleImg.empty()) {
        if(!fgmask.empty() && !background.empty())
            result = alphaBlendingMat(middleImg, background, fgmask);
        else result = middleImg;
    }
    win->postProcessPreviewWidget->setImg(result);
}

void PostProcessingOptionPage::onClickCamImgCheckbox()
{
    if(camImgCheckbox->isChecked())
        matteImgCheckbox->setChecked(false);
    updatePreviewImg();
}
void PostProcessingOptionPage::onClickQuestImgCheckbox()
{
    updatePreviewImg();
}
void PostProcessingOptionPage::onClickMatteImgCheckbox()
{
    if(matteImgCheckbox->isChecked())
        camImgCheckbox->setChecked(false);
    updatePreviewImg();
}
void PostProcessingOptionPage::onClickGreenBackgroundCheckbox()
{
    if(greenBackgroundCheckbox->isChecked())
       blackBackgroundCheckbox->setChecked(false);
    updatePreviewImg();
}
void PostProcessingOptionPage::onClickBlackBackgroundCheckbox()
{
    if(blackBackgroundCheckbox->isChecked())
       greenBackgroundCheckbox->setChecked(false);
    updatePreviewImg();
}

void PostProcessingOptionPage::onSelectBackgroundSubtractorCombo(int id)
{
    refreshBackgroundSubtractorOption();
}

void PostProcessingOptionPage::onClickPlayButton()
{
    if(listCameraTimestamp.size() > 0)
    {
        if(state == PostProcessingState::previewPlay) {
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
            state = PostProcessingState::previewPause;
        } else {
            state = PostProcessingState::previewPlay;
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPause));
            startPlayTimestamp = libQuestMR::getTimestampMs() - (listCameraTimestamp[cameraFrameId]- listCameraTimestamp[0]);
        }
    }
}

void PostProcessingOptionPage::onClickStopButton()
{
    playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
    state = PostProcessingState::previewPause;
    updateRecordingFile();
}
