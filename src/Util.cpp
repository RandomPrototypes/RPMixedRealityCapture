#include "Util.hpp"
#include <QWidget>
#include <chrono>

void clearLayout(QLayout *layout) {
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while (auto item = layout->takeAt(0)) {
      delete item->widget();
      clearLayout(item->layout());
   }
}

std::string getCurrentDateTimeStr()
{
     std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
     time_t tt = std::chrono::system_clock::to_time_t(now);
     tm local_tm = *localtime(&tt);
     char buffer[255];
     snprintf(buffer, sizeof(buffer), "%04d%02d%02d%02d%02d%02d", local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
     return buffer;
}
