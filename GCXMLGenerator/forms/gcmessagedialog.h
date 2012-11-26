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

#ifndef GCMESSAGEDIALOG_H
#define GCMESSAGEDIALOG_H

#include <QDialog>

/*---------------------------------------------------------------------------------------------------

   GCMessageDialog provides a user dialog prompt with the option to remember the user's preference.

---------------------------------------------------------------------------------------------------*/

namespace Ui
{
  class GCMessageDialog;
}

class GCMessageDialog : public QDialog
{
  Q_OBJECT
  
public:

  enum Icon
  {
    NoIcon,
    Information,
    Warning,
    Critical,
    Question
  };

  enum ButtonCombo
  {
    OKOnly,
    YesNo,
    OKCancel
  };

  enum Buttons
  {
    Yes,
    No,
    OK,
    Cancel
  };

  explicit GCMessageDialog( bool *remember,
                            const QString &heading,
                            const QString &text,
                            ButtonCombo    buttons,
                            Buttons        defaultButton,
                            Icon           icon = NoIcon );
  ~GCMessageDialog();

private slots:
  void setRememberUserChoice( bool remember );
  
private:
  Ui::GCMessageDialog *ui;
  bool *m_remember;
};

#endif // GCMESSAGEDIALOG_H
