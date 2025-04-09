#include "portaldemo.h"
#include <QApplication>
#include <chrono>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  PortalDemo demo;
  demo.show();
  printf("prepare to show: %ld\n",
         std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count());
  return QApplication::exec();
}
