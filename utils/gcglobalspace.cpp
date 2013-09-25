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

#include "utils/gcglobalspace.h"
#include <QSettings>
#include <QDir>

/*--------------------------------------------------------------------------------------*/

namespace GCGlobalSpace
{
  namespace
  {
    /*! Used when saving and loading settings to registry/XML/ini. */
    const QString ORGANISATION = "GoblinCoding";

    /*! Used when saving and loading settings to registry/XML/ini. */
    const QString APPLICATION = "XML Mill";

    const QString HELP = "showHelpButtons";
    const QString VERBOSE = "showTreeItemsVerbose";
    const QString LAST_DIR = "lastDirectory";
    const QString GEOMETRY = "geometry";
    const QString STATE = "windowState";
    const QString USE_DARK = "useDarkTheme";
    const QString SAVE_WINDOW = "saveWindowInformation";
  }

  /*--------------------------------------------------------------------------------------*/

  const QString& getOrganisationName()
  {
    return ORGANISATION;
  }

  /*--------------------------------------------------------------------------------------*/

  const QString& getApplicationName()
  {
    return APPLICATION;
  }

  /*--------------------------------------------------------------------------------------*/

  bool showHelpButtons()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( HELP, true ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setShowHelpButtons( bool show )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( HELP, show );
  }

  /*--------------------------------------------------------------------------------------*/

  bool showTreeItemsVerbose()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( VERBOSE, false ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setShowTreeItemsVerbose( bool show )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( VERBOSE, show );
  }

  /*--------------------------------------------------------------------------------------*/

  QString lastUserSelectedDirectory()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( GCGlobalSpace::LAST_DIR, QDir::homePath() ).toString();
  }

  /*--------------------------------------------------------------------------------------*/

  void setLastUserSelectedDirectory( const QString& dir )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( LAST_DIR, dir );
  }

  /*--------------------------------------------------------------------------------------*/

  QByteArray windowGeometry()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( GEOMETRY ).toByteArray();
  }

  /*--------------------------------------------------------------------------------------*/

  void setWindowGeometry( const QByteArray& geometry )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( GEOMETRY, geometry );
  }

  /*--------------------------------------------------------------------------------------*/

  QByteArray windowState()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( STATE ).toByteArray();
  }

  /*--------------------------------------------------------------------------------------*/

  void setWindowState( const QByteArray& state )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( STATE, state );
  }

  /*--------------------------------------------------------------------------------------*/

  void removeWindowInfo()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );

    if( settings.contains( GEOMETRY ) )
    {
      settings.remove( GEOMETRY );
    }

    if( settings.contains( STATE ) )
    {
      settings.remove( STATE );
    }
  }

  /*--------------------------------------------------------------------------------------*/

  bool useDarkTheme()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( USE_DARK, false ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setUseDarkTheme( bool use )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( USE_DARK, use );
  }

  /*--------------------------------------------------------------------------------------*/

  bool useWindowSettings()
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    return settings.value( SAVE_WINDOW, true ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setUseWindowSettings( bool use )
  {
    QSettings settings( GCGlobalSpace::ORGANISATION, GCGlobalSpace::APPLICATION );
    settings.setValue( SAVE_WINDOW, use );
  }
}

/*--------------------------------------------------------------------------------------*/