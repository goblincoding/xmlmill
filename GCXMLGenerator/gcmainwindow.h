#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
  class GCMainWindow;
}

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow(QWidget *parent = 0);
  ~GCMainWindow();
  
private:
  Ui::GCMainWindow *ui;
};

#endif // GCMAINWINDOW_H
