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

#include "gcdbsessionmanager.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::GCDBSessionManager( QWidget *parent ) :
  QObject       ( parent ),
  m_parentWidget( parent )
{
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( m_parentWidget, "Add New Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addExistingDB()
{
  QString file = QFileDialog::getOpenFileName( m_parentWidget, "Add Existing Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addDBConnection( const QString &dbName )
{
  if( !GCDataBaseInterface::instance()->addDatabase( dbName ) )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    return;
  }

  /* Check if we have a saved user preference for this situation. Regarding the session keys,
    all keys have to start with "Messages" otherwise the main window won't be able to reset
    the user preferences to the defaults.  An alternative would be to implement a function here
    that does DB Session Manager specific setting resets, but I don't see the need for that. */
  bool accepted = GCMessageSpace::userAccepted( "SetSessionActive",
                                                "Set Session",
                                                "Would you like to set the new profile as active?",
                                                GCMessageDialog::YesNo,
                                                GCMessageDialog::Yes,
                                                GCMessageDialog::Question );

  if( accepted )
  {
    setSessionDB( dbName );
  }
  else
  {
    if( !GCDataBaseInterface::instance()->hasActiveSession() )
    {
      showKnownDBForm( GCKnownDBForm::ShowAll );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setSessionDB( const QString &dbName )
{
  if( !GCDataBaseInterface::instance()->setSessionDB( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
                    .arg( GCDataBaseInterface::instance()->getLastError() );
    showErrorMessageBox( error );
  }
  else
  {
    /* If the user set an empty database, prompt to populate it.  This message must
      always be shown (i.e. we don't have to show the custom dialog box that provides
      the \"Don't show this again\" option). */
    if( GCDataBaseInterface::instance()->knownElements().size() < 1 )
    {
      QMessageBox::warning( m_parentWidget,
                            "Empty Profile",
                            "The current active profile is completely empty (aka \"entirely useless\").\n\n"
                            "You can either:\n"
                            "1. Select a different (populated) profile and continue working, or\n"
                            "2. Switch to \"Super User\" mode and start populating this one." );
    }

    emit dbSessionChanged();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDB()
{
  showKnownDBForm( GCKnownDBForm::ToRemove );
}

/*--------------------------------------------------------------------------------------*/


void GCDBSessionManager::removeDBConnection( const QString &dbName )
{
  if( !GCDataBaseInterface::instance()->removeDatabase( dbName ) )
  {
    QString error = QString( "Failed to remove profile \"%1\": [%2]" ).arg( dbName )
                    .arg( GCDataBaseInterface::instance()->getLastError() );
    showErrorMessageBox( error );
  }

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    QString errMsg( "The active profile has been removed, please set another as active." );
    showErrorMessageBox( errMsg );
    showKnownDBForm( GCKnownDBForm::ShowAll );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::switchDBSession( bool docEmpty )
{
  /* Switching DB sessions while building an XML document could result in all kinds of trouble
    since the items known to the current session may not be known to the next. */
  if( !docEmpty )
  {
    bool accepted = GCMessageSpace::userAccepted( "SwitchingSessionsWarning",
                                                  "Warning!",
                                                  "Switching profile sessions while building an XML document "
                                                  "will cause the document to be reset and your work will be lost. "
                                                  "If this is fine, proceed with \"OK\".\n\n"
                                                  "On the other hand, if you wish to keep your work, please hit \"Cancel\" and "
                                                  "save the document first before coming back here.",
                                                  GCMessageDialog::OKCancel,
                                                  GCMessageDialog::Cancel,
                                                  GCMessageDialog::Warning );

    if( accepted )
    {
      emit reset();
    }
    else
    {
      return;
    }
  }

  showKnownDBForm( GCKnownDBForm::ShowAll );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::showKnownDBForm( GCKnownDBForm::Buttons buttons )
{
  GCKnownDBForm *knownDBForm = new GCKnownDBForm( GCDataBaseInterface::instance()->getDBList(), buttons, m_parentWidget );

  connect( knownDBForm,   SIGNAL( newConnection() ),       this, SLOT( addNewDB() ) );
  connect( knownDBForm,   SIGNAL( existingConnection() ),  this, SLOT( addExistingDB() ) );
  connect( knownDBForm,   SIGNAL( dbSelected( QString ) ), this, SLOT( setSessionDB( QString ) ) );
  connect( knownDBForm,   SIGNAL( dbRemoved ( QString ) ), this, SLOT( removeDBConnection( QString ) ) );

  /* If we don't have an active DB session, it's probably at program
    start-up and the user wishes to exit the application by clicking "Cancel". */
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    connect( knownDBForm, SIGNAL( userCancelled() ), m_parentWidget, SLOT( close() ) );
    knownDBForm->show();
  }
  else
  {
    connect( knownDBForm, SIGNAL( userCancelled() ), this, SIGNAL( userCancelledKnownDBForm() ) );
    knownDBForm->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( m_parentWidget, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
