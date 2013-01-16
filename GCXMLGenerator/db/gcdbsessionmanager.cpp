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
#include "ui_gcdbsessionmanager.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"

#include <QFileDialog>
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::GCDBSessionManager( QWidget *parent ) :
  QDialog      ( parent ),
  ui           ( new Ui::GCDBSessionManager ),
  m_currentRoot( "" )
{
  ui->setupUi( this );

  setDBList();

  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SLOT( addExistingDatabase() ) );
  connect( ui->addNewButton,      SIGNAL( clicked() ), this, SLOT( addNewDatabase() ) );
  connect( ui->cancelButton,      SIGNAL( clicked() ), this, SLOT( reject() ) );
  connect( ui->okButton,          SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ) );
  connect( ui->okButton,          SIGNAL( clicked() ), this, SLOT( accept() ) );
}

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::~GCDBSessionManager()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::selectActiveDatabase()
{
  this->setWindowTitle( "Select a profile for this session" );
  setDBList();

  ui->addNewButton->setVisible( true );
  ui->addExistingButton->setVisible( true );

  disconnect( ui->okButton, SIGNAL( clicked() ), this, SLOT( removeDBConnection() ) );
  connect   ( ui->okButton, SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ), Qt::UniqueConnection );

  this->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::switchActiveDatabase( const QString &currentRoot )
{
  /* Switching DB sessions while building an XML document could result in all kinds of trouble
    since the items known to the current session may not be known to the next. */
  m_currentRoot = currentRoot;
  selectActiveDatabase();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDatabase( const QString &currentRoot )
{
  this->setWindowTitle( "Remove profile" );

  m_currentRoot = currentRoot;
  ui->addNewButton->setVisible( false );
  ui->addExistingButton->setVisible( false );

  disconnect( ui->okButton, SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ) );
  connect   ( ui->okButton, SIGNAL( clicked() ), this, SLOT( removeDBConnection() ), Qt::UniqueConnection );

  this->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addExistingDatabase( const QString &currentRoot )
{
  m_currentRoot = currentRoot;

  QString file = QFileDialog::getOpenFileName( this, "Add Existing Profile", QDir::homePath(), "XML Profiles (*.db)" );

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

  QString file = QFileDialog::getSaveFileName( this, "Add New Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDBConnection()
{
  if( !m_currentRoot.isEmpty() )
  {
    if( GCDataBaseInterface::instance()->knownRootElements().contains( m_currentRoot ) )
    {
      bool accepted = GCMessageSpace::userAccepted( "RemoveActiveSessionWarning",
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

      if( !accepted )
      {
        return;
      }
    }
  }

  QString dbName = ui->comboBox->currentText();

  if( !GCDataBaseInterface::instance()->removeDatabase( dbName ) )
  {
    QString error = QString( "Failed to remove profile \"%1\": [%2]" ).arg( dbName )
                    .arg( GCDataBaseInterface::instance()->getLastError() );
    showErrorMessageBox( error );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setActiveDatabase()
{
  setActiveDatabase( ui->comboBox->currentText() );
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
      QMessageBox::StandardButton accept = QMessageBox::question( this,
                                                                  "Unsupported document",
                                                                  "The new profile doesn't support your current document. The\n"
                                                                  "document will be reset if you continue.",
                                                                  QMessageBox::Ok | QMessageBox::Cancel,
                                                                  QMessageBox::Cancel );
      if( accept != QMessageBox::Ok )
      {
        return;
      }

      emit reset();
      m_currentRoot = "";
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
    this->hide();
    this->accept();
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

  setDBList();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setDBList()
{
  ui->comboBox->clear();

  QStringList dbList = GCDataBaseInterface::instance()->getDBList();

  if( dbList.empty() )
  {
    ui->okButton->setVisible( false );
    ui->comboBox->addItem( "Let there be profiles! (i.e. you'll have to add some)" );
  }
  else
  {
    ui->okButton->setVisible( true );
    ui->comboBox->addItems( dbList );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
