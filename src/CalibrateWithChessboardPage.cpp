#include "CalibrateWithChessboardPage.h"
#include <QSpinBox>
#include <QDir>
#include <QMessageBox>

CalibrateWithChessboardPage::CalibrateWithChessboardPage(MainWindow *win)
    :win(win)
{

}

void CalibrateWithChessboardPage::setPage()
{
    state = CalibState::capture;
    win->record_folder = "";
    win->listCalibrationFrames.clear();
    win->camPreviewWidget = NULL;
    win->currentPageName = MainWindow::PageName::calibrateWithChessboard;
    win->clearMainWidget();

    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(100,500,100,20);
    //layout->setAlignment(Qt::AlignTop);

    QLabel *calibrationLabel = new QLabel;
    calibrationLabel->setText("calibration pattern: ");
    layout->addWidget(calibrationLabel, 0, 2);

    QComboBox *patternTypeCombo = new QComboBox;
    patternTypeCombo->addItem("chessboard");
    /*patternTypeCombo->addItem("charuco");
    patternTypeCombo->addItem("circles");
    patternTypeCombo->addItem("assymetric circles");*/
    layout->addWidget(patternTypeCombo, 0, 3);

    QLabel *rowsLabel = new QLabel();
    rowsLabel->setText("Rows: ");
    rowsSpinBox = new QSpinBox;
    rowsSpinBox->setRange(0, 99);
    rowsSpinBox->setSingleStep(1);
    rowsSpinBox->setValue(8);
    QLabel *columnsLabel = new QLabel();
    columnsLabel->setText("Columns: ");
    columnsSpinBox = new QSpinBox;
    columnsSpinBox->setRange(0, 99);
    columnsSpinBox->setSingleStep(1);
    columnsSpinBox->setValue(11);
    QLabel *checkerWidthLabel = new QLabel();
    checkerWidthLabel->setText("Checker width (mm): ");
    checkerWidthSpinBox = new QSpinBox;
    checkerWidthSpinBox->setRange(0, 99);
    checkerWidthSpinBox->setSingleStep(1);
    checkerWidthSpinBox->setValue(15);
    layout->addWidget(rowsLabel, 1, 0);
    layout->addWidget(rowsSpinBox, 1, 1);
    layout->addWidget(columnsLabel, 1, 2);
    layout->addWidget(columnsSpinBox, 1, 3);
    layout->addWidget(checkerWidthLabel, 1, 4);
    layout->addWidget(checkerWidthSpinBox, 1, 5);





    QPushButton *startCalibratingButton = new QPushButton("start calibration");
    startCalibratingButton->setMaximumWidth(300);

    layout->addWidget(startCalibratingButton, 2, 2);

    win->mainWidget->setLayout(layout);

    connect(startCalibratingButton,SIGNAL(clicked()),this,SLOT(onClickStartCalibration()));
}

void CalibrateWithChessboardPage::captureFrame()
{
    CalibrationFrame frame;
    frame.img = win->videoInput->getImgCopy(&frame.imgTimestamp);
    win->listCalibrationFrames.push_back(frame);
}

void CalibrateWithChessboardPage::onClickStartCalibration()
{
    boardSize = cv::Size(std::stoi(columnsSpinBox->text().toStdString()), std::stoi(rowsSpinBox->text().toStdString()));
    checkerWidth = std::stof(checkerWidthSpinBox->text().toStdString());
    win->clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    //layout->setAlignment(Qt::AlignTop);

    QHBoxLayout *hlayout = new QHBoxLayout();
    QPushButton *captureFrameButton = new QPushButton("capture frame");
    QPushButton *stopCalibratingButton = new QPushButton("stop calibration");
    stopCalibratingButton->setMaximumWidth(300);

    hlayout->addWidget(captureFrameButton);
    hlayout->addWidget(stopCalibratingButton);

    win->camPreviewWidget = new OpenCVWidget(cv::Size(1280, 720));

    win->camPreviewWidget->setImg(cv::Mat());

    layout->addLayout(hlayout);
    layout->addWidget(win->camPreviewWidget);

    win->mainWidget->setLayout(layout);

    connect(captureFrameButton,SIGNAL(clicked()),this,SLOT(onClickCaptureFrame()));
    connect(stopCalibratingButton,SIGNAL(clicked()),this,SLOT(onClickStopCalibration()));
    connect(win->camPreviewWidget,SIGNAL(clicked()),this,SLOT(onClickPreviewWidget()));


    win->record_folder = "output/calib"+getCurrentDateTimeStr();
    QDir().mkdir(win->record_folder.c_str());
    win->recording = true;

    win->startCamera();

    if(win->questComThreadData != NULL)
        currentTriggerCount = win->questComThreadData->getTriggerCount();
}

void CalibrateWithChessboardPage::onClickCaptureFrame()
{
    captureFrame();
}

void CalibrateWithChessboardPage::onClickStopCalibration()
{
    if(state != CalibState::capture)
        return ;
    if(win->listCalibrationFrames.size() < 4) {
        QMessageBox msgBox;
        msgBox.setText("You must capture at least 4 frames for calibration!!!");
        msgBox.exec();
    } else {
        win->recording = false;
        state = CalibState::calibrate;
    }
}

void CalibrateWithChessboardPage::detectChessboardsAndCalibrate()
{
    for(size_t i = 0; i < win->listCalibrationFrames.size(); i++)
    {
        cv::Mat img = win->listCalibrationFrames[i].img;
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners( img, boardSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FAST_CHECK);
        if(found)
            win->listCalibrationFrames[i].corners = corners;
    }
}

void CalibrateWithChessboardPage::onTimer()
{
    if(win->recording) {
        if(win->videoInput->hasNewImg)
        {
            cv::Mat img = win->videoInput->getImgCopy();
            std::vector<cv::Point2f> corners;
            bool found = cv::findChessboardCorners( img, boardSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FAST_CHECK);
            qDebug() << "search chessboard " << boardSize.width << "x" << boardSize.height << ": " << (found ? "found" : "not found");
            cv::drawChessboardCorners(img, boardSize, cv::Mat(corners), found);
            win->camPreviewWidget->setImg(img);
        }
        if(win->questComThreadData != NULL && win->questComThreadData->getTriggerCount() > currentTriggerCount)
        {
            qDebug() << "trigger";
            captureFrame();
            currentTriggerCount = win->questComThreadData->getTriggerCount();
        }
    } else if(state == CalibState::calibrate) {
        if(win->recording_finished) {
            detectChessboardsAndCalibrate();
            state = CalibState::verify;
            win->currentCalibrationFrame = 0;
        }
    } else if(state == CalibState::verify) {
        cv::Mat img = win->listCalibrationFrames[win->currentCalibrationFrame].img.clone();
        std::vector<cv::Point2f> corners = win->listCalibrationFrames[win->currentCalibrationFrame].corners;
        cv::drawChessboardCorners(img, boardSize, cv::Mat(corners), corners.size() > 0);
        win->camPreviewWidget->setImg(img);
    }
}

void CalibrateWithChessboardPage::onClickPreviewWidget()
{
    if(state == CalibState::verify)
    {
        qDebug() << "click on preview";
        win->currentCalibrationFrame = (win->currentCalibrationFrame+1) % win->listCalibrationFrames.size();
    }
}
