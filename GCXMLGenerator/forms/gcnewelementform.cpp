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

#include "gcnewelementform.h"
#include "ui_gcnewelementform.h"
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCNewElementForm::GCNewElementForm( QWidget *parent ) :
  QWidget( parent ),
  ui     ( new Ui::GCNewElementForm )
{
  ui->setupUi( this );

  connect( ui->addPushButton,  SIGNAL( clicked() ), this, SLOT( addElementAndAttributes() ) );
  connect( ui->donePushButton, SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->helpToolButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );

  setAttribute( Qt::WA_DeleteOnClose );
  setWindowFlags( Qt::Dialog );
}

/*--------------------------------------------------------------------------------------*/

GCNewElementForm::~GCNewElementForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCNewElementForm::addElementAndAttributes()
{
  QString element = ui->lineEdit->text();

  if( !element.isEmpty() )
  {
    QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );
    emit newElementDetails( element, attributes );
  }

  ui->lineEdit->clear();
  ui->plainTextEdit->clear();
}

/*--------------------------------------------------------------------------------------*/

void GCNewElementForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Enter the name of the new element in the \"Element Name\"\n"
                            "field (no surprises there) and the attributes associated\n"
                            "with the element in the \"Attribute Names\" text area (again\n"
                            "pretty obvious).\n\n"
                            "What you might not know, however, is that although you can only add\n"
                            "one element at a time, you can add all the attributes for the\n"
                            "element in one go: simply stick each of them on a separate line\n"
                            "in the text edit area and hit \"Add\" when you're done.\n\n"
                            "(oh, and if the element doesn't have associated attributes, just\n"
                            "leave the text edit area empty)" );
}

/*--------------------------------------------------------------------------------------*/
