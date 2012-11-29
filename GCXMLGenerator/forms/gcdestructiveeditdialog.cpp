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
#include "utils/gcmessagespace.h"
#include <QMessageBox>
#include <QTreeWidgetItem>

/*--------------------------------------------------------------------------------------*/

GCDestructiveEditDialog::GCDestructiveEditDialog( QWidget *parent ) :
  QDialog               ( parent ),
  ui                    ( new Ui::GCDestructiveEditDialog ),
  m_currentElement      ( "" ),
  m_currentElementParent( "" ),
  m_currentAttribute    ( "" ),
  m_deletedElements     ()
{
  ui->setupUi( this );

  connect( ui->elementHelpButton,     SIGNAL( clicked() ), this, SLOT( showElementHelp() ) );
  connect( ui->attributeHelpButton,   SIGNAL( clicked() ), this, SLOT( showAttributeHelp() ) );
  connect( ui->updateValuesButton,    SIGNAL( clicked() ), this, SLOT( updateAttributeValues() ) );
  connect( ui->deleteAttributeButton, SIGNAL( clicked() ), this, SLOT( deleteAttribute() ) );
  connect( ui->deleteElementButton,   SIGNAL( clicked() ), this, SLOT( deleteElement() ) );

  connect( ui->comboBox,   SIGNAL( currentIndexChanged( QString ) ),      this, SLOT( attributeActivated( QString ) ) );
  connect( ui->treeWidget, SIGNAL( itemClicked( QTreeWidgetItem*,int ) ), this, SLOT( treeWidgetItemActivated( QTreeWidgetItem*,int ) ) );

  populateTreeWidget();

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCDestructiveEditDialog::~GCDestructiveEditDialog()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::treeWidgetItemActivated( QTreeWidgetItem *item, int column )
{
  if( item->parent() )
  {
    m_currentElementParent = item->parent()->text( column );
  }

  m_currentElement = item->text( column );

  bool success( false );
  QStringList attributes = GCDataBaseInterface::instance()->attributes( m_currentElement, success );

  if( success )
  {
    ui->comboBox->clear();
    ui->comboBox->addItems( attributes );
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::attributeActivated( const QString &attribute )
{
  bool success( false );
  m_currentAttribute = attribute;
  QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( m_currentElement, m_currentAttribute, success );

  if( success )
  {
    ui->plainTextEdit->clear();

    foreach( QString value, attributeValues )
    {
      ui->plainTextEdit->insertPlainText( QString( "%1\n" ).arg( value ) );
    }
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::deleteElement( const QString &element )
{
  /* If the element name is empty, then this is the first call to this function (i.e
    the slot has been called after the user clicked the relevant push button), in that
    case, the first element to be removed is the current one. */
  QString currentElement = ( element.isEmpty() ) ? m_currentElement : element;

  bool success( false );
  QStringList children = GCDataBaseInterface::instance()->children( currentElement, success );
  m_deletedElements.clear();

  if( success )
  {
    /* Attributes and values must be removed before we can remove elements and we must also
      ensure that children are removed before their parents.  To achieve this, we need to ensure
      that we clean the element tree from the "bottom up". */
    if( !children.isEmpty() )
    {
      foreach( QString child, children )
      {
        deleteElement( child );
      }
    }

    /* Remove all the attributes (and their known values) associated with this element. */
    QStringList attributes = GCDataBaseInterface::instance()->attributes( currentElement, success );

    if( success )
    {
      foreach( QString attribute, attributes )
      {
        if( !GCDataBaseInterface::instance()->removeAttribute( currentElement, attribute ) )
        {
          showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
        }
      }

      /* Now we can remove the element itself. */
      if( !GCDataBaseInterface::instance()->removeElement( currentElement ) )
      {
        showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
      }
      else
      {
        m_deletedElements.append( currentElement );
      }

      /* Check if the user removed a root element. */
      if( GCDataBaseInterface::instance()->knownRootElements().contains( currentElement ) )
      {
        if( !GCDataBaseInterface::instance()->removeRootElement( currentElement ) )
        {
          showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
        }
      }
    }
    else
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

    /* Remove the element from all its parents' first level child lists. */
    updateChildLists();

    ui->comboBox->clear();
    ui->plainTextEdit->clear();
    populateTreeWidget();
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::removeChildElement()
{
  if( !m_currentElementParent.isEmpty() )
  {
    if( !GCDataBaseInterface::instance()->removeChildElement( m_currentElementParent, m_currentElement ) )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }
    else
    {
      ui->comboBox->clear();
      ui->plainTextEdit->clear();
      populateTreeWidget();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::updateAttributeValues()
{
  QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );

  if( attributes.isEmpty() )
  {
    bool accepted = GCMessageSpace::userAccepted( "UpdateEmptyAttributeValues",
                                                  "Update with empty attribute values?",
                                                  "Since you have removed all the known values from this attribute, "
                                                  "would you perhaps like to remove it in its entirety?",
                                                  GCMessageDialog::YesNo,
                                                  GCMessageDialog::No,
                                                  GCMessageDialog::Question );

    if( accepted )
    {
      deleteAttribute();
    }
  }
  else
  {
    /* All existing values will be replaced with whatever remained in the text edit by the time the
      user was done. */
    if( !GCDataBaseInterface::instance()->replaceAttributeValues( m_currentElement, m_currentAttribute, attributes ) )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::deleteAttribute()
{
  if( !GCDataBaseInterface::instance()->removeAttribute( m_currentElement, m_currentAttribute ) )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
  else
  {
    ui->comboBox->removeItem( ui->comboBox->findText( m_currentAttribute ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::showElementHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Removing a child will remove the currently highlighted element\n"
                            "from the list of children associated with its parent, i.e. it will\n"
                            "only affect the relationship between the two elements, the element\n"
                            "itself is not deleted in the process.\n\n"
                            "Deleting an element will also delete its children, associated\n"
                            "attributes, the associated attributes of its children all\n"
                            "the known values for all the attributes thus deleted and remove the\n"
                            "element as child from absolutely every element that had it listed as\n"
                            "a first level child element.\n\n"
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

void GCDestructiveEditDialog::updateChildLists()
{
  bool success( false );
  QStringList knownElements = GCDataBaseInterface::instance()->knownElements();

  foreach( QString element, knownElements )
  {
    QStringList children = GCDataBaseInterface::instance()->children( element, success );

    if( success && !children.isEmpty() )
    {
      foreach( QString deletedElement, m_deletedElements )
      {
        if( children.contains( deletedElement ) )
        {
          if( !GCDataBaseInterface::instance()->removeChildElement( element, deletedElement ) )
          {
            showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
          }
        }
      }
    }
    else
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::populateTreeWidget()
{
  ui->treeWidget->clear();

  foreach( QString element, GCDataBaseInterface::instance()->knownRootElements() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText( 0, element );

    ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
    processNextElement( element, item );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDestructiveEditDialog::processNextElement( const QString &element, QTreeWidgetItem *parent )
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
      processNextElement( child, item );
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
