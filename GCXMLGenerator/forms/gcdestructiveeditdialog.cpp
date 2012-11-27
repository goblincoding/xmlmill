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

#include "gcdestructiveeditdialog.h"
#include "ui_gcdestructiveeditdialog.h"
#include "db/gcdatabaseinterface.h"
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCDestructiveEditDialog::GCDestructiveEditDialog( QWidget *parent ) :
  QDialog( parent ),
  ui     ( new Ui::GCDestructiveEditDialog )
{
  ui->setupUi( this );

  connect( ui->elementHelpButton,   SIGNAL( clicked() ), this, SLOT( showElementHelp() ) );
  connect( ui->attributeHelpButton, SIGNAL( clicked() ), this, SLOT( showAttributeHelp() ) );

  foreach( QString element, GCDataBaseInterface::instance()->knownRootElements() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText( 0, element );

    ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
    populateElementHierarchy( element, item );
  }

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCDestructiveEditDialog::~GCDestructiveEditDialog()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::showElementHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Deleting an element will also delete its children, associated\n"
                            "attributes, the associated attributes of its children and all\n"
                            "the known values for all the attributes thus deleted.\n\n"
                            "I suggest you tread lightly :)\n" );
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::showAttributeHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "1. Deleting an attribute will also delete all its known values.\n"
                            "2. Only those values remaining in the text edit below will be\n"
                            "   saved against the attribute shown in the drop down (this\n"
                            "   effectively means that you could also add new values to the\n"
                            "   attribute).  Just make sure that all the values you want to\n"
                            "   associate with the attribute appear on their own lines." );
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::populateElementHierarchy( const QString &element, QTreeWidgetItem *parent )
{
  bool success( false );
  QStringList children = GCDataBaseInterface::instance()->children( element, success );

  if( success )
  {
    foreach( QString element, children )
    {
      QTreeWidgetItem *item = new QTreeWidgetItem;
      item->setText( 0, element );

      parent->addChild( item );  // takes ownership
      populateElementHierarchy( element, item );
    }
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
