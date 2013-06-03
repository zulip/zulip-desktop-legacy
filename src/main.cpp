#include "HumbugWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  HumbugWindow w;
  w.show();

  return a.exec();
}
