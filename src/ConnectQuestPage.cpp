#include "mainwindow.h"
#include "ConnectQuestPage.h"
#include "CameraSelectPage.h"
#include "FirstMenuPage.h"
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

    QHBoxLayout *bottomHLayout = new QHBoxLayout();

    connectButton = new QPushButton(win->isCalibrationSection ? "connect" : "next");
    connectButton->setStyleSheet("font-size: 20px;");
    bottomHLayout->addWidget(connectButton);

    bottomHLayout->addSpacing(50);

    QPushButton *backToMenuButton = new QPushButton("Back to menu");
    backToMenuButton->setMaximumWidth(500);
    backToMenuButton->setStyleSheet("font-size: 20px;");
    //layout->setAlignment(backToMenuButton, Qt::AlignHCenter);
    bottomHLayout->addWidget(backToMenuButton);

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(ipAddressLayout);
    layout->addLayout(bottomHLayout,Qt::AlignCenter);

    connect(connectButton, SIGNAL(clicked()), this, SLOT(onClickConnectButton()));
    connect(backToMenuButton,SIGNAL(clicked()),this,SLOT(onClickBackToMenuButton()));

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
        //win->cameraSelectPage->setPage();
        win->questConnectionStatus = MainWindow::QuestConnectionStatus::NotConnected;
        connectButton->setText(win->isCalibrationSection ? "connect" : "next");
        connectButton->setEnabled(true);
    } else if(win->questConnectionStatus == MainWindow::QuestConnectionStatus::Connected) {
        win->cameraSelectPage->setPage();
    }
}


void ConnectQuestPage::onClickBackToMenuButton()
{
    win->stopQuestCommunicator();
    win->firstMenuPage->setPage();
}
