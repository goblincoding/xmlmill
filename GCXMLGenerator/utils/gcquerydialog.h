#ifndef GCQUERYDIALOG_H
#define GCQUERYDIALOG_H

#include <QDialog>

namespace Ui {
  class GCQueryDialog;
}

class GCQueryDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCQueryDialog( QWidget *parent = 0 );
  ~GCQueryDialog();
  
private:
  Ui::GCQueryDialog *ui;
};

#endif // GCQUERYDIALOG_H
