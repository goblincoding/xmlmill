/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "GoblinCoding's XML Studio".
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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "gcmessagespace.h"
#include <QSettings>

/*--------------------------------------------------------------------------------------*/

namespace GCMessageSpace
{
  /* Unnamed namespace hides our "member" variables. */
  namespace
  {
    QSettings settings( "William Hallatt", "GoblinCoding's XML Studio" );
    bool settingsInitialised( false );
  }

  /*------------------------------------------------------------------------------------*/
  /* Uses QSettings to save the user preference to the registry (Windows) or
    relevant XML files (Mac) or ini (Unix). */
  bool userAccepted( const QString &uniqueMessageKey,
                     const QString &heading,
                     const QString &text,
                     GCMessageDialog::ButtonCombo buttons,
                     GCMessageDialog::Buttons defaultButton,
                     GCMessageDialog::Icon icon,
                     bool saveCancel )
  {
    if( !settingsInitialised )
    {
      settings.setValue( "Messages", "Save dialog prompt user preferences." );
      settingsInitialised = true;
    }

    /* Check if the user previously requested that his/her choice must be saved. */
    QString key = QString( "Messages/%1" ).arg( uniqueMessageKey );
    QString valueKey = QString( key + "/Preference" );

    bool remembered = settings.value( key, false ).toBool();

    if( !remembered )
    {
      GCMessageDialog dialog( &remembered, heading, text, buttons, defaultButton, icon );
      QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog.exec() );

      if( accept == QDialog::Accepted )
      {
        if( remembered )
        {
          settings.setValue( key, true );
          settings.setValue( valueKey, true );
        }

        return true;
      }
      else
      {
        /* For some message prompts, it isn't desirable to save "Cancel" values and
          it is therefore necessary to ensure that none of these values are remembered
          by accident. */
        if( remembered )
        {
          if( saveCancel )
          {
            settings.setValue( key, true );
            settings.setValue( valueKey, false );
          }
          else
          {
            settings.setValue( key, false );
          }
        }

        return false;
      }
    }
    else
    {
      /* If we do have a remembered setting, act accordingly. */
      return settings.value( valueKey ).toBool();
    }
  }

  /*------------------------------------------------------------------------------------*/
  void forgetAllPreferences()
  {
    settings.beginGroup( "Messages" );
    settings.remove( "" );
    settings.endGroup();
  }
}
