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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef ADDITEMSFORM_H
#define ADDITEMSFORM_H

#include <QDialog>

namespace Ui {
class AddItemsForm;
}

/// Allows the user to add elements and attributes to the active database.

/** This form allows the user to add new elements and their associated
 * attributes to the database.  Although only one element can be added at a time
 * (with or without attributes), all an element's attributes can be provided in
 * one go through simply ensuring that each attribute appears on its own line in
 * the input text edit.  The user will also be allowed to continue adding
 * elements until "Done" is selected.  Finally, the Qt::WA_DeleteOnClose flag is
 * set for all instances of this form.  If you're not familiar with Qt, this
 * means that Qt will delete this widget as soon as the widget accepts the close
 * event (i.e. you don't need to worry about clean-up of dynamically created
 * instances of this object). */
class AddItemsForm : public QDialog {
  Q_OBJECT

public:
  /*! Constructor. */
  explicit AddItemsForm(QWidget *parent = 0);

  /*! Destructor. */
  ~AddItemsForm();

private slots:
  /*! Triggered when the "Add" button is clicked.  The new element will be added
   * as a first level child of the representative selected tree widget item if
   * such an item exists, or as a new root element if it doesn't. */
  void addElementAndAttributes();

  /*! Disables the line edit when an existing element is selected in the drop
   * down. */
  void comboValueChanged(const QString &element);

  /*! Displays help specific to this form. */
  void showHelp();

private:
  /*! Populates the combo box with all the element names known to the active
   * database. */
  void populateCombo();

  Ui::AddItemsForm *ui;
};

#endif // ADDITEMSFORM_H
