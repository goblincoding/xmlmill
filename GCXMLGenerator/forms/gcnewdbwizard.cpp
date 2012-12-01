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

#include "gcnewdbwizard.h"
#include "ui_gcnewdbwizard.h"

#include <QFileDialog>

/*--------------------------------------------------------------------------------------*/

GCNewDBWizard::GCNewDBWizard( QWidget *parent ) :
  QWizard      ( parent ),
  ui           ( new Ui::GCNewDBWizard ),
  m_dbFileName ( "" ),
  m_xmlFileName( "" )
{
  ui->setupUi(this);
  connect( ui->importFileButton, SIGNAL( clicked() ), this, SLOT( openXMLFileDialog() ) );
  connect( ui->newDBButton,      SIGNAL( clicked() ), this, SLOT( openDBFileDialog() ) );
}

/*--------------------------------------------------------------------------------------*/

GCNewDBWizard::~GCNewDBWizard()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

QString GCNewDBWizard::dbFileName()
{
  return m_dbFileName;
}

/*--------------------------------------------------------------------------------------*/

QString GCNewDBWizard::xmlFileName()
{
  return m_xmlFileName;
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::accept()
{
  m_xmlFileName = ui->importFileLineEdit->text();
  m_dbFileName  = ui->newDBLineEdit->text();

  QDialog::accept();
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::openDBFileDialog()
{
  QString fileName = QFileDialog::getSaveFileName( this, "Add New Profile", QDir::homePath(), "Profile Files (*.db)" );

  /* If the user clicked "OK", continue (a cancellation will result in an empty file name). */
  if( !fileName.isEmpty() )
  {
    ui->newDBLineEdit->setText( fileName );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::openXMLFileDialog()
{
  QString fileName = QFileDialog::getOpenFileName( this, "Open XML File", QDir::homePath(), "XML Files (*.*)" );

  /* If the user clicked "OK", continue (a cancellation will result in an empty file name). */
  if( !fileName.isEmpty() )
  {
    ui->importFileLineEdit->setText( fileName );
  }
}

/*--------------------------------------------------------------------------------------*/
