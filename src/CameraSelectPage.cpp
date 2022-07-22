#include "mainwindow.h"
#include "CameraSelectPage.h"
#include "FirstMenuPage.h"
#include "CalibrationOptionPage.h"
#include "RecordMixedRealityPage.h"
#include "PostProcessingOptionPage.h"
#include <QRadioButton>
#include <QToolButton>
#include <QDebug>
using namespace RPCameraInterface;

CameraSelectPage::CameraSelectPage(MainWindow *win)
    :win(win)
{

}

void CameraSelectPage::setPage()
{
    qDebug() << "CameraSelectPage::setPage()";
    win->currentPageName = MainWindow::PageName::cameraSelect;
    win->clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 10);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Select the type of camera...");
    QFont font;
    font.setPointSize(20);
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
        cameraButton->setStyleSheet("font-size: 20px;");
        hlayout->addWidget(cameraButton);
        connect(cameraButton,&QRadioButton::clicked,[=](){onClickCameraButton(index);});
    }

    cameraParamLayout = new QVBoxLayout();

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    //backToMenuButton->setMaximumWidth(300);
    backToMenuButton->setStyleSheet("font-size: 20px;");
    layout->setAlignment(backToMenuButton, Qt::AlignHCenter);


    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(hlayout);
    layout->addLayout(cameraParamLayout);
    layout->addSpacing(50);
    layout->addWidget(backToMenuButton);

    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));

    win->mainWidget->setLayout(layout);
}

void CameraSelectPage::refreshCameraComboBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator)
{
    qDebug() << "refreshCameraComboBox";
    listCameraCombo->clear();
    camEnumerator->detectCameras();

    listCameraIds.clear();
    for (size_t i = 0; i < camEnumerator->count(); i++) {
        listCameraIds.push_back(camEnumerator->getCameraId(i));
        listCameraCombo->addItem(QString(camEnumerator->getCameraName(i)));
        qDebug() << "listCameraCombo->addItem(" << camEnumerator->getCameraName(i) << ")";
    }
    refreshCameraFormatComboBox();
}

void CameraSelectPage::refreshCameraFormatComboBox()
{
    qDebug() << "refreshCameraFormatComboBox";
    listCameraFormatCombo->clear();
    if(listCameraCombo->currentIndex() >= 0 && listCameraCombo->currentIndex() < listCameraIds.size()) {
        std::shared_ptr<CameraInterface> cam = getCameraInterface(win->listCameraEnumerator[win->currentCameraEnumId]->getBackend());
        qDebug() << "cam->open(" << listCameraIds[listCameraCombo->currentIndex()].c_str() << ")";
        if(!cam->open(listCameraIds[listCameraCombo->currentIndex()].c_str()))
        {
            qDebug() << "!cam->open : " << cam->getErrorMsg();
            return ;
        }
        std::vector<ImageFormat> listFormats = cam->getListAvailableFormat();
        for (size_t i = 0; i < listFormats.size(); i++) {
            std::string format = std::to_string(listFormats[i].width) + "x" + std::to_string(listFormats[i].height) + " ("+toString(listFormats[i].type).c_str()+")";
            listCameraFormatCombo->addItem(QString(format.c_str()));
            qDebug() << "listCameraFormatCombo->addItem(" << format.c_str() << ")";
        }
        cam->close();
        win->listCameraFormats = listFormats;
        win->currentCameraFormatId = 0;
    }
}

void CameraSelectPage::setCameraParamBox(std::shared_ptr<RPCameraInterface::CameraEnumerator> camEnumerator)
{
    clearLayout(cameraParamLayout);

    QHBoxLayout *cameraSelectLayout = new QHBoxLayout();
    cameraSelectLayout->setContentsMargins(10,50,10,50);

    QHBoxLayout *cameraSelectFormatLayout = new QHBoxLayout();
    cameraSelectFormatLayout->setContentsMargins(10,50,10,50);


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
            textLabel->setMaximumWidth(100);
            if(!strcmp(field->getType(), "text"))
            {
                QLineEdit *lineEdit = new QLineEdit();
                lineEdit->setText(field->getValue());
                lineEdit->setContentsMargins(10,30,10,30);
                lineEdit->setMaximumWidth(300);

                field->setExtraParam(lineEdit);

                hLayout2->addWidget(lineEdit);
            }
            vLayout1->addLayout(hLayout2);
        }
        hLayout1->addLayout(vLayout1);
        QToolButton *refreshButton = new QToolButton();
        refreshButton->setIcon(win->style()->standardIcon(QStyle::SP_BrowserReload));
        refreshButton->setMaximumWidth(50);
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
    cameraLabel->setText("Camera: ");

    listCameraCombo = new QComboBox;

    QLabel *cameraFormatLabel = new QLabel;
    cameraFormatLabel->setText("Format: ");

    listCameraFormatCombo = new QComboBox;



    QPushButton *selectButton = new QPushButton("select");
    selectButton->setStyleSheet("font-size: 20px;");


    cameraSelectLayout->addWidget(cameraLabel);
    cameraSelectLayout->addWidget(listCameraCombo);

    cameraSelectFormatLayout->addWidget(cameraFormatLabel);
    cameraSelectFormatLayout->addWidget(listCameraFormatCombo);


    cameraParamLayout->addLayout(cameraSelectLayout);

    cameraParamLayout->addLayout(cameraSelectFormatLayout);

    cameraParamLayout->addWidget(selectButton);

    connect(selectButton,SIGNAL(clicked()),this,SLOT(onClickSelectCameraButton()));
    connect(listCameraCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectCameraCombo(int)));

    refreshCameraComboBox(camEnumerator);
}

void CameraSelectPage::onClickCameraButton(int i)
{
    win->currentCameraEnumId = i;
    setCameraParamBox(win->listCameraEnumerator[i]);
}

void CameraSelectPage::onSelectCameraCombo(int i)
{
    refreshCameraFormatComboBox();
}

void CameraSelectPage::onClickSelectCameraButton()
{
    win->cameraId = listCameraIds[listCameraCombo->currentIndex()];
    win->currentCameraFormatId = listCameraFormatCombo->currentIndex();

    if(win->isCalibrationSection)
        win->calibrationOptionPage->setPage();
    else {
        //win->recordMixedRealityPage->setPage();
        win->postProcessingOptionPage->setPage(true);
    }
}

void CameraSelectPage::onClickBackToMenuButton()
{
    win->stopQuestCommunicator();
    win->firstMenuPage->setPage();
}
