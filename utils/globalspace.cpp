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

#include "utils/globalspace.h"
#include <QSettings>
#include <QDir>

/*--------------------------------------------------------------------------------------*/

namespace GlobalSpace
{
  namespace
  {
    const QString HELP = "showHelpButtons";
    const QString VERBOSE = "showTreeItemsVerbose";
    const QString LAST_DIR = "lastDirectory";
    const QString GEOMETRY = "geometry";
    const QString STATE = "windowState";
    const QString USE_DARK = "useDarkTheme";
    const QString SAVE_WINDOW = "saveWindowInformation";
  }

  /*--------------------------------------------------------------------------------------*/

  bool showHelpButtons()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( HELP, true ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setShowHelpButtons( bool show )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( HELP, show );
  }

  /*--------------------------------------------------------------------------------------*/

  bool showTreeItemsVerbose()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( VERBOSE, false ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setShowTreeItemsVerbose( bool show )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( VERBOSE, show );
  }

  /*--------------------------------------------------------------------------------------*/

  QString lastUserSelectedDirectory()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( GlobalSpace::LAST_DIR, QDir::homePath() ).toString();
  }

  /*--------------------------------------------------------------------------------------*/

  void setLastUserSelectedDirectory( const QString& dir )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( LAST_DIR, dir );
  }

  /*--------------------------------------------------------------------------------------*/

  QByteArray windowGeometry()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( GEOMETRY ).toByteArray();
  }

  /*--------------------------------------------------------------------------------------*/

  void setWindowGeometry( const QByteArray& geometry )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( GEOMETRY, geometry );
  }

  /*--------------------------------------------------------------------------------------*/

  QByteArray windowState()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( STATE ).toByteArray();
  }

  /*--------------------------------------------------------------------------------------*/

  void setWindowState( const QByteArray& state )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( STATE, state );
  }

  /*--------------------------------------------------------------------------------------*/

  void removeWindowInfo()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );

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
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( USE_DARK, false ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setUseDarkTheme( bool use )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( USE_DARK, use );
  }

  /*--------------------------------------------------------------------------------------*/

  bool useWindowSettings()
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    return settings.value( SAVE_WINDOW, true ).toBool();
  }

  /*--------------------------------------------------------------------------------------*/

  void setUseWindowSettings( bool use )
  {
    QSettings settings( GlobalSpace::ORGANISATION, GlobalSpace::APPLICATION );
    settings.setValue( SAVE_WINDOW, use );
  }
}

/*--------------------------------------------------------------------------------------*/
