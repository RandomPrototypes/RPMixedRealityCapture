#include "mainwindow.h"
#include <QRadioButton>

using namespace RPCameraInterface;

void MainWindow::setCameraSelectPage()
{
    currentPageName = "cameraSelect";
    clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 10);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Select the type of camera...");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    hlayout = new QHBoxLayout();
    layout->setContentsMargins(50, 50, 50, 200);
    CameraMngr *cameraMngr = CameraMngr::getInstance();
    for(size_t i = 0; i < cameraMngr->listCameraEnumAndFactory.size(); i++)
    {
        std::string type = "&"+cameraMngr->listCameraEnumAndFactory[i].enumerator->cameraType;
        QRadioButton *cameraButton = new QRadioButton(tr(type.c_str()));
        hlayout->addWidget(cameraButton);
        connect(cameraButton,&QRadioButton::clicked,[=](){onClickCameraButton(i);});
    }

    cameraParamLayout = new QVBoxLayout();


    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(hlayout);
    layout->addLayout(cameraParamLayout);

    mainWidget->setLayout(layout);
}

void MainWindow::refreshCameraComboBox(CameraEnumerator *camEnumerator)
{
    listCameraCombo->clear();
    camEnumerator->detectCameras();

    listCameraIds.clear();
    for (size_t i = 0; i < camEnumerator->count(); i++) {
        listCameraIds.push_back(camEnumerator->getCameraId(i));
        listCameraCombo->addItem(QString(camEnumerator->getCameraName(i).c_str()));
    }

}

void MainWindow::setCameraParamBox(CameraEnumerator *camEnumerator)
{
    clearLayout(cameraParamLayout);

    QHBoxLayout *cameraSelectLayout = new QHBoxLayout();
    cameraSelectLayout->setContentsMargins(10,50,10,50);

    if(camEnumerator->listRequiredField.size() > 0)
    {
        QHBoxLayout *hLayout1 = new QHBoxLayout();
        QVBoxLayout *vLayout1 = new QVBoxLayout();
        for(size_t i = 0; i < camEnumerator->listRequiredField.size(); i++)
        {
            CameraEnumeratorField& field = camEnumerator->listRequiredField[i];
            QHBoxLayout *hLayout2 = new QHBoxLayout();
            QLabel *textLabel = new QLabel;
            textLabel->setText((field.text+": ").c_str());
            hLayout2->addWidget(textLabel);
            if(field.type == "text")
            {
                QLineEdit *lineEdit = new QLineEdit();
                lineEdit->setText(field.value.c_str());
                lineEdit->setContentsMargins(10,30,10,30);

                field.extra_param = lineEdit;

                hLayout2->addWidget(lineEdit);
            }
            vLayout1->addLayout(hLayout2);
        }
        hLayout1->addLayout(vLayout1);
        QPushButton *refreshButton = new QPushButton("refresh");
        connect(refreshButton,&QPushButton::clicked,[=]()
        {
            for(size_t i = 0; i < camEnumerator->listRequiredField.size(); i++)
            {
                CameraEnumeratorField& field = camEnumerator->listRequiredField[i];
                if(field.type == "text")
                    field.value = ((QLineEdit*)field.extra_param)->text().toStdString();
            }
            refreshCameraComboBox(camEnumerator);
        });
        hLayout1->addWidget(refreshButton);
        cameraParamLayout->addLayout(hLayout1);
    }

    QLabel *cameraLabel = new QLabel;
    cameraLabel->setText("camera: ");

    listCameraCombo = new QComboBox;

    refreshCameraComboBox(camEnumerator);

    QPushButton *selectButton = new QPushButton("select");

    cameraSelectLayout->addWidget(cameraLabel);
    cameraSelectLayout->addWidget(listCameraCombo);


    cameraParamLayout->addLayout(cameraSelectLayout);

    cameraParamLayout->addWidget(selectButton);

    connect(selectButton,SIGNAL(clicked()),this,SLOT(onClickSelectCameraButton()));
}

void MainWindow::onClickCameraButton(int i)
{
    currentCameraEnumId = i;
    CameraMngr *cameraMngr = CameraMngr::getInstance();
    CameraEnumerator *camEnumerator = cameraMngr->listCameraEnumAndFactory[currentCameraEnumId].enumerator;
    setCameraParamBox(camEnumerator);
}

void MainWindow::onClickSelectCameraButton()
{
    cameraId = listCameraIds[listCameraCombo->currentIndex()];

    videoInput->videoThread = new std::thread([&]()
        {
            videoThreadFunc(cameraId);
        }
    );
    setCalibrationOptionPage();
    //setCameraPreview();
}
