/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "XML Mill".
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

#ifndef GCREMOVEITEMSFORM_H
#define GCREMOVEITEMSFORM_H

#include <QDialog>
#include <QList>

namespace Ui
{
  class GCRemoveItemsForm;
}

class QTreeWidgetItem;

/*------------------------------------------------------------------------------------------

  This form allows the user to remove items from the active database.

------------------------------------------------------------------------------------------*/

class GCRemoveItemsForm : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCRemoveItemsForm( QWidget *parent = 0 );
  ~GCRemoveItemsForm();

private slots:
  void treeWidgetItemSelected( QTreeWidgetItem *item, int column );
  void attributeActivated     ( const QString &attribute );
  void deleteElement          ( const QString &element = QString() );
  void removeChildElement     ();
  void updateAttributeValues  ();
  void deleteAttribute        ();
  void showElementHelp        ();
  void showAttributeHelp      ();
  
private:
  void updateChildLists();
  void populateTreeWidget();
  void processNextElement ( const QString &element, QTreeWidgetItem *parent );
  void showErrorMessageBox( const QString &errorMsg );

  Ui::GCRemoveItemsForm *ui;
  QString m_currentElement;
  QString m_currentElementParent;
  QString m_currentAttribute;
  QList< QString > m_deletedElements;
};

#endif // GCREMOVEITEMSFORM_H
