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
  explicit GCSessionDBForm( QStringList dbList, bool remove, QWidget *parent = 0 );
  ~GCSessionDBForm();

signals:
  void dbSelected( QString );
  void dbRemoved ( QString );
  void newConnection     ();
  void existingConnection();
  void userCancelled     ();
  
private:
  Ui::GCSessionDBForm *ui;
  bool                m_remove;

private slots:
  void select();
};

#endif // GCSESSIONDBFORM_H
