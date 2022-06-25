#include "mainwindow.h"
#include "PostProcessingOptionPage.h"
#include "RecordMixedRealityPage.h"
#include "FirstMenuPage.h"
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
    MixedRealityCompositorConfig& config = getCompositorConfig();
    config.videoSize = cv::Size(1280,720);
    config.camSubsampling = 1;
    config.questSubsampling = 1;
    config.camErosion = 0;
    config.questErosion = 0;
    config.camDelayMs = 0;
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
        win->startQuestRecorder();
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

    //hlayout1->addWidget(startEncodingButton);

    QTabWidget *tabWidget = new QTabWidget;

    QWidget *camBackgroundSubtractorOptionWidget = new QWidget();
    QGridLayout *camSettingLayout = new QGridLayout();
    camBackgroundSubtractorOptionWidget->setLayout(camSettingLayout);

    selectPlayAreaButton = new QPushButton();
    selectPlayAreaButton->setText("select play area");
    camSettingLayout->addWidget(selectPlayAreaButton, 0, 0, 1, 2);

    QLabel *delayLabel = new QLabel;
    delayLabel->setText("camera delay (ms):");
    QSpinBox *delaySpin = new QSpinBox();
    delaySpin->setValue(config.camDelayMs);
    delaySpin->setMinimum(-100000);
    delaySpin->setMaximum(100000);
    delaySpin->setSingleStep(50);
    camSettingLayout->addWidget(delayLabel, 1, 0);
    camSettingLayout->addWidget(delaySpin, 1, 1);
    connect(delaySpin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
        getCompositorConfig().camDelayMs = val;
    });

    QLabel *backgroundSubtractorLabel = new QLabel;
    backgroundSubtractorLabel->setText("Background subtractor method:");
    listCamBackgroundSubtractorCombo = new QComboBox;
    listCamBackgroundSubtractorCombo->addItem(QString("None"));
    for(int i = 0; i < libQuestMR::getBackgroundSubtractorCount(); i++)
        listCamBackgroundSubtractorCombo->addItem(QString(libQuestMR::getBackgroundSubtractorName(i).c_str()));
    camSettingLayout->addWidget(backgroundSubtractorLabel, 2, 0);
    camSettingLayout->addWidget(listCamBackgroundSubtractorCombo, 2, 1);

    QLabel *camSubsamplingLabel = new QLabel;
    camSubsamplingLabel->setText("subsampling:");
    QSpinBox *camSubsamplingSpin = new QSpinBox();
    camSubsamplingSpin->setValue(config.camSubsampling);
    camSubsamplingSpin->setMinimum(1);
    camSubsamplingSpin->setMaximum(16);
    camSubsamplingSpin->setSingleStep(1);
    camSettingLayout->addWidget(camSubsamplingLabel, 3, 0);
    camSettingLayout->addWidget(camSubsamplingSpin, 3, 1);
    connect(camSubsamplingSpin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
        MixedRealityCompositorConfig& config = getCompositorConfig();
        config.camSubsampling = val;
        if(config.camBackgroundSubtractor != NULL)
            config.camBackgroundSubtractor->restart();
    });

    QLabel *camErosionLabel = new QLabel;
    camErosionLabel->setText("erosion:");
    QSpinBox *camErosionSpin = new QSpinBox();
    camErosionSpin->setValue(config.camErosion);
    camErosionSpin->setMinimum(-30);
    camErosionSpin->setMaximum(30);
    camErosionSpin->setSingleStep(1);
    camSettingLayout->addWidget(camErosionLabel, 4, 0);
    camSettingLayout->addWidget(camErosionSpin, 4, 1);
    connect(camErosionSpin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
        MixedRealityCompositorConfig& config = getCompositorConfig();
        config.camErosion = val;
    });

    camBackgroundSubtractorOptionLayout = new QGridLayout();
    camSettingLayout->addLayout(camBackgroundSubtractorOptionLayout, 5, 0, 3, 2);


    QWidget *questBackgroundSubtractorOptionWidget = new QWidget();
    QGridLayout *questSettingLayout = new QGridLayout();
    questBackgroundSubtractorOptionWidget->setLayout(questSettingLayout);

    QLabel *questBackgroundSubtractorLabel = new QLabel;
    questBackgroundSubtractorLabel->setText("Background subtractor method:");
    listQuestBackgroundSubtractorCombo = new QComboBox;
    listQuestBackgroundSubtractorCombo->addItem(QString("None"));
    for(int i = 0; i < libQuestMR::getBackgroundSubtractorCount(); i++)
        listQuestBackgroundSubtractorCombo->addItem(QString(libQuestMR::getBackgroundSubtractorName(i).c_str()));
    questSettingLayout->addWidget(questBackgroundSubtractorLabel, 1, 0);
    questSettingLayout->addWidget(listQuestBackgroundSubtractorCombo, 1, 1);

    QLabel *questSubsamplingLabel = new QLabel;
    questSubsamplingLabel->setText("subsampling:");
    QSpinBox *questSubsamplingSpin = new QSpinBox();
    questSubsamplingSpin->setValue(config.questSubsampling);
    questSubsamplingSpin->setMinimum(1);
    questSubsamplingSpin->setMaximum(16);
    questSubsamplingSpin->setSingleStep(1);
    questSettingLayout->addWidget(questSubsamplingLabel, 2, 0);
    questSettingLayout->addWidget(questSubsamplingSpin, 2, 1);
    connect(questSubsamplingSpin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
        MixedRealityCompositorConfig& config = getCompositorConfig();
        config.questSubsampling = val;
        if(config.questBackgroundSubtractor != NULL)
            config.questBackgroundSubtractor->restart();
    });

    QLabel *questErosionLabel = new QLabel;
    questErosionLabel->setText("erosion:");
    QSpinBox *questErosionSpin = new QSpinBox();
    questErosionSpin->setValue(config.questErosion);
    questErosionSpin->setMinimum(-30);
    questErosionSpin->setMaximum(30);
    questErosionSpin->setSingleStep(1);
    questSettingLayout->addWidget(questErosionLabel, 3, 0);
    questSettingLayout->addWidget(questErosionSpin, 3, 1);
    connect(questErosionSpin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
        MixedRealityCompositorConfig& config = getCompositorConfig();
        config.questErosion = val;
    });

    questBackgroundSubtractorOptionLayout = new QGridLayout();
    questSettingLayout->addLayout(questBackgroundSubtractorOptionLayout, 4, 0, 3, 2);

    tabWidget->addTab(camBackgroundSubtractorOptionWidget, tr("Camera settings"));
    tabWidget->addTab(questBackgroundSubtractorOptionWidget, tr("Quest settings"));

    QLabel *previewLabel = new QLabel;
    previewLabel->setText("preview");

    QHBoxLayout *preview_checkbox_layout = new QHBoxLayout();
    camImgCheckbox = new QCheckBox;
    camImgCheckbox->setText("camera img");
    camImgCheckbox->setChecked(true);

    questImgCheckbox = new QCheckBox;
    questImgCheckbox->setText("quest img");
    questImgCheckbox->setChecked(true);

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

    //layout->addWidget(backgroundSubtractorLabel, 3, 0);
    //layout->addWidget(listBackgroundSubtractorCombo, 3, 1);

    layout->addLayout(preview_checkbox_layout, 5, 0, 1, 2);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(win->postProcessPreviewWidget);

    hlayout->addWidget(tabWidget);

    layout->addLayout(hlayout, 6, 0, 3, 6);
    //layout->addWidget(win->postProcessPreviewWidget, 6, 0, 3, 3);

    //layout->addLayout(backgroundSubtractorOptionLayout, 6, 3, 3, 1);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    //backToMenuButton->setMaximumWidth(300);
    backToMenuButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(backToMenuButton, Qt::AlignHCenter);
    layout->addWidget(backToMenuButton, 11, 0, 1, 6);

    win->mainWidget->setLayout(layout);

    connect(listCamBackgroundSubtractorCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectCamBackgroundSubtractorCombo(int)));
    connect(listQuestBackgroundSubtractorCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectQuestBackgroundSubtractorCombo(int)));

    connect(camImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickCamImgCheckbox()));
    connect(questImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickQuestImgCheckbox()));
    connect(matteImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickMatteImgCheckbox()));
    connect(greenBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickGreenBackgroundCheckbox()));
    connect(blackBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickBlackBackgroundCheckbox()));

    connect(selectPlayAreaButton,SIGNAL(clicked()),this,SLOT(onClickSelectPlayAreaButton()));
    connect(win->postProcessPreviewWidget,SIGNAL(clicked()),this,SLOT(onClickPreviewWidget()));

    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));



    refreshBackgroundSubtractorOption(false);
    refreshBackgroundSubtractorOption(true);
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

MixedRealityCompositorConfig& PostProcessingOptionPage::getCompositorConfig()
{
    return isLivePreview ? win->previewCompositorConfig : win->recordingCompositorConfig;
}

void PostProcessingOptionPage::refreshBackgroundSubtractorOption(bool questBackground)
{
    MixedRealityCompositorConfig& config = getCompositorConfig();
    QGridLayout *optionLayout;
    if(questBackground) {
        optionLayout = questBackgroundSubtractorOptionLayout;
        clearLayout(optionLayout);
        int bgMethodId = listQuestBackgroundSubtractorCombo->currentIndex() - 1;//subtract 1 because first option is None
        if(bgMethodId < 0)
        {
            config.questBackgroundSubtractor = NULL;
            return ;
        }
        config.questBackgroundSubtractor = libQuestMR::createBackgroundSubtractor(bgMethodId);
    } else {
        optionLayout = camBackgroundSubtractorOptionLayout;
        clearLayout(optionLayout);
        int bgMethodId = listCamBackgroundSubtractorCombo->currentIndex() - 1;//subtract 1 because first option is None
        if(bgMethodId < 0)
        {
            config.camBackgroundSubtractor = NULL;
            return ;
        }
        config.camBackgroundSubtractor = libQuestMR::createBackgroundSubtractor(bgMethodId);
    }

    int startLine = 0;

    auto& backgroundSubtractor = (questBackground ? config.questBackgroundSubtractor : config.camBackgroundSubtractor);
    for(int i = 0; i < backgroundSubtractor->getParameterCount(); i++) {
        if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeInt) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QSpinBox *spin = new QSpinBox();
            spin->setValue(backgroundSubtractor->getParameterValAsInt(i));
            optionLayout->addWidget(label, startLine+i, 0);
            optionLayout->addWidget(spin, startLine+i, 1);
            connect(spin,QOverload<int>::of(&QSpinBox::valueChanged),[=](int val){
                backgroundSubtractor->setParameterVal(i, val);
            });
        } else if(backgroundSubtractor->getParameterType(i) == libQuestMR::BackgroundSubtractorParamType::ParamTypeBool) {
            QLabel *label = new QLabel;
            label->setText(backgroundSubtractor->getParameterName(i).str().c_str());
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setChecked(backgroundSubtractor->getParameterValAsBool(i));
            optionLayout->addWidget(label, startLine+i, 0);
            optionLayout->addWidget(checkbox, startLine+i, 1);
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
            optionLayout->addWidget(label, startLine+i, 0);
            optionLayout->addLayout(hlayout, startLine+i, 1);
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
        if(win->questInput->hasNewImg)
            currentFrameQuest = win->questInput->getImgCopy();
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
        //qDebug() << "videoTickImpl";
        uint64_t quest_timestamp;
        currentFrameQuest = questVideoMngr->getMostRecentImg(&quest_timestamp);
        while(questVideoSrc->isValid() && quest_timestamp < listCameraTimestamp[cameraFrameId] + getCompositorConfig().camDelayMs) {
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
        //qDebug() << "videoTickImpl";
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
    MixedRealityCompositorConfig& config = getCompositorConfig();
    if(cameraFrameId == 0 && config.camBackgroundSubtractor != NULL)
        config.camBackgroundSubtractor->restart();
    if(!currentFrameCam.empty())
        config.videoSize = currentFrameCam.size();
    if(config.playAreaMask.size() != config.videoSize)
        updatePlayArea();

    config.useGreenBackground = greenBackgroundCheckbox->isChecked();
    config.useBlackBackground = blackBackgroundCheckbox->isChecked();
    config.useQuestImg = questImgCheckbox->isChecked();
    config.useCamImg = camImgCheckbox->isChecked();
    config.useMatteImg = matteImgCheckbox->isChecked();
    cv::Mat result = win->composeMixedRealityImg(currentFrameQuest, currentFrameCam, config);


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

void PostProcessingOptionPage::onSelectCamBackgroundSubtractorCombo(int id)
{
    refreshBackgroundSubtractorOption(false);
}

void PostProcessingOptionPage::onSelectQuestBackgroundSubtractorCombo(int id)
{
    refreshBackgroundSubtractorOption(true);
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
    MixedRealityCompositorConfig& config = getCompositorConfig();
    config.playAreaMask = cv::Mat(config.videoSize, CV_8UC1);
    if(playAreaShape.size() < 3)
    {
        config.playAreaROI = cv::Rect(0,0,config.videoSize.width,config.videoSize.height);
        config.playAreaMask.setTo(cv::Scalar(255));
    } else {
        config.playAreaMask.setTo(cv::Scalar(0));
        std::vector<cv::Point> list(playAreaShape.size());
        for(size_t i = 0; i < playAreaShape.size(); i++)
            list[i] = cv::Point(playAreaShape[i]);
        config.playAreaROI = cv::boundingRect(list);

        const cv::Point* ppt[1] = { &list[0] };
        int npt[] = { (int)list.size() };
        cv::fillPoly(config.playAreaMask, ppt, npt, 1, cv::Scalar( 255 ), cv::LINE_8);
        qDebug() << "updated mask";
    }
    if(config.camBackgroundSubtractor != NULL)
        config.camBackgroundSubtractor->restart();
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
    MixedRealityCompositorConfig& config = getCompositorConfig();
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
            //qDebug() << "timestamp: " << quest_timestamp << ", frameId: " << frameId;
        }
        if(!questVideoSrc->isValid())
            break;
        while(cameraFrameId+1 < listCameraTimestamp.size() && absdiff(quest_timestamp, listCameraTimestamp[cameraFrameId+1]) < absdiff(quest_timestamp, listCameraTimestamp[cameraFrameId]))
        {
            //qDebug() << "target timestamp : " << quest_timestamp;
            //qDebug() << "currentTimestamp : " << listCameraTimestamp[cameraFrameId];
            (*capCameraVid) >> currentFrameCam;
            cameraFrameId++;
        }
        libQuestMR::QuestAudioData **listAudioData;
        int nbAudioFrames = questVideoMngr->getMostRecentAudio(&listAudioData);
        if(questImg.empty())
            continue;

        if(!firstFrame && quest_timestamp <= last_timestamp)
            continue;

        cv::Mat img = win->composeMixedRealityImg(questImg, currentFrameCam, config);//.camBackgroundSubtractor, config.camSubsampling, config.playAreaROI, config.playAreaMask, config.videoSize, true, true, false, false, false);

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
    MixedRealityCompositorConfig& config = getCompositorConfig();
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

    if(config.camBackgroundSubtractor == NULL) {
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

void PostProcessingOptionPage::onClickBackToMenuButton()
{
    win->stopQuestRecorder();
    win->stopCamera();
    win->firstMenuPage->setPage();
}
