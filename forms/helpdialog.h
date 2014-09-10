/* Copyright (c) 2012 - 2015 by William Hallatt.
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
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

namespace Ui {
class HelpDialog;
}

/// Displays "Help" information.

/** The Qt::WA_DeleteOnClose flag is set for all instances of this form.  If
 * you're unfamiliar with Qt, this means that Qt will delete this widget as soon
 * as the widget accepts the close event (i.e. you don't need to worry about
 * clean-up of dynamically created instances of this object). */
class HelpDialog : public QDialog {
  Q_OBJECT

public:
  /*! Constructor.
      @param text - the "Help" text that should be displayed. */
  explicit HelpDialog(const QString &text, QWidget *parent = 0);

  /*! Destructor. */
  ~HelpDialog();

private:
  Ui::HelpDialog *ui;
};

#endif // HELPDIALOG_H
