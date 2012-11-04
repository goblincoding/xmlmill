#ifndef GCKNOWNDBFORM_H
#define GCKNOWNDBFORM_H

#include <QDialog>

namespace Ui {
  class GCKnownDBForm;
}

class GCKnownDBForm : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCKnownDBForm( QStringList dbList, bool remove, QWidget *parent = 0 );
  ~GCKnownDBForm();

signals:
  void dbSelected( QString );
  void dbRemoved ( QString );
  void newConnection     ();
  void existingConnection();
  void userCancelled     ();
  
private:
  Ui::GCKnownDBForm *ui;
  bool                m_remove;

private slots:
  void select();
};

#endif // GCKNOWNDBFORM_H
