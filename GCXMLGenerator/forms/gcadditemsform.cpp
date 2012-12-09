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

#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCAddItemsForm::GCAddItemsForm( QWidget *parent ) :
  QDialog( parent ),
  ui     ( new Ui::GCAddItemsForm )
{
  ui->setupUi( this );

  connect( ui->addPushButton,  SIGNAL( clicked() ), this, SLOT( addElementAndAttributes() ) );
  connect( ui->donePushButton, SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->helpToolButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );

  populateTreeWidget();
  ui->treeWidget->expandAll();

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCAddItemsForm::~GCAddItemsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::populateTreeWidget()
{
  ui->treeWidget->clear();

  /* It is possible that there may be multiple document types saved to this profile. */
  foreach( QString element, GCDataBaseInterface::instance()->knownRootElements() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText( 0, element );

    ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
    processNextElement( element, item );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::processNextElement( const QString &element, QTreeWidgetItem *parent )
{
  bool success( false );
  QStringList children = GCDataBaseInterface::instance()->children( element, success );

  if( success )
  {
    foreach( QString child, children )
    {
      QTreeWidgetItem *item = new QTreeWidgetItem;
      item->setText( 0, child );

      parent->addChild( item );  // takes ownership

      /* Since it isn't illegal to have elements with children of the same name, we cannot
        block it in the DB, however, if we DO have elements with children of the same name,
        this recursive call enters an infinite loop, so we need to make sure that doesn't
        happen. */
      if( child != element )
      {
        processNextElement( child, item );
      }
    }
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::addElementAndAttributes()
{
  QString element = ui->lineEdit->text();

  if( !element.isEmpty() )
  {
    /* If the profile is empty, add the new element as a root element by default. */
    if( GCDataBaseInterface::instance()->profileEmpty() )
    {
      if( !GCDataBaseInterface::instance()->addRootElement( element ) )
      {
        showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
      }
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
          showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
        }
      }
      else
      {
        showErrorMessageBox( "Please select a parent element in the tree." );
        return;
      }
    }

    /* Add the new element and associated attributes to the DB. */
    QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );

    if( !GCDataBaseInterface::instance()->addElement( element, QStringList(), attributes ) )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }
  }

  ui->lineEdit->clear();
  ui->plainTextEdit->clear();
  populateTreeWidget();
  ui->treeWidget->expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "If the active profile is empty, the new element will be added as a\n"
                            "root (main document) element for the current profile, otherwise it\n"
                            "will be added as a child of the element highlighted in the \"Element\n"
                            "Hierarchy\" tree.\n\n"
                            "Although you can only add one element at a time, you can add all the\n"
                            "element's attributes in one go: simply stick each of them on a separate\n"
                            "line in the text edit area and hit \"Add\" when you're done.\n\n"
                            "(oh, and if the element doesn't have associated attributes, just\n"
                            "leave the text edit area empty)" );
}

/*--------------------------------------------------------------------------------------*/

void GCAddItemsForm::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
