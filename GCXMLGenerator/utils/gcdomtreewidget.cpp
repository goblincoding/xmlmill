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

#include "gcdomtreewidget.h"
#include "gctreewidgetitem.h"
#include "gcmessagespace.h"
#include "db/gcdatabaseinterface.h"

#include <QDomDocument>

/*--------------------------------------------------------------------------------------*/

GCDOMTreeWidget::GCDOMTreeWidget( QWidget *parent ) :
  QTreeWidget( parent ),
  m_items    (),
  m_domDoc   ( new QDomDocument ),
  m_isEmpty  ( true )
{
  connect( this, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( elementChanged( QTreeWidgetItem*, int ) ) );
}

/*--------------------------------------------------------------------------------------*/

GCDOMTreeWidget::~GCDOMTreeWidget()
{
  delete m_domDoc;
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::addComment( const QString &commentText )
{
  QDomComment comment = m_domDoc->createComment( commentText );
  GCTreeWidgetItem* gcItem = dynamic_cast< GCTreeWidgetItem* >( currentItem() );

  if( gcItem )
  {
    gcItem->element().parentNode().insertBefore( comment, gcItem->element() );
    m_isEmpty = false;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::addElement( const QString &elementName, bool asSibling )
{
  /* If we already have element nodes, add the new item to whichever
    parent item is currently highlighted in the tree widget. */
  GCTreeWidgetItem *parentItem = NULL;

  if( !m_isEmpty )
  {
    parentItem = dynamic_cast< GCTreeWidgetItem* >( currentItem() );

    if( asSibling && parentItem )
    {
      parentItem = dynamic_cast< GCTreeWidgetItem* >( parentItem->parent() );
    }
  }

  /* There is probably no chance of this ever happening, but defensive programming FTW! */
  if( !elementName.isEmpty() )
  {
    /* Update the current DOM document by creating and adding the new element. */
    QDomElement newElement = m_domDoc->createElement( elementName );

    /* Update the tree widget. */
    GCTreeWidgetItem *newItem = new GCTreeWidgetItem( newElement, 0 );
    newItem->setText( 0, elementName );
    newItem->setFlags( newItem->flags() | Qt::ItemIsEditable );

    /* If we already have mapped element nodes, add the new item to whichever
      parent item is currently highlighted in the tree widget. */
    if( parentItem )
    {
      parentItem->addChild( newItem );

      /* Expand the item's parent for convenience. */
      expandItem( parentItem );

      QDomElement parent = parentItem->element().parentNode().toElement(); // shallow copy
      parent.appendChild( newElement );
    }
    else
    {
      /* Since we don't have any existing nodes if we get to this point, we need to initialise
        the tree widget with its first item. */
      invisibleRootItem()->addChild( newItem );  // takes ownership
      m_domDoc->appendChild( newElement );
      m_isEmpty = false;
    }

    /* Add all the known attributes associated with this element name to the new element. */
    QStringList attributes = GCDataBaseInterface::instance()->attributes( elementName );

    for( int i = 0; i < attributes.size(); ++i )
    {
      newElement.setAttribute( attributes.at( i ), QString( "" ) );
    }

    emit itemValueChanged( newItem );
    setCurrentItem( newItem, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::clearAndReset()
{
  m_domDoc->clear();
  this->clear();
  m_isEmpty = true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDOMTreeWidget::setContent( const QString &text, QString *errorMsg, int *errorLine, int *errorColumn )
{
  return m_domDoc->setContent( text, errorMsg, errorLine, errorColumn );
}

/*--------------------------------------------------------------------------------------*/

bool GCDOMTreeWidget::isDocumentCompatible() const
{
  return GCDataBaseInterface::instance()->isDocumentCompatible( m_domDoc );
}

/*--------------------------------------------------------------------------------------*/

bool GCDOMTreeWidget::batchProcessDOMDocument() const
{
  return GCDataBaseInterface::instance()->batchProcessDOMDocument( m_domDoc );
}

/*--------------------------------------------------------------------------------------*/

bool GCDOMTreeWidget::isEmpty() const
{
  return m_isEmpty;
}

/*--------------------------------------------------------------------------------------*/

QDomElement GCDOMTreeWidget::root() const
{
  return m_domDoc->documentElement();
}

/*--------------------------------------------------------------------------------------*/

QString GCDOMTreeWidget::toString() const
{
  return m_domDoc->toString( 2 );
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem *GCDOMTreeWidget::item( QDomElement element )
{
  foreach( GCTreeWidgetItem* item, m_items )
  {
    if( item->element() == element )
    {
      return item;
    }
  }

  return 0;
}

/*--------------------------------------------------------------------------------------*/

const QList< GCTreeWidgetItem* > &GCDOMTreeWidget::items() const
{
  return m_items;
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::elementChanged( QTreeWidgetItem *item, int column )
{
  GCTreeWidgetItem *activeItem = dynamic_cast< GCTreeWidgetItem* >( item );

  if( activeItem )
  {
    QString newName  = activeItem->text( column );
    QString previousName = activeItem->element().tagName();

    if( newName.isEmpty() )
    {
      /* Reset if the user failed to specify a valid name. */
      activeItem->setText( column, previousName );
    }
    else
    {
      if( newName != previousName )
      {
        QList< QTreeWidgetItem* > identicalItems = this->findItems( previousName, Qt::MatchExactly | Qt::MatchRecursive );

        /* Update the element names in our active DOM doc and the tree widget. */
        for( int i = 0; i < identicalItems.count(); ++i )
        {
          GCTreeWidgetItem *gcItem = dynamic_cast< GCTreeWidgetItem* >( item );

          if( gcItem )
          {
            gcItem->element().setTagName( newName );
            gcItem->setText( column, newName );
          }
        }

        /* The name change may introduce a new element too so we can safely call "addElement" below as
        it doesn't do anything if the element already exists in the database, yet it will obviously
        add the element if it doesn't.  In the latter case, the children  and attributes associated with
        the old name will be assigned to the new element in the process. */
        QStringList attributes = GCDataBaseInterface::instance()->attributes( previousName );

        if( attributes.isEmpty() )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        QStringList children = GCDataBaseInterface::instance()->children( previousName );

        if( children.isEmpty() )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        if( !GCDataBaseInterface::instance()->addElement( newName, children, attributes ) )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        /* If we are, in fact, dealing with a new element, we also want the "new" element's associated attributes
        to be updated with the known values of these attributes. */
        foreach( QString attribute, attributes )
        {
          QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( previousName, attribute );

          if( !GCDataBaseInterface::instance()->updateAttributeValues( newName, attribute, attributeValues ) )
          {
            GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
          }
        }

        emit itemValueChanged( item );
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/
