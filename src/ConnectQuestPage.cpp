#include "mainwindow.h"

void MainWindow::setConnectToQuestPage()
{
    currentPageName = "connectToQuest";
    clearMainWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(300, 0, 300, 200);

    QLabel *textLabel = new QLabel;

    textLabel->setText("Enter the ip address of the quest...");
    QFont font;
    font.setPointSize(15);
    textLabel->setFont(font);

    QHBoxLayout *ipAddressLayout = new QHBoxLayout();
    ipAddressLayout->setContentsMargins(10,50,10,50);
    QLabel *ipAddressLabel = new QLabel;
    ipAddressLabel->setText("ip address: ");
    ipAddressField = new QLineEdit();
    ipAddressField->setFixedWidth(200);
    ipAddressField->setText("192.168.10.105");
    ipAddressLayout->addWidget(ipAddressLabel);
    ipAddressLayout->addWidget(ipAddressField);

    connectButton = new QPushButton("connect");

    layout->addWidget(textLabel,Qt::AlignCenter);
    layout->addLayout(ipAddressLayout);
    layout->addWidget(connectButton,Qt::AlignCenter);

    connect(connectButton, &QPushButton::released, this, &MainWindow::onClickConnectToQuestButton);

    mainWidget->setLayout(layout);
}

void MainWindow::onClickConnectToQuestButton()
{
    QString ipaddress = ipAddressField->text();
    connectButton->setText("connecting...");
    connectButton->setEnabled(false);
    questConnectionStatus = QuestConnectionStatus::Connecting;

    questCommunicatorThread = new std::thread([&]()
        {
            questCommunicatorThreadFunc();
        }
    );
}
