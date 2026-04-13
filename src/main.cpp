#include <QApplication>

#include "main_widget.h"

int main(int argc, char *argv[]) {

  // Basic setups
  QApplication app(argc, argv);

  MainWidget window;
  window.show();

  return app.exec();
}
