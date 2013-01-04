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

/// Provides a user dialog prompt with the option to save the user's preference.
namespace Ui
{
  class GCMessageDialog;
}

class GCMessageDialog : public QDialog
{
  Q_OBJECT
  
public:

  /*! Determines the type of icon that will be set on the message dialog. */
  enum Icon
  {
    NoIcon,       /*!< No icon will be shown. */
    Information,  /*!< The message is of an informative nature. */
    Warning,      /*!< The message is a warning. */
    Critical,     /*!< The message contains critical information. */
    Question      /*!< The message is a question and requires user input. */
  };

  /*! Determines the combination of buttons that will be shown. */
  enum ButtonCombo
  {
    OKOnly,   /*!< Only the "OK" button should be made available. */
    YesNo,    /*!< The buttons shown should be "Yes" and "No". */
    OKCancel  /*!< The buttons shown should be "OK" and "Cancel". */
  };

  /*! Represents the individual buttons available. */
  enum Buttons
  {
    Yes,
    No,
    OK,
    Cancel
  };

  /*! Constructor.
      @param remember - this flag should be passed in from the calling object and will be set
                        when the user checks the relevant box
      @param heading - the message box header
      @param text - the actual message text
      @param buttons - the buttons that should be displayed for this particular message
      @param defaultButton - the button that should be highlighted as the default
      @param icon - the icon associated with this particular message. */
  explicit GCMessageDialog( bool *remember,
                            const QString &heading,
                            const QString &text,
                            ButtonCombo buttons,
                            Buttons defaultButton,
                            Icon icon = NoIcon );

  /*! Destructor. */
  ~GCMessageDialog();

private slots:
  /*! Triggered when the user checks or unchecks the "Don't ask me again" box. */
  void setRememberUserChoice( bool remember );
  
private:
  Ui::GCMessageDialog *ui;
  bool *m_remember;
};

#endif // GCMESSAGEDIALOG_H
