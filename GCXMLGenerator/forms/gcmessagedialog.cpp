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

#include "gcmessagedialog.h"
#include "ui_gcmessagedialog.h"

/*--------------------------------------------------------------------------------------*/

GCMessageDialog::GCMessageDialog( bool *remember,
                                  const QString &heading,
                                  const QString &text,
                                  ButtonCombo buttons,
                                  Buttons defaultButton,
                                  Icon icon ) 
    :
  ui        ( new Ui::GCMessageDialog ),
  m_remember( remember )
{
  ui->setupUi(this);
  ui->plainTextEdit->setPlainText( text );

  switch( buttons )
  {
    case YesNo:
      ui->acceptButton->setText( "Yes" );
      ui->rejectButton->setText( "No" );
      break;
    case OKCancel:
      ui->acceptButton->setText( "OK" );
      ui->rejectButton->setText( "Cancel" );
      break;
    case OKOnly:
      ui->acceptButton->setText( "OK" );
      ui->rejectButton->setVisible( false );
  }

  switch( defaultButton )
  {
    case Yes:
    case OK:
      ui->acceptButton->setDefault( true );
      break;
    case No:
    case Cancel:
      ui->rejectButton->setDefault( true );
  }

  switch( icon )
  {
    case Information:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxInformation ) );
      break;
    case Warning:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxWarning ) );
      break;
    case Critical:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxCritical ) );
      break;
    case Question:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxQuestion ) );
      break;
    case NoIcon:
    default:
      ui->iconButton->setIcon( QIcon() );
  }

  connect( ui->acceptButton, SIGNAL( clicked() ),       this, SLOT( accept() ) );
  connect( ui->rejectButton, SIGNAL( clicked() ),       this, SLOT( reject() ) );
  connect( ui->checkBox,     SIGNAL( toggled( bool ) ), this, SLOT( setRememberUserChoice( bool ) ) );

  setWindowTitle( heading );
}

/*--------------------------------------------------------------------------------------*/

GCMessageDialog::~GCMessageDialog()
{
  /* The variable that "m_remember" points to is owned externally. */
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMessageDialog::setRememberUserChoice( bool remember )
{
  *m_remember = remember;
}

/*--------------------------------------------------------------------------------------*/
