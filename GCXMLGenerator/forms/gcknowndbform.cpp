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

#include "gcknowndbform.h"
#include "ui_gcknowndbform.h"
#include <QFileDialog>

/*--------------------------------------------------------------------------------------*/

GCKnownDBForm::GCKnownDBForm( const QStringList &dbList, Buttons buttons, QWidget *parent ) :
  QDialog  ( parent),
  ui       ( new Ui::GCKnownDBForm ),
  m_buttons( buttons ),
  m_remove ( false )
{
  ui->setupUi( this );

  if( dbList.empty() )
  {
    ui->okButton->setVisible( false );
    ui->comboBox->addItem( "Let there be databases! (i.e. you'll have to add some)" );
  }
  else
  {
    ui->comboBox->addItems( dbList );
  }

  switch( m_buttons )
  {
    case SelectOnly:
      ui->addNewButton->setVisible( false );
      ui->addExistingButton->setVisible( false );
      break;
    case SelectAndExisting:
      ui->addNewButton->setVisible( false );
      break;
    case ToRemove:
      ui->addNewButton->setVisible( false );
      ui->addExistingButton->setVisible( false );
      m_remove = true;
      break;
    case ShowAll:
      break;
  }

  connect( ui->addNewButton,      SIGNAL( clicked() ), this, SIGNAL( newConnection() ) );
  connect( ui->addNewButton,      SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SIGNAL( existingConnection() ) );
  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->cancelButton,      SIGNAL( clicked() ), this, SIGNAL( userCancelled() ) );
  connect( ui->cancelButton,      SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->okButton,          SIGNAL( clicked() ), this, SLOT  ( select() ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCKnownDBForm::~GCKnownDBForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCKnownDBForm::select()
{
  if( m_remove )
  {
    emit dbRemoved( ui->comboBox->currentText() );
  }
  else
  {
    emit dbSelected( ui->comboBox->currentText() );
  }

  this->close();
}

/*--------------------------------------------------------------------------------------*/
