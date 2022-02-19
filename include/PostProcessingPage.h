#ifndef POSTPROCESSINGPAGE_H
#define POSTPROCESSINGPAGE_H

#include "mainwindow.h"

class PostProcessingPage : public QObject
{
    Q_OBJECT
public:
    PostProcessingPage(MainWindow *win);

    void setPage();
    void processOutput();
    void postProcessThreadFunc();
    void onTimer();
private:
    MainWindow *win;
    QProgressBar *progressBar;
    double postProcessVal = 0;
};


#endif // POSTPROCESSINGPAGE_H
