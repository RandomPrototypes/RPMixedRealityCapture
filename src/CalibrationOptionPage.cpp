#include "mainwindow.h"
#include <QFileDialog>
#include <libQuestMR/QuestCalibData.h>
#include "CalibrateCameraQuestMenuPage.h"
#include "CalibrationOptionPage.h"
#include "CheckCalibrationPage.h"
#include "FirstMenuPage.h"

CalibrationOptionPage::CalibrationOptionPage(MainWindow *win)
    :win(win)
{
}

void CalibrationOptionPage::setPage()
{
    win->currentPageName = MainWindow::PageName::calibrationOption;
    win->clearMainWidget();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(200,50,200,50);


    QPushButton *checkCurrentCalibButton = new QPushButton("Check current calibration");
    checkCurrentCalibButton->setMaximumWidth(300);
    checkCurrentCalibButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(checkCurrentCalibButton, Qt::AlignHCenter);
    layout->addWidget(checkCurrentCalibButton);

    QPushButton *importExportCalibrationFileButton = new QPushButton("Import/export calibration file");
    importExportCalibrationFileButton->setMaximumWidth(300);
    importExportCalibrationFileButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(importExportCalibrationFileButton, Qt::AlignHCenter);
    layout->addWidget(importExportCalibrationFileButton);

    QPushButton *calibrateCameraQuestButton = new QPushButton("Calibrate camera/quest");
    calibrateCameraQuestButton->setMaximumWidth(300);
    calibrateCameraQuestButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(calibrateCameraQuestButton, Qt::AlignHCenter);
    layout->addWidget(calibrateCameraQuestButton);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    backToMenuButton->setMaximumWidth(300);
    backToMenuButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(backToMenuButton, Qt::AlignHCenter);
    layout->addWidget(backToMenuButton);

    win->mainWidget->setLayout(layout);

    layout->setAlignment(Qt::AlignHCenter);
    layout->setSpacing(100);

    connect(checkCurrentCalibButton,SIGNAL(clicked()),this,SLOT(onClickCheckCurrentCalibrationButton()));
    connect(importExportCalibrationFileButton,SIGNAL(clicked()),this,SLOT(onClickImportExportCalibrationFileButton()));
    connect(calibrateCameraQuestButton,SIGNAL(clicked()),this,SLOT(onClickCalibrateCameraQuestButton()));
    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));
}

void CalibrationOptionPage::onClickCalibrateCameraQuestButton()
{
    win->calibrateCameraQuestMenuPage->setPage();
}

void CalibrationOptionPage::onClickImportExportCalibrationFileButton()
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

void CalibrationOptionPage::onClickBackToMenuButton()
{
    win->stopQuestCommunicator();
    win->stopCamera();
    win->firstMenuPage->setPage();
}
