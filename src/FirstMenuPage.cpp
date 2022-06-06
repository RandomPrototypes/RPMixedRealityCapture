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

    QPushButton *calibrationButton = new QPushButton(" Calibration ");
    calibrationButton->setMaximumWidth(200);
    calibrationButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(calibrationButton, Qt::AlignHCenter);
    layout->addWidget(calibrationButton);

    QPushButton *recordingButton = new QPushButton(" Recording ");
    recordingButton->setMaximumWidth(200);
    recordingButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(recordingButton, Qt::AlignHCenter);
    layout->addWidget(recordingButton);

    QPushButton *postProcessingButton = new QPushButton(" Post processing video ");
    postProcessingButton->setMaximumWidth(200);
    postProcessingButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(postProcessingButton, Qt::AlignHCenter);
    layout->addWidget(postProcessingButton);

    win->mainWidget->setLayout(layout);

    layout->setAlignment(Qt::AlignHCenter);
    layout->setSpacing(100);

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
