#include "mainwindow.h"
#include <QFileDialog>
#include "QuestCalibData.h"

void MainWindow::setCalibrationOptionPage()
{
    currentPageName = "calibrationOption";
    clearMainWidget();

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

     mainWidget->setLayout(layout);

     connect(checkCurrentCalibButton,SIGNAL(clicked()),this,SLOT(onClickCheckCurrentCalibrationButton()));
     connect(loadCalibrationFileButton,SIGNAL(clicked()),this,SLOT(onClickLoadCalibrationFileButton()));
     connect(recalibrateCameraPoseButton,SIGNAL(clicked()),this,SLOT(onClickRecalibratePoseButton()));
     connect(recordMixedRealityButton,SIGNAL(clicked()),this,SLOT(onClickRecordMixedRealityButton()));
}

void MainWindow::onClickLoadCalibrationFileButton()
{
    QFileDialog dialog(this);
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

void MainWindow::onClickCheckCurrentCalibrationButton()
{
    setCheckCalibrationPage();
}

void MainWindow::onClickRecalibratePoseButton()
{
    setRecalibratePosePage();
}


void MainWindow::onClickRecordMixedRealityButton()
{
    questInput->videoThread = new std::thread([&]()
        {
            questThreadFunc();
        }
    );
    setRecordMixedRealityPage();
}
