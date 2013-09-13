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

#ifndef GCADDSNIPPETSFORM_H
#define GCADDSNIPPETSFORM_H

#include <QDialog>
#include <QDomElement>

namespace Ui
{
  class GCAddSnippetsForm;
}

class GCTreeWidgetItem;
class QTableWidgetItem;

/// Allows the user to add whole snippets to the active document.

/**
  This form allows the user to add multiple XML snippets of the same structure to the current
  document (with whichever default values the user specifies).  It furthermore allows for the
  option to increment the default values for each snippet (i.e. if the user specifies "1" as
  an attribute value with the option to increment, then the next snippet generated will have
  "2" as the value for the same attribute and so on and so forth (strings will have the
  incremented value appended to the name).  Only one element of each type can be inserted into
  any specific snippet as it makes no sense to insert multiple elements of the same type - for
  those use cases the user must create a smaller snippet subset.

  Also, the Qt::WA_DeleteOnClose flag is set for all instances of this form.  If you're
  unfamiliar with Qt, this means that Qt will delete this widget as soon as the widget
  accepts the close event (i.e. you don't need to worry about clean-up of dynamically
  created instances of this object).
*/
class GCAddSnippetsForm : public QDialog
{
Q_OBJECT

public:
  /*! Constructor.
      @param elementName - the name of the element that will form the basis of the snippet, i.e. this
                           element will be at the top of the snippet's DOM hierarchy.
      @param parentItem - the tree item and corresponding element in the active document to which the
                          snippet will be added. */
  explicit GCAddSnippetsForm( const QString& elementName, GCTreeWidgetItem* parentItem, QWidget* parent = 0 );

  /*! Destructor. */
  ~GCAddSnippetsForm();

signals:
  /*! Informs the listener that a new snippet has been added. The GCTreeWidgetItem thus emitted
      is the item to which the snippet must be added and the QDomElement is the element corresponding
      to this item.  As always, we depend on QDomElement's shallow copy constructor. The GCTreeWidgetItem
      thus emitted is not owned by this class, but is the same one that was passed in as constructor argument. */
  void snippetAdded( GCTreeWidgetItem* parent, QDomElement elementToAdd );

  private slots:
  /*! Triggered when an element is selected in the tree widget.  This function populates the attributes
      table with the known attributes and values associated with the selected element. */
  void elementSelected( GCTreeWidgetItem* item, int column );

  /*! Triggered whenever a user clicks on an attribute in the attribute table, or changes an attribute's
      "include" state. */
  void attributeChanged( QTableWidgetItem* item ) const;

  /*! Triggered whenever an attribute's value changes. */
  void attributeValueChanged() const;

  /*! Triggered whenever the "Add" button is clicked.  This function builds the snippet(s) that must
      be added to the active document and furthermore informs all listeners that a snippet has been added
      via the "snippetAdded" signal. */
  void addSnippet();

  /*! Displays help information for this form. */
  void showHelp();

private:
  /*! Whenever a user checks or unchecks an element to include or exclude it from the snippet being built,
      the element's parent(s) and children need to be updated accordingly.  I.e. including/excluding an
      element must also include/exclude all of its children (and their children, etc) as well as its parent
      (and its parent's parent, etc), for a smooth and intuitive user experience. */
  void updateCheckStates( GCTreeWidgetItem* item ) const;

  Ui::GCAddSnippetsForm* ui;
  GCTreeWidgetItem* m_parentItem;
  bool m_treeItemActivated;
};

#endif // GCADDSNIPPETSFORM_H