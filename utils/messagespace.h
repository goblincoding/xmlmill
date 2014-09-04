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

#ifndef MESSAGESPACE_H
#define MESSAGESPACE_H

#include <QString>

class QWidget;

/*! Responsible for the display of error messages and messages requiring user input with the
    option to remember the user's preference.

    Some message prompts displayed via this namespace contain the option to remember the user's
    preference.  In cases where a user preference can be saved, MessageSpace will persist the changes
    to whatever medium exists on the platform it's running on (Windows registry, Mac XML, Unix ini).

    This space is furthermore responsible for the display of all error messages, but not ALL messages
    (some messages always need to be shown and make more sense implemented in their respective classes)
*/
namespace MessageSpace
{
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

  /*! This function will return the saved user preference (if there is one),
      or prompt the user for a decision and return the user's choice.
      @param uniqueMessageKey - a unique name representing a specific message, this name is saved to
                                the registry/xml/ini file
      @param heading - the message box header
      @param text - the actual message text
      @param buttons - the buttons that should be displayed for this particular message
      @param defaultButton - the button that should be highlighted as the default
      @param icon - the icon associated with this particular message
      @param saveCancel - if this value is set to "false", "Cancel"-ed user preferences
                          will not be saved, irrespective of whether or not the user
                          ticked the relevant box. */
  bool userAccepted( const QString& uniqueMessageKey,
                     const QString& heading,
                     const QString& text,
                     ButtonCombo buttons,
                     Buttons defaultButton,
                     Icon icon = NoIcon,
                     bool saveCancel = true );

  /*! Deletes all saved dialog preferences from the registry/XML/ini files. */
  void forgetAllPreferences();

  /*! Displays a modal error message box with "message". */
  void showErrorMessageBox( QWidget* parent, const QString& message );
}

#endif // MESSAGESPACE_H