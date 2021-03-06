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

#include "gcdbsessionmanager.h"
#include "ui_gcdbsessionmanager.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"
#include "utils/gcglobalspace.h"

#include <QFileDialog>
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::GCDBSessionManager( QWidget* parent )
: QDialog      ( parent ),
  ui           ( new Ui::GCDBSessionManager ),
  m_currentRoot( "" )
{
  ui->setupUi( this );

  setDatabaseList();
  ui->showHelpButton->setVisible( false );

  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SLOT( addExistingDatabase() ) );
  connect( ui->addNewButton, SIGNAL( clicked() ), this, SLOT( addNewDatabase() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
  connect( ui->okButton, SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ) );
  connect( ui->okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
}

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager::~GCDBSessionManager()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::selectActiveDatabase( const QString& currentRoot )
{
  /* Switching DB sessions while building an XML document could result in all kinds of trouble
    since the items known to the current session may not be known to the next. */
  m_currentRoot = currentRoot;

  this->setWindowTitle( "Select a profile for this session" );
  setDatabaseList();

  ui->addNewButton->setVisible( true );
  ui->addExistingButton->setVisible( true );

  disconnect( ui->okButton, SIGNAL( clicked() ), this, SLOT( removeDatabaseConnection() ) );
  connect( ui->okButton, SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ), Qt::UniqueConnection );

  this->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDatabase( const QString& currentRoot )
{
  this->setWindowTitle( "Remove profile" );

  m_currentRoot = currentRoot;
  ui->addNewButton->setVisible( false );
  ui->addExistingButton->setVisible( false );

  disconnect( ui->okButton, SIGNAL( clicked() ), this, SLOT( setActiveDatabase() ) );
  connect( ui->okButton, SIGNAL( clicked() ), this, SLOT( removeDatabaseConnection() ), Qt::UniqueConnection );

  this->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addExistingDatabase( const QString& currentRoot )
{
  m_currentRoot = currentRoot;

  QString file = QFileDialog::getOpenFileName( this, "Add Existing Profile", GCGlobalSpace::lastUserSelectedDirectory(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDatabaseConnection( file );

    /* Save the last visited directory. */
    QFileInfo fileInfo( file );
    QString finalDirectory = fileInfo.dir().path();
    GCGlobalSpace::setLastUserSelectedDirectory( finalDirectory );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addNewDatabase( const QString& currentRoot )
{
  m_currentRoot = currentRoot;

  QString file = QFileDialog::getSaveFileName( this, "Add New Profile", GCGlobalSpace::lastUserSelectedDirectory(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDatabaseConnection( file );

    /* Save the last visited directory. */
    QFileInfo fileInfo( file );
    QString finalDirectory = fileInfo.dir().path();
    GCGlobalSpace::setLastUserSelectedDirectory( finalDirectory );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::removeDatabaseConnection()
{
  QString dbName = ui->comboBox->currentText();

  if( !m_currentRoot.isEmpty() &&
      GCDataBaseInterface::instance()->activeSessionName() == dbName )
  {
    bool accepted = GCMessageSpace::userAccepted( "RemoveActiveSessionWarning",
                                                  "Warning!",
                                                  "Removing the active profile will cause the current "
                                                  "document to be reset and your work will be lost! ",
                                                  GCMessageSpace::OKCancel,
                                                  GCMessageSpace::Cancel,
                                                  GCMessageSpace::Warning );

    if( !accepted )
    {
      return;
    }
  }

  if( !GCDataBaseInterface::instance()->removeDatabase( dbName ) )
  {
    QString error = QString( "Failed to remove profile \"%1\": [%2]" ).arg( dbName )
      .arg( GCDataBaseInterface::instance()->lastError() );
    GCMessageSpace::showErrorMessageBox( this, error );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setActiveDatabase()
{
  setActiveDatabase( ui->comboBox->currentText() );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Profiles are used to store information about XML files, so please: \n\n"
                            "1. Create a new profile for the type of XML you are going to work with (I suggest "
                            "using a name that closely resembles the XML files it will represent), or\n"
                            "2. Add an existing profile(s) if you have any." );
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setActiveDatabase( const QString& dbName )
{
  /* If the current root element is not known to the new session, the user must
    confirm whether or not he/she wants the active document to be reset. */
  if( !m_currentRoot.isEmpty() &&
      !GCDataBaseInterface::instance()->containsKnownRootElement( dbName, m_currentRoot ) )
  {
    QMessageBox::StandardButton accept = QMessageBox::question( this,
                                                                "Unsupported document",
                                                                "The selected profile doesn't support your current document. Your "
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

  if( !GCDataBaseInterface::instance()->setActiveDatabase( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
      .arg( GCDataBaseInterface::instance()->lastError() );
    GCMessageSpace::showErrorMessageBox( this, error );
  }
  else
  {
    this->hide();
    this->accept();
    emit activeDatabaseChanged( dbName );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::addDatabaseConnection( const QString& dbName )
{
  if( !GCDataBaseInterface::instance()->addDatabase( dbName ) )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    return;
  }

  bool accepted = GCMessageSpace::userAccepted( "SwitchActiveSession",
                                                "Switch Session",
                                                "Would you like to switch to the new profile?",
                                                GCMessageSpace::YesNo,
                                                GCMessageSpace::Yes,
                                                GCMessageSpace::Question );

  if( accepted )
  {
    setActiveDatabase( dbName );
  }

  setDatabaseList();
}

/*--------------------------------------------------------------------------------------*/

void GCDBSessionManager::setDatabaseList()
{
  ui->comboBox->clear();

  QStringList dbList = GCDataBaseInterface::instance()->connectionList();

  if( dbList.empty() )
  {
    ui->okButton->setVisible( false );
    ui->comboBox->addItem( "You don't have any known profiles..." );
    ui->showHelpButton->setVisible( true );
  }
  else
  {
    ui->okButton->setVisible( true );
    ui->comboBox->addItems( dbList );
    ui->showHelpButton->setVisible( false );
  }
}

/*--------------------------------------------------------------------------------------*/