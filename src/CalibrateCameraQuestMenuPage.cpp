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
    calibrateCameraWithChessboardButton->setMaximumWidth(500);
    calibrateCameraWithChessboardButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(calibrateCameraWithChessboardButton, Qt::AlignHCenter);
    layout->addWidget(calibrateCameraWithChessboardButton);

    QPushButton *calibrateCameraPoseButton = new QPushButton("Calibrate camera pose");
    calibrateCameraPoseButton->setMaximumWidth(500);
    calibrateCameraPoseButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(calibrateCameraPoseButton, Qt::AlignHCenter);
    layout->addWidget(calibrateCameraPoseButton);

    QPushButton *calibrateFullButton = new QPushButton("Calibrate camera intrinsic and pose (simple)");
    calibrateFullButton->setMaximumWidth(500);
    calibrateFullButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(calibrateFullButton, Qt::AlignHCenter);
    layout->addWidget(calibrateFullButton);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    backToMenuButton->setMaximumWidth(500);
    backToMenuButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(backToMenuButton, Qt::AlignHCenter);
    layout->addWidget(backToMenuButton);

    win->mainWidget->setLayout(layout);
    layout->setAlignment(Qt::AlignHCenter);
    layout->setSpacing(100);

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
