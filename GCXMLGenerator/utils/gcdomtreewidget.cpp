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

#include <QApplication>
#include <QDomDocument>

/*--------------------------------------------------------------------------------------*/

GCDomTreeWidget::GCDomTreeWidget( QWidget *parent ) :
  QTreeWidget( parent ),
  m_domDoc   ( new QDomDocument ),
  m_isEmpty  ( true ),
  m_items    ()
{
  connect( this, SIGNAL( itemClicked( QTreeWidgetItem*,int ) ), this, SLOT( emitGcCurrentItemSelected( QTreeWidgetItem*,int ) ) );
  connect( this, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( emitGcCurrentItemSelected( QTreeWidgetItem*, int ) ) );
  connect( this, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( emitGcCurrentItemChanged( QTreeWidgetItem*, int ) ) );
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

QDomNode GCDomTreeWidget::cloneDocument() const
{
  return m_domDoc->documentElement().cloneNode();
}

/*--------------------------------------------------------------------------------------*/

QString GCDomTreeWidget::toString() const
{
  return m_domDoc->toString( 2 );
}

/*--------------------------------------------------------------------------------------*/

QString GCDomTreeWidget::rootName() const
{
  return m_domDoc->documentElement().tagName();
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

const QList< GCTreeWidgetItem* > &GCDomTreeWidget::allTreeWidgetItems() const
{
  return m_items;
}

/*--------------------------------------------------------------------------------------*/

QList< int > GCDomTreeWidget::findIndicesMatching( const GCTreeWidgetItem *item ) const
{
  QList< int > indices;

  if( !m_isEmpty )
  {
    QString stringToMatch = item->toString();

    /* If there are multiple nodes with the same element name (more likely than not), check which
    of these nodes are exact duplicates with regards to attributes, values, etc. */
    for( int i = 0; i < m_items.size(); ++i )
    {
      GCTreeWidgetItem *treeItem = m_items.at( i );

      if( treeItem->toString() == stringToMatch )
      {
        indices.append( treeItem->index() );
      }
    }

  }

  return indices;
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::setContent( const QString &text, QString *errorMsg, int *errorLine, int *errorColumn )
{
  clearAndReset();

  if( !m_domDoc->setContent( text, errorMsg, errorLine, errorColumn ) )
  {
    return false;
  }

  rebuildTreeWidget();
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::isEmpty() const
{
  return m_isEmpty;
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::currentItemIsRoot() const
{
  if( gcCurrentItem() )
  {
    return ( gcCurrentItem()->element() == m_domDoc->documentElement() );
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::matchesRootName( const QString &elementName ) const
{
  return ( elementName == m_domDoc->documentElement().tagName() );
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::isDocumentCompatible() const
{
  return GCDataBaseInterface::instance()->isDocumentCompatible( m_domDoc );
}

/*--------------------------------------------------------------------------------------*/

bool GCDomTreeWidget::batchProcessSuccess() const
{
  return GCDataBaseInterface::instance()->batchProcessDOMDocument( m_domDoc );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::updateItemNames( const QString &oldName, const QString &newName )
{
  for( int i = 0; i < m_items.size(); ++i )
  {
    if( m_items.at( i )->name() == oldName )
    {
      GCTreeWidgetItem* item = const_cast< GCTreeWidgetItem* >( m_items.at( i ) );
      item->setText( 0, newName );
      item->element().setTagName( newName );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::rebuildTreeWidget()
{
  clear();    // ONLY whack the tree widget items.
  m_items.clear();

  /* Set the document root as the first item in the tree. */
  GCTreeWidgetItem *item = new GCTreeWidgetItem( m_domDoc->documentElement(), m_items.size() );
  item->setFlags( item->flags() | Qt::ItemIsEditable );
  invisibleRootItem()->addChild( item );  // takes ownership
  m_items.append( item );
  m_isEmpty = false;

  processNextElement( item, item->element().firstChildElement() );
  emitGcCurrentItemSelected( item, 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::appendSnippet( GCTreeWidgetItem *parentItem, QDomElement element )
{
  parentItem->element().appendChild( element );
  processNextElement( parentItem, element );
  updateIndices();
  emitGcCurrentItemSelected( currentItem(), 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::processNextElement( GCTreeWidgetItem *parentItem, QDomElement element )
{
  if( parentItem )
  {
    while( !element.isNull() )
    {
      GCTreeWidgetItem *item = new GCTreeWidgetItem( element, m_items.size() );
      item->setFlags( item->flags() | Qt::ItemIsEditable );
      parentItem->addChild( item );  // takes ownership
      m_items.append( item );

      processNextElement( item, element.firstChildElement() );
      element = element.nextSiblingElement();
    }

    qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
  }
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
  emitGcCurrentItemSelected( currentItem(), 0 );
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

void GCDomTreeWidget::addItem( const QString &element, bool toParent )
{
  if( currentItem() )
  {
    insertItem( element, currentItem()->childCount(), toParent );
  }
  else
  {
    insertItem( element, 0, toParent );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::insertItem( const QString &elementName, int index, bool toParent )
{
  QDomElement element = m_domDoc->createElement( elementName );

  /* Create all the possible attributes for the element here, they can be changed
    later on. */
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName );

  for( int i = 0; i < attributeNames.count(); ++i )
  {
    element.setAttribute( attributeNames.at( i ), "" );
  }

  GCTreeWidgetItem *item = new GCTreeWidgetItem( element, m_items.size() );
  m_items.append( item );

  /* I will have to rethink this approach if it turns out that it is too expensive to
    iterate through the tree on each and every addition...for now, this is the easiest
    solution, even if not the best. */
  updateIndices();

  if( m_isEmpty )
  {
    invisibleRootItem()->addChild( item );  // takes ownership
    m_domDoc->appendChild( element );
    m_isEmpty = false;
  }
  else
  {
    if( !toParent )
    {
      currentItem()->insertChild( index, item );
      gcCurrentItem()->element().appendChild( element );
    }
    else
    {
      currentItem()->parent()->insertChild( index, item );
      gcCurrentItem()->gcParent()->element().appendChild( element );
    }
  }

  setCurrentItem( item );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::removeItem( GCTreeWidgetItem *item )
{
  if( item )
  {
    /* Remove the element from the DOM first. */
    QDomNode parentNode = item->element().parentNode();
    parentNode.removeChild( item->element() );

    GCTreeWidgetItem *parentItem = item->gcParent();
    parentItem->removeChild( item );
    m_items.removeAll( item );

    updateIndices();
    emitGcCurrentItemSelected( currentItem(), 0 );    
  }

  m_isEmpty = m_items.isEmpty();
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::addComment( GCTreeWidgetItem *item, const QString &text )
{
  if( item )
  {
    QDomComment comment = m_domDoc->createComment( text );
    item->element().parentNode().insertBefore( comment, item->element() );
  }
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

void GCDomTreeWidget::updateIndices()
{
  QTreeWidgetItemIterator iterator( this );
  int index = 0;

  while( *iterator )
  {
    GCTreeWidgetItem* treeItem = dynamic_cast< GCTreeWidgetItem* >( *iterator );
    treeItem->setIndex( index );
    ++index;
    ++iterator;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::emitGcCurrentItemSelected( QTreeWidgetItem *item, int column )
{
  setCurrentItem( item, column );
  emit gcCurrentItemSelected( dynamic_cast< GCTreeWidgetItem* >( item ), column );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::emitGcCurrentItemChanged( QTreeWidgetItem *item, int column )
{
  setCurrentItem( item, column );
  emit gcCurrentItemSelected( dynamic_cast< GCTreeWidgetItem* >( item ), column );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::clearAndReset()
{
  clear();
  m_domDoc->clear();
  m_items.clear();
  m_isEmpty = true;
}

/*--------------------------------------------------------------------------------------*/
