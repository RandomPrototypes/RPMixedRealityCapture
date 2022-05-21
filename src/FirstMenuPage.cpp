#include "mainwindow.h"
#include <QFileDialog>
#include <libQuestMR/QuestCalibData.h>
#include "FirstMenuPage.h"
#include "ConnectQuestPage.h"
#include "PostProcessingOptionPage.h"

FirstMenuPage::FirstMenuPage(MainWindow *win)
    :win(win)
{
}

void FirstMenuPage::setPage()
{
    win->currentPageName = MainWindow::PageName::firstMenuPage;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();

    QPushButton *calibrationButton = new QPushButton("Calibration");
    layout->addWidget(calibrationButton);

    QPushButton *recordingButton = new QPushButton("Recording");
    layout->addWidget(recordingButton);

    QPushButton *postProcessingButton = new QPushButton("Post processing video");
    layout->addWidget(postProcessingButton);

    win->mainWidget->setLayout(layout);

    connect(calibrationButton,SIGNAL(clicked()),this,SLOT(onClickCalibrationButton()));
    connect(recordingButton,SIGNAL(clicked()),this,SLOT(onClickRecordingButton()));
    connect(postProcessingButton,SIGNAL(clicked()),this,SLOT(onClickPostProcessingButton()));

}

void FirstMenuPage::onClickCalibrationButton()
{
    win->isCalibrationSection = true;
    win->connectQuestPage->setPage();
}

void FirstMenuPage::onClickRecordingButton()
{
    win->isCalibrationSection = false;
    win->connectQuestPage->setPage();
}

void FirstMenuPage::onClickPostProcessingButton()
{
    win->isCalibrationSection = false;
    win->postProcessingOptionPage->setPage();
}
