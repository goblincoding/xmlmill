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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>
#include <QFile>
#include <QTextStream>

/// Contains values and functions used throughout the application.

namespace GlobalSettings {
/*----------------------------------------------------------------------------*/

/*! Used when saving and loading settings to registry/XML/ini. */
const QString ORGANISATION = "GoblinCoding";

/*! Used when saving and loading settings to registry/XML/ini. */
const QString APPLICATION = "XML Mill";

/*----------------------------------------------------------------------------*/

/*! Used by various forms to determine whether or not they must display their
 * "Help" tool buttons. */
bool showHelpButtons();

/*! Saves the user's "Help" button preference to the registry/ini/xml. */
void setShowHelpButtons(bool show);

/*----------------------------------------------------------------------------*/

/*! Used by TreeWidgetItem to determine whether or not it should show its
 * element as "verbose". */
bool showTreeItemsVerbose();

/*! Saves the user's tree item verbosity preference to the registry/ini/xml. */
void setShowTreeItemsVerbose(bool show);

/*----------------------------------------------------------------------------*/

/*! Returns the last directory that the user navigated to through dialogs, etc.
 */
QString lastUserSelectedDirectory();

/*! Saves the last directory the user navigated to through dialogs, etc to the
 * registry/ini/xml. */
void setLastUserSelectedDirectory(const QString &dir);

/*----------------------------------------------------------------------------*/

/*! Returns the window geometry settings. */
QByteArray windowGeometry();

/*! Saves the window geometry settings to the registry/ini/xml. */
void setWindowGeometry(const QByteArray &geometry);

/*! Returns the window state settings. */
QByteArray windowState();

/*! Saves the window state settings to the registry/ini/xml. */
void setWindowState(const QByteArray &state);

/*! Deletes all saved window state and geometry information from the registry.
 */
void removeWindowInfo();

/*----------------------------------------------------------------------------*/

/*! Returns "true" if the application needs to have the dark theme set. */
bool useDarkTheme();

/*! Saves the dark theme state to the registry/ini/xml. */
void setUseDarkTheme(bool use);

/*! Returns "true" if the application needs to save window settings. */
bool useWindowSettings();

/*! Saves the windows settings state to the registry/ini/xml. */
void setUseWindowSettings(bool use);

/*----------------------------------------------------------------------------*/

/*! Default font for displaying XML content (directly or via table and tree
 * views). */
const QString FONT = "Courier New";

/*! Default font size for displaying XML content (directly or via table and tree
 * views). */
const int FONTSIZE = 10;

/*----------------------------------------------------------------------------*/

const QString DB_NAME = "profiles";
}

#endif // GLOBALS_H
