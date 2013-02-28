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
#include "db/gcdatabaseinterface.h"

#include <QDomDocument>

/*--------------------------------------------------------------------------------------*/

GCDomTreeWidget::GCDomTreeWidget( QWidget *parent ) :
  QTreeWidget( parent ),
  m_domDoc   ( new QDomDocument ),
  m_isEmpty  ( true ),
  m_items    ()
{
  connect( this, SIGNAL( itemClicked( QTreeWidgetItem*,int ) ), this, SLOT( emitGcCurrentItemChanged( QTreeWidgetItem*,int ) ) );
}

/*--------------------------------------------------------------------------------------*/

GCDomTreeWidget::~GCDomTreeWidget()
{
  delete m_domDoc;
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem* GCDomTreeWidget::gcCurrentItem() const
{
  return dynamic_cast< GCTreeWidgetItem* >( currentItem() );
}

/*--------------------------------------------------------------------------------------*/

QDomDocument GCDomTreeWidget::document() const
{
  return *m_domDoc;
}

/*--------------------------------------------------------------------------------------*/

QList< GCTreeWidgetItem* > GCDomTreeWidget::includedGcTreeWidgetItems() const
{
  QList< GCTreeWidgetItem* > includedItems;

  for( int i = 0; i < m_items.size(); ++i )
  {
    GCTreeWidgetItem* localItem = m_items.at( i );

    if( !localItem->elementExcluded() )
    {
      includedItems.append( localItem );
    }
  }

  return includedItems;
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::populateFromDatabase( const QString &baseElementName )
{
  clearAndReset();

  if( baseElementName.isEmpty() )
  {
    /* It is possible that there may be multiple document types saved to this profile. */
    foreach( QString element, GCDataBaseInterface::instance()->knownRootElements() )
    {
      m_isEmpty = true;   // forces the new item to be added to the invisible root
      addItem( element );
      processNextElement( element );
    }
  }
  else
  {
    addItem( baseElementName );
    processNextElement( baseElementName );
  }

  expandAll();
  setCurrentItem( invisibleRootItem()->child( 0 ) );
  emitGcCurrentItemChanged( currentItem(), 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::processNextElement( const QString &element )
{
  QStringList children = GCDataBaseInterface::instance()->children( element );

  foreach( QString child, children )
  {
    addItem( child );

    /* Since it isn't illegal to have elements with children of the same name, we cannot
      block it in the DB, however, if we DO have elements with children of the same name,
      this recursive call enters an infinite loop, so we need to make sure that doesn't
      happen. */
    if( child != element )
    {
      processNextElement( child );
    }
  }

  setCurrentItem( currentItem()->parent() );  // required to enforce sibling relationships
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::addItem( const QString &element )
{
  if( currentItem() )
  {
    insertItem( element, currentItem()->childCount() );
  }
  else
  {
    insertItem( element, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::insertItem( const QString &elementName, int index )
{
  QDomElement element = m_domDoc->createElement( elementName );

  /* Create all the possible attributes for the element here, they can be changed
    later on. */
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName );

  for( int i = 0; i < attributeNames.count(); ++i )
  {
    element.setAttribute( attributeNames.at( i ), "" );
  }

  GCTreeWidgetItem *item = new GCTreeWidgetItem( elementName, element );
  m_items.append( item );

  if( m_isEmpty )
  {
    invisibleRootItem()->addChild( item );  // takes ownership
    m_domDoc->appendChild( element );
    m_isEmpty = false;
  }
  else
  {
    currentItem()->insertChild( index, item );
    gcCurrentItem()->element().appendChild( element );
  }

  setCurrentItem( item );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::setAllCheckStates( Qt::CheckState state )
{
  QTreeWidgetItemIterator iterator( this );

  while( *iterator )
  {
    ( *iterator )->setCheckState( 0, state );
    ++iterator;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::emitGcCurrentItemChanged( QTreeWidgetItem *item, int column )
{
  setCurrentItem( item, column );
  emit gcCurrentItemChanged( dynamic_cast< GCTreeWidgetItem* >( item ), column );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::clearAndReset()
{
  this->clear();
  m_domDoc->clear();
  m_items.clear();
  m_isEmpty = true;
}

/*--------------------------------------------------------------------------------------*/
