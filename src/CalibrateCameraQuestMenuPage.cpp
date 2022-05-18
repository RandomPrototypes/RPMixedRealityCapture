#include "mainwindow.h"
#include <QFileDialog>
#include <libQuestMR/QuestCalibData.h>
#include "CalibrateWithChessboardPage.h"
#include "CalibrateCameraQuestMenuPage.h"
#include "CalibrationOptionPage.h"
#include "CalibrateCameraPosePage.h"
#include "RecordMixedRealityPage.h"
#include "FirstMenuPage.h"

CalibrateCameraQuestMenuPage::CalibrateCameraQuestMenuPage(MainWindow *win)
    :win(win)
{
}

void CalibrateCameraQuestMenuPage::setPage()
{
    win->currentPageName = MainWindow::PageName::calibrateCameraQuestMenu;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(200,50,200,50);

    QPushButton *calibrateCameraWithChessboardButton = new QPushButton("Calibrate camera instrinsic with chessboard");
    layout->addWidget(calibrateCameraWithChessboardButton);

    QPushButton *calibrateCameraPoseButton = new QPushButton("Calibrate camera pose");
    layout->addWidget(calibrateCameraPoseButton);

    QPushButton *calibrateFullButton = new QPushButton("Calibrate camera intrinsic and pose (simple)");
    layout->addWidget(calibrateFullButton);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    layout->addWidget(backToMenuButton);

    win->mainWidget->setLayout(layout);

    connect(calibrateCameraWithChessboardButton,SIGNAL(clicked()),this,SLOT(onClickCalibrateCameraWithChessboardButton()));
    connect(calibrateCameraPoseButton,SIGNAL(clicked()),this,SLOT(onClickCalibrateCameraPoseButton()));
    connect(calibrateFullButton,SIGNAL(clicked()),this,SLOT(onClickCalibrateFullButton()));
    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));
}

void CalibrateCameraQuestMenuPage::onClickCalibrateCameraWithChessboardButton()
{
    win->calibrateWithChessboardPage->setPage();
}

void CalibrateCameraQuestMenuPage::onClickCalibrateCameraPoseButton()
{
    win->calibrateCameraPosePage->setEstimateIntrinsic(false);
    win->calibrateCameraPosePage->setPage();
}

void CalibrateCameraQuestMenuPage::onClickCalibrateFullButton()
{
    win->calibrateCameraPosePage->setEstimateIntrinsic(true);
    win->calibrateCameraPosePage->setPage();
}

void CalibrateCameraQuestMenuPage::onClickBackToMenuButton()
{
    win->calibrationOptionPage->setPage();
}
