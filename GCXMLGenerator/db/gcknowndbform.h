#ifndef GCKNOWNDBFORM_H
#define GCKNOWNDBFORM_H

#include <QDialog>

/*--------------------------------------------------------------------------------------*/
/*
  Populates a combo box with the list of known database connections and allows the user to select
  one from the dropdown or to add new or existing database connections (the relevant buttons will
  be shown and/or others hidden depending on which "Buttons" value is provided in the constructor).

  Selecting a database or adding new or existing connections result in the dbSelected() signal being
  emitted.  If the "ToRemove" option was set in the constructor, the dbRemoved() signal will be emitted
  (in both cases with the name of the database connection in question). */

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

  explicit GCKnownDBForm( const QStringList &dbList, Buttons buttons, QWidget *parent );
  ~GCKnownDBForm();

signals:
  void dbSelected( const QString& );
  void dbRemoved ( const QString& );
  void existingConnection();
  void newConnection();
  void userCancelled();
  
private:
  Ui::GCKnownDBForm *ui;
  Buttons            m_buttons;
  bool               m_remove;

private slots:
  void select();
};

#endif // GCKNOWNDBFORM_H
