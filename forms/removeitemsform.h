/* Copyright (c) 2012 - 2013 by William Hallatt.
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

#ifndef REMOVEITEMSFORM_H
#define REMOVEITEMSFORM_H

#include <QDialog>
#include <QList>

namespace Ui
{
  class RemoveItemsForm;
}

class TreeWidgetItem;

/// Allows the user to remove items from the active database.

/** All changes made via this form are irreversible and will be executed immediately against the
    active database.

    The Qt::WA_DeleteOnClose flag is set for all instances of this form.  If you're
    unfamiliar with Qt, this means that Qt will delete this widget as soon as the widget
    accepts the close event (i.e. you don't need to worry about clean-up of dynamically
    created instances of this object).
*/
class RemoveItemsForm : public QDialog
{
Q_OBJECT

public:
  /*! Constructor. */
  explicit RemoveItemsForm( QWidget* parent = 0 );

  /*! Destructor. */
  ~RemoveItemsForm();

  private slots:
  /*! Triggered when an item in the tree widget is clicked.  The element name corresponding to
      the tree widget item is flagged as currently active and the attribute combo box is
      populated with the element's known associated attributes. */
  void elementSelected( TreeWidgetItem* item, int column );

  /*! Triggered when the user clicks the "Delete Element" button. A complete clean-up of everything
      (first level children, attributes, attribute values, etc) is executed recursively for the
      deleted element (in other words, everything related to the element is removed, including
      the entire hierarchy of elements below it). */
  void deleteElement( const QString& element = QString() );

  /*! Triggered when the "Remove Child" button is clicked. The currently active element
      (corresponding to the selected tree widget item) is removed as a first level child
      from its immediate parent element. */
  void removeChildElement();

  /*! Triggered when the current attribute in the attribute combo box changes.  Activation of
      an attribute results in the plain text edit being populated with all the attribute's
      known values. */
  void attributeActivated( const QString& attribute );

  /*! Triggered when the "Update Values" button is clicked.  The attribute values remaining
      in the plain text edit after editing is saved against the selected attribute.  This action
      does not append the values to the existing list, but rather replaces the existing values. */
  void updateAttributeValues();

  /*! Triggered when the "Delete Attribute" button is clicked.  The attribute selected in the
      attribute combo box is deleted from the active element's list of associated attributes. */
  void deleteAttribute();

  /*! Triggered by the "show element help" button.  Displays help information related to actions
      executed against elements. */
  void showElementHelp();

  /*! Triggered by the "show attribute help" button.  Displays help information related to actions
      executed against attributes. */
  void showAttributeHelp();

private:
  /*! Removes a deleted element from all elements that may have it in their first level child lists. */
  void updateChildLists();

  Ui::RemoveItemsForm* ui;
  QString m_currentElement;
  QString m_currentElementParent;
  QString m_currentAttribute;
  QList< QString > m_deletedElements;
};

#endif // REMOVEITEMSFORM_H