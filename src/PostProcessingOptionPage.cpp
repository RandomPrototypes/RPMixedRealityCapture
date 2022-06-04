#include "mainwindow.h"
#include "PostProcessingOptionPage.h"
#include "RecordMixedRealityPage.h"
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

void PostProcessingOptionPage::setPage(bool isLivePreview)
{
    this->isLivePreview = isLivePreview;
    videoSize = cv::Size(1280,720);
    win->currentPageName = MainWindow::PageName::postProcessingOption;
    win->clearMainWidget();

    state = PostProcessingState::previewPause;
    selectShapeState = SelectShapeState::noSelection;

    layout = new QGridLayout();
    layout->setContentsMargins(10,50,10,50);

    QLabel *postProcessingLabel = new QLabel;

    if(isLivePreview) {
        postProcessingLabel->setText("Preview settings: ");
        playButton = NULL;
        durationLabel = NULL;
        startEncodingButton = NULL;
        QPushButton *savePreviewSettingButton = new QPushButton();
        savePreviewSettingButton->setText("save preview settings");
        layout->addWidget(savePreviewSettingButton, 10, 0);
        connect(savePreviewSettingButton,SIGNAL(clicked()),this,SLOT(onClickSavePreviewSettingButton()));
        win->startCamera();
    } else {
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

        layout->addWidget(questRecordingFileLabel, 1, 0);
        layout->addWidget(questRecordingFileEdit, 1, 1);
        layout->addWidget(questRecordingFileBrowseButton, 1, 2);

        layout->addWidget(camRecordingFileLabel, 2, 0);
        layout->addWidget(camRecordingFileEdit, 2, 1);
        layout->addWidget(camRecordingFileBrowseButton, 2, 2);

        connect(questRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickQuestRecordingFileBrowseButton()));
        connect(camRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickCamRecordingFileBrowseButton()));



        startEncodingButton = new QPushButton();
        startEncodingButton->setText("start encoding");
        startEncodingButton->setMaximumWidth(200);

        playButton = new QToolButton();
        playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));

        QToolButton *stopButton = new QToolButton();
        stopButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaStop));

        durationLabel = new QLabel();
        durationLabel->setText("00:00 / 00:00");
        durationLabel->setMaximumWidth(100);

        QHBoxLayout *bottom_hlayout = new QHBoxLayout();
        bottom_hlayout->addWidget(startEncodingButton);
        bottom_hlayout->addWidget(playButton);
        bottom_hlayout->addWidget(stopButton);
        bottom_hlayout->addWidget(durationLabel);
        layout->addLayout(bottom_hlayout, 10, 0, 1, 3);
        connect(playButton,SIGNAL(clicked()),this,SLOT(onClickPlayButton()));
        connect(stopButton,SIGNAL(clicked()),this,SLOT(onClickStopButton()));
        connect(startEncodingButton,SIGNAL(clicked()),this,SLOT(onClickStartEncodingButton()));
    }

    QLabel *backgroundSubtractorLabel = new QLabel;
    backgroundSubtractorLabel->setText("background subtractor method:");
    listBackgroundSubtractorCombo = new QComboBox;
    for(int i = 0; i < libQuestMR::getBackgroundSubtractorCount(); i++)
        listBackgroundSubtractorCombo->addItem(QString(libQuestMR::getBackgroundSubtractorName(i).c_str()));

    QHBoxLayout *hlayout1 = new QHBoxLayout();

    selectPlayAreaButton = new QPushButton();
    selectPlayAreaButton->setText("select play area");

    hlayout1->addWidget(selectPlayAreaButton);
    //hlayout1->addWidget(startEncodingButton);

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

    preview_checkbox_layout->addWidget(previewLabel);
    preview_checkbox_layout->addWidget(camImgCheckbox);
    preview_checkbox_layout->addWidget(questImgCheckbox);
    preview_checkbox_layout->addWidget(matteImgCheckbox);
    preview_checkbox_layout->addWidget(greenBackgroundCheckbox);
    preview_checkbox_layout->addWidget(blackBackgroundCheckbox);


    win->postProcessPreviewWidget = new OpenCVWidget(cv::Size(960, 540));

    win->postProcessPreviewWidget->setImg(cv::Mat());

    layout->addWidget(postProcessingLabel, 0, 0);

    layout->addWidget(backgroundSubtractorLabel, 3, 0);
    layout->addWidget(listBackgroundSubtractorCombo, 3, 1);

    layout->addLayout(hlayout1, 4, 0, 1, 3);

    layout->addLayout(preview_checkbox_layout, 5, 0, 1, 2);

    layout->addWidget(win->postProcessPreviewWidget, 6, 0, 3, 3);

    layout->addLayout(backgroundSubtractorOptionLayout, 6, 3, 3, 3);

    win->mainWidget->setLayout(layout);

    connect(listBackgroundSubtractorCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectBackgroundSubtractorCombo(int)));

    connect(camImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickCamImgCheckbox()));
    connect(questImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickQuestImgCheckbox()));
    connect(matteImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickMatteImgCheckbox()));
    connect(greenBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickGreenBackgroundCheckbox()));
    connect(blackBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickBlackBackgroundCheckbox()));

    connect(selectPlayAreaButton,SIGNAL(clicked()),this,SLOT(onClickSelectPlayAreaButton()));
    connect(win->postProcessPreviewWidget,SIGNAL(clicked()),this,SLOT(onClickPreviewWidget()));


    refreshBackgroundSubtractorOption();
}

void PostProcessingOptionPage::setQuestRecordingFilename(std::string questFilename)
{
    questRecordingFileEdit->setText(questFilename.c_str());
    if(camRecordingFileEdit->text().size() == 0 && endsWith(questFilename, ".questMRVideo")) {
        std::string camFilename = questFilename.substr(0, questFilename.size() - 13) + "_cam.mp4";
        if(std::ifstream(camFilename).good())
            camRecordingFileEdit->setText(camFilename.c_str());
    }
    updateRecordingFile();
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
            connect(spin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
                backgroundSubtractor->setParameterVal(i, val);
            });
        } else if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeBool) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setChecked(backgroundSubtractor->getParameterValAsBool(i));
            backgroundSubtractorOptionLayout->addWidget(label, i, 0);
            backgroundSubtractorOptionLayout->addWidget(checkbox, i, 1);
            connect(checkbox,&QCheckBox::clicked,[=](){
                backgroundSubtractor->setParameterVal(i, checkbox->isEnabled());
            });
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
    if(isLivePreview) {
        if(win->videoInput->hasNewImg)
            currentFrameCam = win->videoInput->getImgCopy();
        updatePreviewImg();
    } else {
        if(state == PostProcessingState::previewPlay || state == PostProcessingState::previewPause) {
            if(state == PostProcessingState::previewPlay) {
                uint64_t timestamp = libQuestMR::getTimestampMs() - startPlayTimestamp + listCameraTimestamp[0];
                readCameraFrame(timestamp);
            }
            updatePreviewImg();
        } else if(state == PostProcessingState::encodingStarted) {
            updatePreviewImg();
            updateDurationLabel();
        } else if(state == PostProcessingState::encodingStopped || state == PostProcessingState::encodingFinished) {
            QMessageBox msgBox;
            if(state == PostProcessingState::encodingStopped)
                msgBox.setText("encoding stopped!!");
            else msgBox.setText("encoding finished!!");
            msgBox.exec();
            state = PostProcessingState::previewPause;
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
            updateRecordingFile();
        }
    }
}

uint64_t absdiff(uint64_t t1, uint64_t t2)
{
    return t2 > t1 ? t2-t1 : t1-t2;
}

void PostProcessingOptionPage::updateDurationLabel()
{
    if(!isLivePreview)
    {
        int currentTime = (int)(listCameraTimestamp[cameraFrameId] - listCameraTimestamp[0])/1000;
        int duration = (int)(listCameraTimestamp[listCameraTimestamp.size()-1] - listCameraTimestamp[0])/1000;
        char txt[255];
        sprintf(txt, "%02d:%02d / %02d:%02d", currentTime/60, currentTime%60, duration/60, duration%60);
        durationLabel->setText(txt);
    }
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
    updateDurationLabel();
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

bool PostProcessingOptionPage::loadCameraRecordingFile()
{
    if(capCameraVid != NULL) {
        capCameraVid->release();
        capCameraVid = NULL;
    }
    cameraFrameId = 0;

    std::string camFilename = camRecordingFileEdit->text().toStdString();
    if(!endsWith(camFilename, "_cam.mp4")) {
        if(camFilename.size() > 0) {
            QMessageBox msgBox;
            msgBox.setText("camera files should ends with _cam.mp4");
            msgBox.exec();
        }
        return false;
    }
    std::string camTimestampFilename = camFilename.substr(0, camFilename.size()-4)+"Timestamp.txt";
    if(!loadCameraTimestamps(camTimestampFilename)) {
        QMessageBox msgBox;
        msgBox.setText(("can not load camera timestamps '"+camTimestampFilename+"'").c_str());
        msgBox.exec();
        return false;
    }
    capCameraVid = std::shared_ptr<cv::VideoCapture>(new cv::VideoCapture(camFilename));
    if(!capCameraVid->isOpened()) {
        QMessageBox msgBox;
        msgBox.setText("Can not connect to the quest...");
        msgBox.exec();
        capCameraVid = NULL;
        return false;
    }
    return true;
}

bool PostProcessingOptionPage::loadQuestRecordingFile()
{
    if(questVideoMngr != NULL) {
        questVideoMngr->detachSource();
        questVideoSrc->close();
        questVideoMngr = NULL;
        questVideoSrc = NULL;
    }
    currentFrameQuest = cv::Mat();
    questFrameId = 0;

    std::string questFilename = questRecordingFileEdit->text().toStdString();
    if(questFilename.size() > 0 && !endsWith(questFilename, ".questMRVideo"))
    {
        QMessageBox msgBox;
        msgBox.setText("quest files should ends with .questMRVideo");
        msgBox.exec();
        questFilename = "";
        questRecordingFileEdit->setText("");
        return false;
    }
    if(questFilename.size() == 0)
        return false;
    questVideoMngr = libQuestMR::createQuestVideoMngr();
    questVideoSrc = libQuestMR::createQuestVideoSourceFile();
    questVideoSrc->open(questFilename.c_str());
    questVideoMngr->attachSource(questVideoSrc);
    std::string questTimestampFilename = questFilename.substr(0, questFilename.size()-13)+"_questTimestamp.txt";
    qDebug() << questTimestampFilename.c_str();
    questVideoMngr->setRecordedTimestampFile(questTimestampFilename.c_str());

    return true;
}

void PostProcessingOptionPage::updateRecordingFile()
{
    bool camValid = loadCameraRecordingFile();
    bool questValid = loadQuestRecordingFile();

    if(!camValid || !questValid)
        return ;

    while(questVideoSrc->isValid() && currentFrameQuest.cols < 100) {
        qDebug() << "videoTickImpl";
        questVideoMngr->VideoTickImpl();
        uint64_t quest_timestamp;
        currentFrameQuest = questVideoMngr->getMostRecentImg(&quest_timestamp);
    }

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
            std::string questFilename = filenames[0].toStdString();
            setQuestRecordingFilename(questFilename);
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
                std::string questFilename = camFilename.substr(0, camFilename.size() - 8) + ".questMRVideo";
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
    if(state == PostProcessingState::encodingStarted)
    {
        cv::Mat img;
        encodingMutex.lock();
        img = encodedFrame.clone();
        encodingMutex.unlock();
        win->postProcessPreviewWidget->setImg(img);
        return ;
    }
    if(cameraFrameId == 0 && backgroundSubtractor != NULL)
        backgroundSubtractor->restart();
    if(!currentFrameCam.empty())
        videoSize = currentFrameCam.size();
    if(playAreaMask.size() != videoSize)
        updatePlayArea();
    cv::Mat background;
    cv::Mat middleImg;
    if(greenBackgroundCheckbox->isChecked()) {
        background = cv::Mat(videoSize, CV_8UC3);
        background.setTo(cv::Scalar(0,255,0));
    } else if(blackBackgroundCheckbox->isChecked()) {
        background = cv::Mat(videoSize, CV_8UC3);
        background.setTo(cv::Scalar(0,0,0));
    } else if(questImgCheckbox->isChecked()) {
        background = currentFrameQuest(cv::Rect(0,0,currentFrameQuest.cols/2,currentFrameQuest.rows));
        cv::resize(background, background, videoSize);
    }
    cv::Mat fgmask;
    if(camImgCheckbox->isChecked() || matteImgCheckbox->isChecked()) {
        if(!currentFrameCam.empty()) {
            if(!background.empty() || matteImgCheckbox->isChecked()) {
                backgroundSubtractor->setROI(playAreaROI);
                backgroundSubtractor->apply(currentFrameCam, fgmask);
                cv::bitwise_and(fgmask, playAreaMask, fgmask);
            } else {
                fgmask = playAreaMask.clone();
            }

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
    if(selectShapeState == SelectShapeState::selecting)
    {
        for(size_t i = 0; i < playAreaShape.size(); i++)
            cv::circle(result, playAreaShape[i], 5, cv::Scalar(0,0,255), 2);
        for(size_t i = 0; i+1 < playAreaShape.size(); i++)
            cv::line(result, playAreaShape[i], playAreaShape[i+1], cv::Scalar(0,0,255), 2);
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
        if(state == PostProcessingState::encodingStarted) {
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
            state = PostProcessingState::encodingPaused;
        } else if(state == PostProcessingState::encodingPaused) {
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPause));
            state = PostProcessingState::encodingStarted;
        } else if(state == PostProcessingState::previewPlay) {
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
            state = PostProcessingState::previewPause;
        } else {
            state = PostProcessingState::previewPlay;
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPause));
            if(listCameraTimestamp.size() > 0)
                startPlayTimestamp = libQuestMR::getTimestampMs() - (listCameraTimestamp[cameraFrameId]- listCameraTimestamp[0]);
        }
    }
}

void PostProcessingOptionPage::onClickStopButton()
{
    playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPlay));
    if(state == PostProcessingState::encodingStarted) {
        state = PostProcessingState::encodingStopped;
    } else {
        state = PostProcessingState::previewPause;
        updateRecordingFile();
    }
}

void PostProcessingOptionPage::onClickSelectPlayAreaButton()
{
    if(selectShapeState == SelectShapeState::noSelection) {
        selectShapeState = SelectShapeState::selecting;
        win->postProcessPreviewWidget->drawCursor = true;
        playAreaShape.clear();
        selectPlayAreaButton->setText("finish the selection");
    } else {
        selectShapeState = SelectShapeState::noSelection;
        win->postProcessPreviewWidget->drawCursor = false;
        updatePlayArea();
        selectPlayAreaButton->setText("select play area");
    }
    updatePreviewImg();
}

void PostProcessingOptionPage::updatePlayArea()
{
    playAreaMask = cv::Mat(videoSize, CV_8UC1);
    if(playAreaShape.size() < 3)
    {
        playAreaROI = cv::Rect(0,0,videoSize.width,videoSize.height);
        playAreaMask.setTo(cv::Scalar(255));
    } else {
        playAreaMask.setTo(cv::Scalar(0));
        std::vector<cv::Point> list(playAreaShape.size());
        for(size_t i = 0; i < playAreaShape.size(); i++)
            list[i] = cv::Point(playAreaShape[i]);
        playAreaROI = cv::boundingRect(list);

        const cv::Point* ppt[1] = { &list[0] };
        int npt[] = { (int)list.size() };
        cv::fillPoly(playAreaMask, ppt, npt, 1, cv::Scalar( 255 ), cv::LINE_8);
        qDebug() << "updated mask";
    }
    if(backgroundSubtractor != NULL) {
        backgroundSubtractor->setROI(playAreaROI);
        backgroundSubtractor->restart();
    }
}

void PostProcessingOptionPage::onClickPreviewWidget()
{
    if(selectShapeState == SelectShapeState::selecting)
    {
        float dist = 10000;
        if(playAreaShape.size() > 0) {
            cv::Point2d diff = win->postProcessPreviewWidget->mousePos - playAreaShape[0];
            dist = sqrt(diff.dot(diff));
        }
        if(dist < 20) {
            selectShapeState = SelectShapeState::noSelection;
            win->postProcessPreviewWidget->drawCursor = false;
            updatePlayArea();
            selectPlayAreaButton->setText("select play area");
        } else {
            playAreaShape.push_back(win->postProcessPreviewWidget->mousePos);
        }
        updatePreviewImg();
    }
}

void PostProcessingOptionPage::encodingThreadFunc()
{
    int bitrate = 8000000;

    videoEncoder = RPCameraInterface::createVideoEncoder();
    videoEncoder->setUseFrameTimestamp(true);
    std::vector<float> remainingAudio;

    bool firstFrame = true;
    uint64_t last_timestamp = 0;
    int last_frameId = -1;

    (*capCameraVid) >> currentFrameCam;
    cameraFrameId = 0;

    while(state != PostProcessingState::encodingStopped && questVideoSrc->isValid())
    {
        if(state == PostProcessingState::encodingPaused) {
            while(state == PostProcessingState::encodingPaused)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        cv::Mat questImg;
        uint64_t quest_timestamp;
        int frameId = last_frameId;
        while((frameId == last_frameId || questImg.empty()) && questVideoSrc->isValid()) {
            questVideoMngr->VideoTickImpl();
            questImg = questVideoMngr->getMostRecentImg(&quest_timestamp, &frameId);
            qDebug() << "timestamp: " << quest_timestamp << ", frameId: " << frameId;
        }
        if(!questVideoSrc->isValid())
            break;
        while(cameraFrameId+1 < listCameraTimestamp.size() && absdiff(quest_timestamp, listCameraTimestamp[cameraFrameId+1]) < absdiff(quest_timestamp, listCameraTimestamp[cameraFrameId]))
        {
            qDebug() << "target timestamp : " << quest_timestamp;
            qDebug() << "currentTimestamp : " << listCameraTimestamp[cameraFrameId];
            (*capCameraVid) >> currentFrameCam;
            cameraFrameId++;
        }
        libQuestMR::QuestAudioData **listAudioData;
        int nbAudioFrames = questVideoMngr->getMostRecentAudio(&listAudioData);
        if(questImg.empty())
            continue;

        if(!firstFrame && quest_timestamp <= last_timestamp)
            continue;

        cv::Mat fgMask;
        backgroundSubtractor->setROI(playAreaROI);
        backgroundSubtractor->apply(currentFrameCam, fgMask);
        cv::bitwise_and(fgMask, playAreaMask, fgMask);
        cv::Mat img = libQuestMR::composeMixedRealityImg(questImg, currentFrameCam, fgMask);
        //cv::Mat img = questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows)).clone();

        if(firstFrame)
            videoEncoder->open(encodingFilename.c_str(), img.rows, img.cols, 30, "", bitrate);
        std::vector<float> audioData = remainingAudio;
        remainingAudio.clear();
        int recordSampleRate = 44100;
        if(nbAudioFrames > 0){
            for(int i = 0; i < nbAudioFrames; i++) {
                int size = listAudioData[i]->getDataLength() / sizeof(float);
                const float *data = (const float*)listAudioData[i]->getData();
                int size2 = size;// * recordSampleRate / listAudioData[i]->getSampleRate();
                for(int j = 0; j < size2; j++)
                    audioData.push_back(data[j*size/size2]);
            }

            int nbAudioPacket = audioData.size() / 2048;
            videoEncoder->write_audio(&audioData[0], nbAudioPacket * 2048, listAudioData[0]->getLocalTimestamp());
            audioData.erase(audioData.begin(), audioData.begin() + nbAudioPacket * 2048);
        }
        remainingAudio = audioData;
        videoEncoder->write(RPCameraInterface::createImageDataFromMat(img, quest_timestamp, false));

        encodingMutex.lock();
        encodedFrame = img.clone();
        encodingMutex.unlock();

        firstFrame = false;
        last_timestamp = quest_timestamp;
    }
    videoEncoder->release();
    videoEncoder = NULL;

    if(state != PostProcessingState::encodingStopped)
        state = PostProcessingState::encodingFinished;
}

void PostProcessingOptionPage::onClickStartEncodingButton()
{
    if(state == PostProcessingState::encodingStarted || state == PostProcessingState::encodingPaused) {
        onClickStopButton();
        return ;
    }
    state = PostProcessingState::encodingPaused;
    bool camValid = loadCameraRecordingFile();
    bool questValid = loadQuestRecordingFile();

    if(!camValid || !questValid) {
        QMessageBox msgBox;
        msgBox.setText("You must select the camera and quest recording files");
        msgBox.exec();
        return ;
    }

    if(backgroundSubtractor == NULL) {
        QMessageBox msgBox;
        msgBox.setText("You must select a background subtraction method");
        msgBox.exec();
        return ;
    }

    QFileDialog dialog(NULL);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("video file (*.mp4)"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        if(filenames.size() == 1)
        {
            state = PostProcessingState::encodingStarted;

            encodingFilename = filenames[0].toStdString();
            qDebug() << encodingFilename.c_str();

            startEncodingButton->setText("stop encoding");
            playButton->setIcon(win->style()->standardIcon(QStyle::SP_MediaPause));

            encodingThread = new std::thread([&]()
                {
                    encodingThreadFunc();
                }
            );
        }
    }
}

void PostProcessingOptionPage::onClickSavePreviewSettingButton()
{
    win->recordMixedRealityPage->setPage();
}
