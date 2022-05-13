#include "mainwindow.h"
#include <QFileDialog>
#include <libQuestMR/QuestCalibData.h>
#include "FirstMenuPage.h"
#include "ConnectQuestPage.h"

FirstMenuPage::FirstMenuPage(MainWindow *win)
    :win(win)
{
}

void FirstMenuPage::setPage()
{
    win->currentPageName = MainWindow::PageName::firstMenuPage;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();

    QPushButton *calibrationButton = new QPushButton("calibration");
    layout->addWidget(calibrationButton);

    QPushButton *recordingButton = new QPushButton("recording");
    layout->addWidget(recordingButton);

    win->mainWidget->setLayout(layout);

    connect(calibrationButton,SIGNAL(clicked()),this,SLOT(onClickCalibrationButton()));
    connect(recordingButton,SIGNAL(clicked()),this,SLOT(onClickRecordingButton()));
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
