#include "portaldemo.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  PortalDemo demo;
  demo.show();
  return QApplication::exec();
}
