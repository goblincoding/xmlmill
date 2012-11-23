/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "GoblinCoding's XML Studio".
 *
 * The official website for this project is <http://www.goblincoding.com> and,
 * although not compulsory, it would be appreciated if all works of whatever
 * nature using this source code (in whole or in part) include a reference to
 * this site.
 *
 * Should you wish to contact me for whatever reason, please do so via:
 *
 *                 <http://www.goblincoding.com/contact>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef GCKNOWNDBFORM_H
#define GCKNOWNDBFORM_H

#include <QDialog>

/*---------------------------------------------------------------------------------------------------

  Populates a combo box with the list of known database connections and allows the user to select
  one from the dropdown or to add new or existing database connections (the relevant buttons will
  be shown and/or others hidden depending on which "Buttons" value is provided in the constructor).

  Selecting a database or adding new or existing connections result in the dbSelected() signal being
  emitted.  If the "ToRemove" option was set in the constructor, the dbRemoved() signal will be emitted
  (in both cases with the name of the database connection in question).

---------------------------------------------------------------------------------------------------*/

namespace Ui
{
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
