#include <QApplication>
#include "portaldemo.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  PortalDemo demo;
  demo.show();
  return QApplication::exec();
}
