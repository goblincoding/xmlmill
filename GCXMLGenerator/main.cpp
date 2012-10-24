#include <QtGui/QApplication>
#include "gcmainwindow.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  GCMainWindow w;
  w.show();
  
  return a.exec();
}
