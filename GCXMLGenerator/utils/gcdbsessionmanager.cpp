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

#include "gcdbsessionmanager.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"

#include <QFileDialog>
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::GCDBSessionManager( QWidget *parent ) :
  QObject       ( parent ),
  m_parentWidget( parent ),
  m_currentRoot ( "" )
{
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::initialiseSession()
{
  showKnownDBForm( GCKnownDBForm::ShowAll );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::switchActiveDatabase( const QString &currentRoot )
{
  /* Switching DB sessions while building an XML document could result in all kinds of trouble
    since the items known to the current session may not be known to the next. */
  m_currentRoot = currentRoot;
  showKnownDBForm( GCKnownDBForm::ShowAll );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDatabase( const QString &currentRoot )
{
  m_currentRoot = currentRoot;
  showKnownDBForm( GCKnownDBForm::ToRemove );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addExistingDatabase( const QString &currentRoot )
{
  m_currentRoot = currentRoot;

  QString file = QFileDialog::getOpenFileName( m_parentWidget, "Add Existing Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addNewDatabase( const QString &currentRoot )
{
  m_currentRoot = currentRoot;

  QString file = QFileDialog::getSaveFileName( m_parentWidget, "Add New Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDBConnection( const QString &dbName )
{
  if( !m_currentRoot.isEmpty() )
  {
    if( GCDataBaseInterface::instance()->knownRootElements().contains( m_currentRoot ) )
    {
      GCMessageSpace::userAccepted( "RemoveActiveSessionWarning",
                                    "Warning!",
                                    "Removing the active profile will cause the current "
                                    "document to be reset and your work will be lost.\n\n "
                                    "If you are not removing the current profile (it could be "
                                    "that the profile you are removing just happens to know "
                                    "of the same document style you're currently working on), "
                                    "then you can safely ignore this message.",
                                    GCMessageDialog::OKCancel,
                                    GCMessageDialog::Cancel,
                                    GCMessageDialog::Warning );
    }
  }

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
    emit reset();
    m_currentRoot = "";
    QString errMsg( "The active profile has been removed, please select another." );
    showErrorMessageBox( errMsg );
    showKnownDBForm( GCKnownDBForm::ShowAll );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setActiveDatabase( const QString &dbName )
{
  /* If the current root element is not known to the new session, the user must
    confirm whether or not he/she wants the active document to be reset. */
  if( !m_currentRoot.isEmpty() )
  {
    if( !GCDataBaseInterface::instance()->containsKnownRootElement( dbName, m_currentRoot ) )
    {
      QMessageBox::StandardButton accepted = QMessageBox::warning( m_parentWidget,
                                                                   "Warning!",
                                                                   "The new profile doesn't support your current document's\n"
                                                                   "content, switching will cause the document to be reset and\n"
                                                                   "your work will be lost.\n\n"
                                                                   "Proceed?",
                                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                                   QMessageBox::Cancel );

      if( accepted == QMessageBox::Ok )
      {
        return;
      }
      else
      {
        emit reset();
        m_currentRoot = "";
      }
    }
  }

  if( !GCDataBaseInterface::instance()->setActiveDatabase( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
                    .arg( GCDataBaseInterface::instance()->getLastError() );
    showErrorMessageBox( error );
  }
  else
  {
    emit activeDatabaseChanged( dbName );
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

  bool accepted = GCMessageSpace::userAccepted( "SwitchActiveSession",
                                                "Switch Session",
                                                "Would you like to switch to the new profile?",
                                                GCMessageDialog::YesNo,
                                                GCMessageDialog::Yes,
                                                GCMessageDialog::Question );

  if( accepted )
  {
    setActiveDatabase( dbName );
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

void GCDBSessionManager::showKnownDBForm( GCKnownDBForm::Buttons buttons )
{
  GCKnownDBForm *knownDBForm = new GCKnownDBForm( GCDataBaseInterface::instance()->getDBList(), buttons, m_parentWidget );

  connect( knownDBForm, SIGNAL( newConnection() ),      this, SLOT( addNewDatabase() ) );
  connect( knownDBForm, SIGNAL( existingConnection() ), this, SLOT( addExistingDatabase() ) );
  connect( knownDBForm, SIGNAL( selectedDatabase( QString ) ), this, SLOT( setActiveDatabase( QString ) ) );
  connect( knownDBForm, SIGNAL( databaseRemoved ( QString ) ), this, SLOT( removeDBConnection( QString ) ) );

  /* If we don't have an active DB session, it's probably at program
    start-up and the user wishes to exit the application by clicking "Cancel". */
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    connect( knownDBForm, SIGNAL( userCancelled() ), m_parentWidget, SLOT( close() ) );
    knownDBForm->show();
  }
  else
  {
    knownDBForm->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( m_parentWidget, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
