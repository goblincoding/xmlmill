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

#include "gcremoveitemsform.h"
#include "ui_gcremoveitemsform.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gcmessagespace.h"
#include "utils/gcglobalspace.h"
#include "utils/gctreewidgetitem.h"

#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCRemoveItemsForm::GCRemoveItemsForm( QWidget *parent ) :
  QDialog               ( parent ),
  ui                    ( new Ui::GCRemoveItemsForm ),
  m_currentElement      ( "" ),
  m_currentElementParent( "" ),
  m_currentAttribute    ( "" ),
  m_deletedElements     ()
{
  ui->setupUi( this );
  ui->showAttributeHelpButton->setVisible( GCGlobalSpace::showHelpButtons() );
  ui->showElementHelpButton->setVisible( GCGlobalSpace::showHelpButtons() );

  connect( ui->showElementHelpButton,   SIGNAL( clicked() ), this, SLOT( showElementHelp() ) );
  connect( ui->showAttributeHelpButton, SIGNAL( clicked() ), this, SLOT( showAttributeHelp() ) );
  connect( ui->updateValuesButton,      SIGNAL( clicked() ), this, SLOT( updateAttributeValues() ) );
  connect( ui->deleteAttributeButton,   SIGNAL( clicked() ), this, SLOT( deleteAttribute() ) );
  connect( ui->deleteElementButton,     SIGNAL( clicked() ), this, SLOT( deleteElement() ) );
  connect( ui->removeFromParentButton,  SIGNAL( clicked() ), this, SLOT( removeChildElement() ) );

  connect( ui->treeWidget, SIGNAL( gcCurrentItemSelected( GCTreeWidgetItem*,int,bool ) ), this, SLOT( elementSelected( GCTreeWidgetItem*,int ) ) );
  connect( ui->comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeActivated( QString ) ) );

  ui->treeWidget->populateFromDatabase();

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCRemoveItemsForm::~GCRemoveItemsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::elementSelected( GCTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );

  if( item )
  {
    if( item->gcParent() )
    {
      m_currentElementParent = item->gcParent()->name();
    }

    m_currentElement = item->name();

    /* Since it isn't illegal to have elements with children of the same name, we cannot
      block it in the DB, however, if we DO have elements with children of the same name,
      we don't want the user to delete the element since bad things will happen. */
    if( m_currentElement == m_currentElementParent )
    {
      ui->deleteElementButton->setEnabled( false );
    }
    else
    {
      ui->deleteElementButton->setEnabled( true );
    }

    QStringList attributes = GCDataBaseInterface::instance()->attributes( m_currentElement );

    ui->comboBox->clear();
    ui->comboBox->addItems( attributes );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::attributeActivated( const QString &attribute )
{
  m_currentAttribute = attribute;
  QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( m_currentElement, m_currentAttribute );

  ui->plainTextEdit->clear();

  foreach( QString value, attributeValues )
  {
    ui->plainTextEdit->insertPlainText( QString( "%1\n" ).arg( value ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::deleteElement( const QString &element )
{
  /* If the element name is empty, then this function was called directly by the user
    clicking on "delete" (as opposed to this function being called further down below
    during the recursive process of getting rid of the element's children) in that case,
    the first element to be removed is the current one (set in "elementSelected"). */
  QString currentElement = ( element.isEmpty() ) ? m_currentElement : element;

  QStringList children = GCDataBaseInterface::instance()->children( currentElement );
  m_deletedElements.clear();

  /* Attributes and values must be removed before we can remove elements and we must also
    ensure that children are removed before their parents.  To achieve this, we need to ensure
    that we clean the element tree from "the bottom up". */
  if( !children.isEmpty() )
  {
    foreach( QString child, children )
    {
      if( GCDataBaseInterface::instance()->isUniqueChildElement( currentElement, child ) )
      {
        deleteElement( child );
      }
    }
  }

  /* Remove all the attributes (and their known values) associated with this element. */
  QStringList attributes = GCDataBaseInterface::instance()->attributes( currentElement );

  foreach( QString attribute, attributes )
  {
    if( !GCDataBaseInterface::instance()->removeAttribute( currentElement, attribute ) )
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    }
  }

  /* Now we can remove the element itself. */
  if( !GCDataBaseInterface::instance()->removeElement( currentElement ) )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
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
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    }
  }

  /* Remove the element from all its parents' first level child lists. */
  updateChildLists();

  ui->comboBox->clear();
  ui->plainTextEdit->clear();
  ui->treeWidget->populateFromDatabase();
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::removeChildElement()
{
  if( !m_currentElementParent.isEmpty() )
  {
    if( !GCDataBaseInterface::instance()->removeChildElement( m_currentElementParent, m_currentElement ) )
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    }
    else
    {
      if( GCDataBaseInterface::instance()->isUniqueChildElement( m_currentElementParent, m_currentElement ) )
      {
        bool accepted = GCMessageSpace::userAccepted( "RemoveUnlistedElement",
                                                      "Element not used",
                                                      QString( "\"%1\" is not assigned to any other element (i.e. "
                                                               "it isn't used anywhere else in the profile).\n"
                                                               "Would you like to remove the element completely?" ).arg( m_currentElement ),
                                                      GCMessageSpace::YesNo,
                                                      GCMessageSpace::No,
                                                      GCMessageSpace::Question );

        if( accepted )
        {
          deleteElement();
        }
      }

      ui->comboBox->clear();
      ui->plainTextEdit->clear();
      ui->treeWidget->populateFromDatabase();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::updateAttributeValues()
{
  QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );
  attributes.removeAll( "" );

  if( attributes.isEmpty() )
  {
    bool accepted = GCMessageSpace::userAccepted( "UpdateEmptyAttributeValues",
                                                  "Update with empty attribute values?",
                                                  "All known values were removed. "
                                                  "Would you like to remove the attribute completely?",
                                                  GCMessageSpace::YesNo,
                                                  GCMessageSpace::No,
                                                  GCMessageSpace::Question );

    if( accepted )
    {
      deleteAttribute();
    }
  }
  else
  {
    /* All existing values will be replaced with whatever remained in the text edit by the time the
      user was done. */
    if( !GCDataBaseInterface::instance()->updateAttributeValues( m_currentElement, m_currentAttribute, attributes, true ) )
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    }
    else
    {
      QMessageBox::information( this, "Success", "Done!" );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::deleteAttribute()
{
  if( !GCDataBaseInterface::instance()->removeAttribute( m_currentElement, m_currentAttribute ) )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
  }
  else
  {
    /* Purely for cosmetic effect - updates the tree item to reflect the correct node text when
      in "verbose" mode. */
    ui->treeWidget->gcCurrentItem()->excludeAttribute( m_currentAttribute );

    ui->comboBox->removeItem( ui->comboBox->findText( m_currentAttribute ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::updateChildLists()
{
  QStringList knownElements = GCDataBaseInterface::instance()->knownElements();

  foreach( QString element, knownElements )
  {
    QStringList children = GCDataBaseInterface::instance()->children( element );

    if( !children.isEmpty() )
    {
      foreach( QString deletedElement, m_deletedElements )
      {
        if( children.contains( deletedElement ) )
        {
          if( !GCDataBaseInterface::instance()->removeChildElement( element, deletedElement ) )
          {
            GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
          }
        }
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::showElementHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "\"Remove Child\" will remove the currently highlighted element "
                            "from its parent element's child list, i.e. it will only "
                            "affect the relationship between the two elements, the element "
                            "itself is not deleted in the process and will remain in the profile. \n\n"
                            "\"Delete Element\" will delete the element, the element's children, "
                            "the children's children (etc, etc), its associated attributes, the "
                            "associated attributes of its children (and their children, etc, etc), all "
                            "the known values for all the attributes thus deleted and finally also "
                            "remove the element (and its children and the children's children, etc etc) "
                            "from every single child list that contains it.\n\n"
                            "None of this can be undone. " );
}

/*--------------------------------------------------------------------------------------*/

void GCRemoveItemsForm::showAttributeHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "\"Delete Attribute\" will also delete all its known values.\n\n"
                            "\"Update Attribute Values\" - Only those values remaining in the text edit when "
                            "\"Update Attribute Values\" is clicked will be saved against the attribute shown "
                            "in the drop down (this effectively means that you could also add new values "
                            "to the attribute if you wish).  Just make sure that all the values you want to "
                            "associate with the attribute when you're done appear on separate lines." );
}

/*--------------------------------------------------------------------------------------*/
