#include "mainwindow.h"
#include <QFileDialog>
#include "QuestCalibData.h"
#include "CalibrateWithChessboardPage.h"
#include "CalibrationOptionPage.h"
#include "CheckCalibrationPage.h"
#include "CalibrateCameraPosePage.h"
#include "RecordMixedRealityPage.h"

CalibrationOptionPage::CalibrationOptionPage(MainWindow *win)
    :win(win)
{
}

void CalibrationOptionPage::setPage()
{
    win->currentPageName = MainWindow::PageName::calibrationOption;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();

    QPushButton *checkCurrentCalibButton = new QPushButton("check current calibration");
    layout->addWidget(checkCurrentCalibButton);

    QPushButton *loadCalibrationFileButton = new QPushButton("load calibration file");
    layout->addWidget(loadCalibrationFileButton);

    QPushButton *saveCurrentCalibrationFileButton = new QPushButton("save calibration to file");
    layout->addWidget(saveCurrentCalibrationFileButton);

    QPushButton *calibrateWithChessboardButton = new QPushButton("calibrate camera with chessboard");
    layout->addWidget(calibrateWithChessboardButton);

    QPushButton *recalibrateCameraPoseButton = new QPushButton("recalibrate camera pose");
    layout->addWidget(recalibrateCameraPoseButton);

    QPushButton *recordMixedRealityButton = new QPushButton("record mixed reality");
    layout->addWidget(recordMixedRealityButton);

    win->mainWidget->setLayout(layout);

    connect(checkCurrentCalibButton,SIGNAL(clicked()),this,SLOT(onClickCheckCurrentCalibrationButton()));
    connect(loadCalibrationFileButton,SIGNAL(clicked()),this,SLOT(onClickLoadCalibrationFileButton()));
    connect(calibrateWithChessboardButton,SIGNAL(clicked()),this,SLOT(onClickCalibrateWithChessboardButton()));
    connect(recalibrateCameraPoseButton,SIGNAL(clicked()),this,SLOT(onClickRecalibratePoseButton()));
    connect(recordMixedRealityButton,SIGNAL(clicked()),this,SLOT(onClickRecordMixedRealityButton()));
}

void CalibrationOptionPage::onClickCalibrateWithChessboardButton()
{
    win->calibrateWithChessboardPage->setPage();
}

void CalibrationOptionPage::onClickLoadCalibrationFileButton()
{
    QFileDialog dialog(win);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("calib file (*.xml *.json)"));
    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        if(filenames.size() == 1)
        {
            libQuestMR::QuestCalibData calibData;
            calibData.loadXMLFile(filenames[0].toStdString().c_str());

            printf("%s\n", calibData.generateXMLString().c_str());
        }
    }
}

void CalibrationOptionPage::onClickCheckCurrentCalibrationButton()
{
    win->checkCalibrationPage->setPage();
}

void CalibrationOptionPage::onClickRecalibratePoseButton()
{
    win->calibrateCameraPosePage->setPage();
}


void CalibrationOptionPage::onClickRecordMixedRealityButton()
{
    win->questInput->videoThread = new std::thread([&]()
        {
            win->questThreadFunc();
        }
    );
    win->recordMixedRealityPage->setPage();
}
