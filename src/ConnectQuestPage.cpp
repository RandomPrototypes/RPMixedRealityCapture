#include "mainwindow.h"
#include "ConnectQuestPage.h"
#include "CameraSelectPage.h"
#include <QMessageBox>

ConnectQuestPage::ConnectQuestPage(MainWindow *win)
    :win(win)
{
}

void ConnectQuestPage::setPage()
{
    win->currentPageName = MainWindow::PageName::connectToQuest;
    win->clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(300, 0, 300, 200);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Enter the ip address of the quest...");
    QFont font;
    font.setPointSize(20);
    textLabel->setFont(font);

    QHBoxLayout *ipAddressLayout = new QHBoxLayout();
    ipAddressLayout->setContentsMargins(10,50,10,50);
    QLabel *ipAddressLabel = new QLabel;
    ipAddressLabel->setText("ip address: ");
    ipAddressLabel->setStyleSheet("font-size: 20px;");
    ipAddressField = new QLineEdit();
    ipAddressField->setFixedWidth(200);
    ipAddressField->setStyleSheet("font-size: 20px;");
    ipAddressField->setText(win->questIpAddress.c_str());//"192.168.10.105");
    ipAddressLayout->addWidget(ipAddressLabel);
    ipAddressLayout->addWidget(ipAddressField);


    connectButton = new QPushButton(win->isCalibrationSection ? "connect" : "next");
    connectButton->setStyleSheet("font-size: 20px;");

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(ipAddressLayout);
    layout->addWidget(connectButton,Qt::AlignCenter);

    connect(connectButton, SIGNAL(clicked()), this, SLOT(onClickConnectButton()));

    win->mainWidget->setLayout(layout);
}

void ConnectQuestPage::onClickConnectButton()
{
    win->questIpAddress = ipAddressField->text().toStdString();
    if(win->isCalibrationSection) {
        connectButton->setText("connecting...");
        connectButton->setEnabled(false);
        win->questConnectionStatus = MainWindow::QuestConnectionStatus::Connecting;

        win->startQuestCommunicator();
    } else {
        win->cameraSelectPage->setPage();
    }
}

void ConnectQuestPage::onTimer()
{
    if(win->questConnectionStatus == MainWindow::QuestConnectionStatus::ConnectionFailed)
    {
        QMessageBox msgBox;
        msgBox.setText("Can not connect to the quest...");
        msgBox.exec();
        win->cameraSelectPage->setPage();
    } else if(win->questConnectionStatus == MainWindow::QuestConnectionStatus::Connected) {
        win->cameraSelectPage->setPage();
    }
}
