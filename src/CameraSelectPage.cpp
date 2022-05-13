#include "mainwindow.h"
#include "CameraSelectPage.h"
#include "CalibrationOptionPage.h"
#include "RecordMixedRealityPage.h"
#include <QRadioButton>
using namespace RPCameraInterface;

CameraSelectPage::CameraSelectPage(MainWindow *win)
    :win(win)
{

}

void CameraSelectPage::setPage()
{
    win->currentPageName = MainWindow::PageName::cameraSelect;
    win->clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 10);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Select the type of camera...");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    QHBoxLayout *hlayout = new QHBoxLayout();
    layout->setContentsMargins(50, 50, 50, 200);
    std::vector<CaptureBackend> availableBackends = getAvailableCaptureBackends();
    win->listCameraEnumerator.clear();
    for(size_t i = 0; i < availableBackends.size(); i++)
    {
        std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator = getCameraEnumerator(availableBackends[i]);
        int index = win->listCameraEnumerator.size();
        win->listCameraEnumerator.push_back(camEnumerator);
        std::string type = std::string("&")+camEnumerator->getCameraType();
        QRadioButton *cameraButton = new QRadioButton(tr(type.c_str()));
        hlayout->addWidget(cameraButton);
        connect(cameraButton,&QRadioButton::clicked,[=](){onClickCameraButton(index);});
    }

    cameraParamLayout = new QVBoxLayout();


    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(hlayout);
    layout->addLayout(cameraParamLayout);

    win->mainWidget->setLayout(layout);
}

void CameraSelectPage::refreshCameraComboBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator)
{
    listCameraCombo->clear();
    camEnumerator->detectCameras();

    listCameraIds.clear();
    for (size_t i = 0; i < camEnumerator->count(); i++) {
        listCameraIds.push_back(camEnumerator->getCameraId(i));
        listCameraCombo->addItem(QString(camEnumerator->getCameraName(i)));
    }

}

void CameraSelectPage::setCameraParamBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator)
{
    clearLayout(cameraParamLayout);

    QHBoxLayout *cameraSelectLayout = new QHBoxLayout();
    cameraSelectLayout->setContentsMargins(10,50,10,50);

    if(camEnumerator->getNbParamField() > 0)
    {
        QHBoxLayout *hLayout1 = new QHBoxLayout();
        QVBoxLayout *vLayout1 = new QVBoxLayout();
        for(size_t i = 0; i < camEnumerator->getNbParamField(); i++)
        {
            CameraEnumeratorField* field = camEnumerator->getParamField(i);
            QHBoxLayout *hLayout2 = new QHBoxLayout();
            QLabel *textLabel = new QLabel;
            textLabel->setText((std::string(field->getText())+": ").c_str());
            hLayout2->addWidget(textLabel);
            if(!strcmp(field->getType(), "text"))
            {
                QLineEdit *lineEdit = new QLineEdit();
                lineEdit->setText(field->getValue());
                lineEdit->setContentsMargins(10,30,10,30);

                field->setExtraParam(lineEdit);

                hLayout2->addWidget(lineEdit);
            }
            vLayout1->addLayout(hLayout2);
        }
        hLayout1->addLayout(vLayout1);
        QPushButton *refreshButton = new QPushButton("refresh");
        connect(refreshButton,&QPushButton::clicked,[=]()
        {
            for(size_t i = 0; i < camEnumerator->getNbParamField(); i++)
            {
                CameraEnumeratorField* field = camEnumerator->getParamField(i);
                if(!strcmp(field->getType(), "text"))
                    field->setValue(((QLineEdit*)field->getExtraParam())->text().toStdString().c_str());
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

void CameraSelectPage::onClickCameraButton(int i)
{
    win->currentCameraEnumId = i;
    setCameraParamBox(win->listCameraEnumerator[i]);
}

void CameraSelectPage::onClickSelectCameraButton()
{
    win->cameraId = listCameraIds[listCameraCombo->currentIndex()];

    if(win->isCalibrationSection)
        win->calibrationOptionPage->setPage();
    else {
        win->recordMixedRealityPage->setPage();
    }
}
