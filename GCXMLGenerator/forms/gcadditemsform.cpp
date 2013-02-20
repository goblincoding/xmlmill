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

#include "gcadditemsform.h"
#include "ui_gcadditemsform.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"
#include "utils/gcglobalspace.h"

#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

const QString CREATE_NEW = "Create New Element";

/*--------------------------------------------------------------------------------------*/

GCAddItemsForm::GCAddItemsForm( QWidget *parent ) :
  QDialog( parent ),
  ui     ( new Ui::GCAddItemsForm )
{
  ui->setupUi( this );
  ui->showHelpButton->setVisible( GCGlobalSpace::showHelpButtons() );

  connect( ui->addNewButton,   SIGNAL( clicked() ), this, SLOT( addElementAndAttributes() ) );
  connect( ui->donePushButton, SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
  connect( ui->comboBox,       SIGNAL( activated( QString ) ), this, SLOT( comboValueChanged( QString ) ) );

  populateCombo();

  ui->treeWidget->constructElementHierarchy();
  ui->treeWidget->expandAll();

  if( ui->treeWidget->topLevelItemCount() != 0 )
  {
    ui->treeWidget->setCurrentItem( ui->treeWidget->topLevelItem( 0 ) );
  }

  ui->lineEdit->setFocus();
  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCAddItemsForm::~GCAddItemsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::populateCombo()
{
  ui->comboBox->clear();

  /* It should not be possible to add the root element as a child to any other element. */
  QStringList elements( GCDataBaseInterface::instance()->knownElements() );

  foreach( QString root, GCDataBaseInterface::instance()->knownRootElements() )
  {
    elements.removeAll( root );
  }

  ui->comboBox->addItem( CREATE_NEW );
  ui->comboBox->addItems( elements );
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::addElementAndAttributes()
{
  QString element = ui->lineEdit->text();

  if( !element.isEmpty() )
  {
    QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );

    if( GCDataBaseInterface::instance()->knownElements().contains( element ) )
    {
      if( !GCDataBaseInterface::instance()->updateElementAttributes( element, attributes ) )
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
      }
    }
    else
    {
      if( !GCDataBaseInterface::instance()->addElement( element, QStringList(), attributes ) )
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
      }
    }

    /* If the profile is empty, add the new element as a root element by default. */
    if( GCDataBaseInterface::instance()->profileEmpty() )
    {
      if( !GCDataBaseInterface::instance()->addRootElement( element ) )
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
      }

      ui->treeWidget->addItem( element );
    }
    else
    {
      /* If the profile isn't empty, the user must specify a parent element. */
      if( ui->treeWidget->currentItem() )
      {
        /* Also add it to the parent element's child list. */
        if( !GCDataBaseInterface::instance()->updateElementChildren( ui->treeWidget->currentItem()->text( 0 ),
                                                                     QStringList( element ) ) )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        ui->treeWidget->insertItem( element, 0 );
      }
      else
      {
        GCMessageSpace::showErrorMessageBox( this, "Please select a parent element in the tree." );
        return;
      }
    }
  }

  populateCombo();
  ui->lineEdit->clear();
  ui->plainTextEdit->clear();
  ui->treeWidget->expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::comboValueChanged( QString element )
{
  if( element != CREATE_NEW )
  {
    ui->lineEdit->setText( element );
    ui->lineEdit->setEnabled( false );

    QStringList attributes = GCDataBaseInterface::instance()->attributes( element );

    ui->plainTextEdit->clear();

    foreach( QString value, attributes )
    {
      ui->plainTextEdit->insertPlainText( QString( "%1\n" ).arg( value ) );
    }
  }
  else
  {
    ui->lineEdit->setEnabled( true );
    ui->lineEdit->clear();
    ui->plainTextEdit->clear();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "The new (or existing) element will be added as a a child of the element highlighted "
                            "in the \"Element Hierarchy\" tree (if the active profile is empty, the new element "
                            "will become the root/main document element for the current profile).\n\n"
                            "Although you can only add one element at a time, you can add all the "
                            "element's attributes in one go: simply stick each of them on a separate "
                            "line in the text edit area and hit \"Add\" when you're done.\n\n"
                            "(If the element doesn't have associated attributes, just "
                            "leave the text edit area empty)" );
}

/*--------------------------------------------------------------------------------------*/
