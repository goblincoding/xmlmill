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

GCDOMTreeWidget::GCDOMTreeWidget( QWidget *parent ) :
  QTreeWidget( parent ),
  m_domDoc   ( new QDomDocument ),
  m_isEmpty  ( true )
{
  connect( this, SIGNAL( itemClicked( QTreeWidgetItem*,int ) ), this, SLOT( emitGCItemClicked( QTreeWidgetItem*,int ) ) );
}

/*--------------------------------------------------------------------------------------*/

GCDOMTreeWidget::~GCDOMTreeWidget()
{
  delete m_domDoc;
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem* GCDOMTreeWidget::currentGCItem()
{
  return dynamic_cast< GCTreeWidgetItem* >( currentItem() );
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::constructElementHierarchy( const QString &baseElementName )
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
    processNextElement( baseElementName );
  }

  expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::processNextElement( const QString &element )
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

void GCDOMTreeWidget::addItem( const QString &element )
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

void GCDOMTreeWidget::insertItem( const QString &element, int index )
{
  GCTreeWidgetItem *item = new GCTreeWidgetItem( element );

  if( m_isEmpty )
  {
    invisibleRootItem()->addChild( item );  // takes ownership
    m_isEmpty = false;
  }
  else
  {
    currentItem()->insertChild( index, item );
  }

  setCurrentItem( item );
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::emitGCItemClicked( QTreeWidgetItem *item, int column )
{
  emit gcItemClicked(  dynamic_cast< GCTreeWidgetItem* >( item ), column );
}

/*--------------------------------------------------------------------------------------*/

void GCDOMTreeWidget::clearAndReset()
{
  this->clear();
  m_domDoc->clear();
  m_isEmpty = true;
}

/*--------------------------------------------------------------------------------------*/
