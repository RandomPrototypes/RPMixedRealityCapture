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
    win->currentCameraEnumId = -1;
    currentCameraId = -1;
    currentCameraResolutionId = -1;
    currentCameraResolutionId = -1;
    ignoreIndexChangeSignal = false;
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
    camEnumerator->detectCameras();
    listCameraCombo->clear();

    listCameraIds.clear();
    ignoreIndexChangeSignal = true;
    for (size_t i = 0; i < camEnumerator->count(); i++) {
        listCameraIds.push_back(camEnumerator->getCameraId(i));
        qDebug() << "listCameraCombo->addItem(" << camEnumerator->getCameraName(i) << ")";
        listCameraCombo->addItem(QString(camEnumerator->getCameraName(i)));
    }
    ignoreIndexChangeSignal = false;
    //refreshCameraResolutionComboBox();
    emit onSelectCameraCombo(0);
}

int CameraSelectPage::findCameraFormatId(int width, int height, ImageType encoding)
{
    for(size_t i = 0; i < win->listCameraFormats.size(); i++) {
        if(win->listCameraFormats[i].width == width && win->listCameraFormats[i].height == height && win->listCameraFormats[i].type == encoding)
            return i;
    }
    return -1;
}

void CameraSelectPage::refreshCameraResolutionComboBox()
{
    qDebug() << "refreshCameraResolutionComboBox";
    listCameraResolutionCombo->clear();
    listCameraEncodingCombo->clear();
    if(currentCameraId >= 0 && currentCameraId < listCameraIds.size()) {
        std::shared_ptr<CameraInterface> cam = getCameraInterface(win->listCameraEnumerator[win->currentCameraEnumId]->getBackend());
        qDebug() << "cam->open(" << listCameraIds[currentCameraId].c_str() << ")";
        if(!cam->open(listCameraIds[currentCameraId].c_str()))
        {
            qDebug() << "!cam->open : " << cam->getErrorMsg();
            return ;
        }
        win->listCameraFormats = cam->getListAvailableFormat();
        listResolution = getListResolution(win->listCameraFormats);
        ignoreIndexChangeSignal = true;
        for (size_t i = 0; i < listResolution.size(); i++) {
            std::string res = std::to_string(listResolution[i].width) + "x" + std::to_string(listResolution[i].height);
            qDebug() << "listCameraResolutionCombo->addItem(" << res.c_str() << ")";
            listCameraResolutionCombo->addItem(QString(res.c_str()));
        }
        ignoreIndexChangeSignal = false;
        cam->close();
        //currentCameraResolutionId = 0;
        //refreshCameraEncodingComboBox();
        emit onSelectResolutionCombo(0);
    }
}

void CameraSelectPage::refreshCameraEncodingComboBox()
{
    qDebug() << "refreshCameraEncodingComboBox";
    listCameraEncodingCombo->clear();
    if(currentCameraResolutionId >= 0 && currentCameraResolutionId < listResolution.size() ) {
        ImageFormat resolution = listResolution[currentCameraResolutionId];
        listEncoding = getListImageType(win->listCameraFormats, resolution.width, resolution.height);
        for (size_t i = 0; i < listEncoding.size(); i++) {
            if(listEncoding[i] == ImageType::JPG) {
                listEncoding.erase(listEncoding.begin()+i);
                listEncoding.insert(listEncoding.begin(), ImageType::JPG);
                break;
            }
        }
        ignoreIndexChangeSignal = true;
        for (size_t i = 0; i < listEncoding.size(); i++) {
            ImageType encoding = listEncoding[i];
            qDebug() << "listCameraEncodingCombo->addItem(" << toString(listEncoding[i]).c_str() << ")";
            listCameraEncodingCombo->addItem(QString(toString(listEncoding[i]).c_str()));
        }
        ignoreIndexChangeSignal = false;
        currentCameraEncodingId = 0;
        if(listEncoding.size() > 0)
            win->currentCameraFormatId = findCameraFormatId(resolution.width, resolution.height, listEncoding[0]);
       else win->currentCameraFormatId = 0;
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

    listCameraResolutionCombo = new QComboBox;
    listCameraEncodingCombo = new QComboBox;



    QPushButton *selectButton = new QPushButton("select");
    selectButton->setStyleSheet("font-size: 20px;");


    cameraSelectLayout->addWidget(cameraLabel);
    cameraSelectLayout->addWidget(listCameraCombo);

    cameraSelectFormatLayout->addWidget(cameraFormatLabel);
    cameraSelectFormatLayout->addWidget(listCameraResolutionCombo);
    cameraSelectFormatLayout->addWidget(listCameraEncodingCombo);


    cameraParamLayout->addLayout(cameraSelectLayout);

    cameraParamLayout->addLayout(cameraSelectFormatLayout);

    cameraParamLayout->addWidget(selectButton);

    connect(selectButton,SIGNAL(clicked()),this,SLOT(onClickSelectCameraButton()));
    connect(listCameraCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectCameraCombo(int)));
    connect(listCameraResolutionCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectResolutionCombo(int)));
    connect(listCameraEncodingCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onSelectEncodingCombo(int)));

    refreshCameraComboBox(camEnumerator);
}

void CameraSelectPage::onClickCameraButton(int i)
{
    if(ignoreIndexChangeSignal)
        return;
    if(win->currentCameraEnumId != i) {
        qDebug() << "onClickCameraButton";
        win->currentCameraEnumId = i;
        currentCameraId = -1;
        currentCameraResolutionId = -1;
        currentCameraEncodingId = -1;
        setCameraParamBox(win->listCameraEnumerator[i]);
    } else {
        qDebug() << "onClickCameraButton skipped because same index";
    }
}

void CameraSelectPage::onSelectCameraCombo(int i)
{
    if(ignoreIndexChangeSignal)
        return;
    if(currentCameraId != i) {
        qDebug() << "onSelectCameraCombo";
        currentCameraId = i;
        currentCameraResolutionId = -1;
        currentCameraEncodingId = -1;
        refreshCameraResolutionComboBox();
    } else {
        qDebug() << "onSelectCameraCombo skipped because same index";
    }
}

void CameraSelectPage::onSelectResolutionCombo(int i)
{
    if(ignoreIndexChangeSignal)
        return;
    if(currentCameraResolutionId != i) {
        qDebug() << "onSelectResolutionCombo";
        currentCameraResolutionId = i;
        currentCameraEncodingId = -1;
        refreshCameraEncodingComboBox();
    } else {
        qDebug() << "onSelectResolutionCombo skipped because same index";
    }
}

void CameraSelectPage::onSelectEncodingCombo(int i)
{
    if(ignoreIndexChangeSignal)
        return;
    if(currentCameraEncodingId != i) {
        qDebug() << "onSelectEncodingCombo";
        currentCameraEncodingId = i;
    } else {
        qDebug() << "onSelectEncodingCombo skipped because same index";
    }
}

void CameraSelectPage::onClickSelectCameraButton()
{
    win->cameraId = listCameraIds[currentCameraId];
    ImageFormat resolution = listResolution[currentCameraResolutionId];
    ImageType type = listEncoding[currentCameraEncodingId];
    win->currentCameraFormatId = findCameraFormatId(resolution.width, resolution.height, type);
    qDebug() << "selected " << resolution.width << "x" << resolution.height << " : " << toString(type).c_str();
    ImageFormat selectedFormat = win->listCameraFormats[win->currentCameraFormatId];
    qDebug() << "match " << win->currentCameraFormatId << " : " << selectedFormat.toString().c_str();

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
