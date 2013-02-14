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

#ifndef GCGLOBALS_H
#define GCGLOBALS_H

#include <QString>
#include <QFile>
#include <QTextStream>

/// Contains values used throughout the application.

namespace GCGlobalSpace
{
  /*--------------------------------------------------------------------------------------*/

  /*! Used by various forms to determine whether or not they must display their "Help" tool buttons. */
  bool showHelpButtons();

  /*! Saves the user's preference to the registry/ini/xml. */
  void setShowHelpButtons( bool show );

  /*--------------------------------------------------------------------------------------*/

  /*! Used when saving and loading settings to registry/XML/ini, used in GCMainWindow and GCMessageSpace. */
  const QString ORGANISATION = "William Hallatt";

  /*! Used when saving and loading settings to registry/XML/ini, used in GCMainWindow and GCMessageSpace. */
  const QString APPLICATION  = "XML Mill";

  /*--------------------------------------------------------------------------------------*/

  /*! Default font when displaying XML content, used in GCMainWindow and GCRestoreFilesForm. */
  const QString FONT = "Courier New";

  /*! Default font when displaying XML content, used in GCMainWindow and GCRestoreFilesForm. */
  const int FONTSIZE = 10;

  /*--------------------------------------------------------------------------------------*/
}

#endif // GCGLOBALS_H
