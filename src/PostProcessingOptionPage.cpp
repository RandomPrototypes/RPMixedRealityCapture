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

PostProcessingOptionPage::PostProcessingOptionPage(MainWindow *win)
    :win(win)
{

}

void PostProcessingOptionPage::setPage()
{
    win->currentPageName = MainWindow::PageName::postProcessingOption;
    win->clearMainWidget();

    layout = new QGridLayout();
    layout->setContentsMargins(100,100,100,300);

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

    backgroundSubtractorOptionLayout = new QGridLayout();

    QLabel *previewLabel = new QLabel;
    previewLabel->setText("preview");

    QHBoxLayout *preview_checkbox_layout = new QHBoxLayout();
    camImgCheckbox = new QCheckBox;
    camImgCheckbox->setText("camera img");

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

    layout->addWidget(postProcessingLabel, 0, 0);

    layout->addWidget(questRecordingFileLabel, 1, 0);
    layout->addWidget(questRecordingFileEdit, 1, 1);
    layout->addWidget(questRecordingFileBrowseButton, 1, 2);

    layout->addWidget(camRecordingFileLabel, 2, 0);
    layout->addWidget(camRecordingFileEdit, 2, 1);
    layout->addWidget(camRecordingFileBrowseButton, 2, 2);

    layout->addWidget(backgroundSubtractorLabel, 3, 0);
    layout->addWidget(listBackgroundSubtractorCombo, 3, 1);

    layout->addWidget(previewLabel, 4, 0);
    layout->addLayout(preview_checkbox_layout, 4, 1);

    layout->addWidget(win->postProcessPreviewWidget, 5, 0, 3, 3);

    layout->addLayout(backgroundSubtractorOptionLayout, 5, 3, 3, 3);
    win->mainWidget->setLayout(layout);

    connect(questRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickQuestRecordingFileBrowseButton()));
    connect(camRecordingFileBrowseButton,SIGNAL(clicked()),this,SLOT(onClickCamRecordingFileBrowseButton()));
    connect(listBackgroundSubtractorCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectBackgroundSubtractorCombo(int)));

    connect(camImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickCamImgCheckbox()));
    connect(questImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickQuestImgCheckbox()));
    connect(matteImgCheckbox,SIGNAL(clicked()),this,SLOT(onClickMatteImgCheckbox()));
    connect(greenBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickGreenBackgroundCheckbox()));
    connect(blackBackgroundCheckbox,SIGNAL(clicked()),this,SLOT(onClickBlackBackgroundCheckbox()));

    refreshBackgroundSubtractorOption();
}

void PostProcessingOptionPage::refreshBackgroundSubtractorOption()
{
    clearLayout(backgroundSubtractorOptionLayout);
    int bgMethodId = listBackgroundSubtractorCombo->currentIndex();
    if(bgMethodId < 0)
        return ;
    std::shared_ptr<libQuestMR::BackgroundSubtractor> backgroundSubtractor = libQuestMR::createBackgroundSubtractor(bgMethodId);
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
}

void PostProcessingOptionPage::updateCamRecordingFile()
{
    std::string filename = camRecordingFileEdit->text().toStdString();
    cv::VideoCapture cap(filename);
    if(!cap.isOpened()) {
        QMessageBox msgBox;
        msgBox.setText("Can not connect to the quest...");
        msgBox.exec();
        return ;
    }
    cap >> firstFrameCam;
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
            updateCamRecordingFile();
        }
    }
}

void PostProcessingOptionPage::updatePreviewImg()
{
    cv::Size size(1280,720);
    if(!firstFrameCam.empty())
        size = firstFrameCam.size();
    cv::Mat background = cv::Mat(size, CV_8UC3);
    cv::Mat middleImg;
    if(greenBackgroundCheckbox->isChecked()) {
        background.setTo(cv::Scalar(0,255,0));
    } else if(blackBackgroundCheckbox->isChecked()) {
        background.setTo(cv::Scalar(0,0,0));
    }
    if(camImgCheckbox->isChecked()) {
        middleImg = firstFrameCam;
    }
    cv::Mat result = background;
    if(!middleImg.empty())
        result = middleImg;
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
