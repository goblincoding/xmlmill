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

#include "additemsform.h"
#include "ui_additemsform.h"
#include "db/dbinterface.h"
#include "utils/messagespace.h"
#include "utils/globalspace.h"
#include "utils/treewidgetitem.h"

#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

const QString CREATE_NEW = "Create New Element";

/*--------------------------------------------------------------------------------------*/

AddItemsForm::AddItemsForm( QWidget* parent )
: QDialog( parent ),
  ui     ( new Ui::AddItemsForm )
{
  ui->setupUi( this );
  ui->showHelpButton->setVisible( GlobalSpace::showHelpButtons() );

  connect( ui->addNewButton, SIGNAL( clicked() ), this, SLOT( addElementAndAttributes() ) );
  connect( ui->donePushButton, SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
  connect( ui->comboBox, SIGNAL( activated( const QString& ) ), this, SLOT( comboValueChanged( const QString& ) ) );

  populateCombo();
  ui->treeWidget->populateFromDatabase();

  if( ui->treeWidget->topLevelItemCount() != 0 )
  {
    ui->treeWidget->setCurrentItem( ui->treeWidget->topLevelItem( 0 ) );
  }

  ui->lineEdit->setFocus();
  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

AddItemsForm::~AddItemsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void AddItemsForm::populateCombo()
{
  ui->comboBox->clear();

  /* It should not be possible to add the root element as a child to any other element. */
  QStringList elements( DataBaseInterface::instance()->knownElements() );

  foreach( QString root, DataBaseInterface::instance()->knownRootElements() )
  {
   elements.removeAll( root );
  }

  ui->comboBox->addItem( CREATE_NEW );
  ui->comboBox->addItems( elements );
  comboValueChanged( CREATE_NEW );
}

/*--------------------------------------------------------------------------------------*/

void AddItemsForm::addElementAndAttributes()
{
  QString element = ui->lineEdit->text();

  if( !element.isEmpty() )
  {
    QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );

    if( DataBaseInterface::instance()->knownElements().contains( element ) )
    {
      if( !DataBaseInterface::instance()->updateElementAttributes( element, attributes ) )
      {
        MessageSpace::showErrorMessageBox( this, DataBaseInterface::instance()->lastError() );
      }
    }
    else
    {
      if( !DataBaseInterface::instance()->addElement( element, QStringList(), attributes ) )
      {
        MessageSpace::showErrorMessageBox( this, DataBaseInterface::instance()->lastError() );
      }
    }

    /* If the profile is empty, add the new element as a root element by default. */
    if( DataBaseInterface::instance()->isProfileEmpty() )
    {
      if( !DataBaseInterface::instance()->addRootElement( element ) )
      {
        MessageSpace::showErrorMessageBox( this, DataBaseInterface::instance()->lastError() );
      }

      ui->treeWidget->addItem( element );
    }
    else
    {
      /* If the profile isn't empty, the user must specify a parent element. */
      if( ui->treeWidget->currentItem() )
      {
        /* Also add it to the parent element's child list. */
        if( !DataBaseInterface::instance()->updateElementChildren( ui->treeWidget->CurrentItem()->name(),
                                                                     QStringList( element ) ) )
        {
          MessageSpace::showErrorMessageBox( this, DataBaseInterface::instance()->lastError() );
        }

        ui->treeWidget->insertItem( element, 0 );
      }
      else
      {
        MessageSpace::showErrorMessageBox( this, "Please select a parent element in the tree." );
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

void AddItemsForm::comboValueChanged( const QString& element )
{
  if( element != CREATE_NEW )
  {
    ui->lineEdit->setText( element );
    ui->lineEdit->setEnabled( false );

    QStringList attributes = DataBaseInterface::instance()->attributes( element );

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

void AddItemsForm::showHelp()
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
