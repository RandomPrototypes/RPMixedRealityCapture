#ifndef CONNECTQUESTPAGE_H
#define CONNECTQUESTPAGE_H

#include "mainwindow.h"


class ConnectQuestPage : public QObject
{
    Q_OBJECT
public:
    ConnectQuestPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickConnectButton();
    void onClickBackToMenuButton();

private:
    MainWindow *win;
    QLineEdit *ipAddressField;
    QPushButton *connectButton;
};
#endif // CONNECTQUESTPAGE_H
