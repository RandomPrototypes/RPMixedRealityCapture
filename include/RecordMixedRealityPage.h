#ifndef RECORDMIXEDREALITYPAGE_H
#define RECORDMIXEDREALITYPAGE_H

#include "mainwindow.h"

class RecordMixedRealityPage : public QObject
{
    Q_OBJECT
public:
    RecordMixedRealityPage(MainWindow *win);

    void setPage();
    void onTimer();
public slots:
    void onClickStartRecordingButton();
private:
    MainWindow *win;
    QPushButton *startRecordingButton;
};

#endif // RECORDMIXEDREALITYPAGE_H
