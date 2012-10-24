#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"

GCMainWindow::GCMainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::GCMainWindow)
{
  ui->setupUi(this);
}

GCMainWindow::~GCMainWindow()
{
  delete ui;
}
