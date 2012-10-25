#ifndef GCSESSIONDBFORM_H
#define GCSESSIONDBFORM_H

#include <QDialog>

namespace Ui {
  class GCSessionDBForm;
}

class GCSessionDBForm : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCSessionDBForm( QStringList dbList, QWidget *parent = 0 );
  ~GCSessionDBForm();

signals:
  void dbSelected   ( QString );
  void newConnection( QString );
  void userCancelled();
  
private:
  Ui::GCSessionDBForm *ui;

private slots:
  void open();
  void addNew();
};

#endif // GCSESSIONDBFORM_H
