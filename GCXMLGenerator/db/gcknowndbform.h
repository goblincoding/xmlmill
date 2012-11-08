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
  enum Buttons
  {
    SelectOnly,
    SelectAndExisting,
    ToRemove,
    ShowAll
  };

  explicit GCKnownDBForm( QStringList dbList, Buttons buttons, QWidget *parent );
  ~GCKnownDBForm();

signals:
  void dbSelected( QString );
  void dbRemoved ( QString );
  void newConnection     ();
  void existingConnection();
  void userCancelled     ();
  
private:
  Ui::GCKnownDBForm *ui;
  Buttons            m_buttons;
  bool               m_remove;

private slots:
  void select();
};

#endif // GCKNOWNDBFORM_H
