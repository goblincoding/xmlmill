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
#include "utils/gcmessagespace.h"
#include "utils/gcglobalspace.h"

#include <QApplication>
#include <QDomDocument>
#include <QAction>
#include <QMouseEvent>
#include <QInputDialog>

#include <QDebug>

/*--------------------------------------------------------------------------------------*/

GCDomTreeWidget::GCDomTreeWidget( QWidget *parent ) :
  QTreeWidget    ( parent ),
  m_activeItem   ( NULL ),
  m_domDoc       ( new QDomDocument ),
  m_commentNode  (),
  m_isEmpty      ( true ),
  m_busyIterating( false ),
  m_busyDropping ( false ),
  m_items        ()
{
  setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );
  setSelectionMode( QAbstractItemView::SingleSelection );
  setDragDropMode( QAbstractItemView::InternalMove );

  QAction *rename = new QAction( "Rename element", this );
  addAction( rename );
  connect( rename, SIGNAL( triggered() ), this, SLOT( renameItem() ) );

  QAction *remove = new QAction( "Remove element", this );
  addAction( remove );
  connect( remove, SIGNAL( triggered() ), this, SLOT( removeItem() ) );

  setContextMenuPolicy( Qt::ActionsContextMenu );

  connect( this, SIGNAL( currentItemChanged( QTreeWidgetItem*,QTreeWidgetItem* ) ), this, SLOT( currentGcItemChanged( QTreeWidgetItem*,QTreeWidgetItem* ) ) );
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
  return m_activeItem;
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

QString GCDomTreeWidget::activeCommentValue() const
{
  if( !m_commentNode.isNull() )
  {
    /* Check if the comment is an actual comment or if it's valid XML that's been
      commented out. */
    QDomDocument doc;

    if( !doc.setContent( m_commentNode.nodeValue() ) )
    {
      return m_commentNode.nodeValue();
    }
  }

  return QString();
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::setActiveCommentValue( const QString &value )
{
  /* Check if we're editing an existing comment, or if we should add a new one. */
  if( !m_commentNode.isNull() )
  {
    if( !value.isEmpty() )
    {
      m_commentNode.setNodeValue( value );
    }
    else
    {
      m_comments.removeAll( m_commentNode );
      m_commentNode.parentNode().removeChild( m_commentNode );
    }
  }
  else
  {
    if( m_activeItem &&
        !value.isEmpty() )
    {
      QDomComment comment = m_domDoc->createComment( value );
      m_activeItem->element().parentNode().insertBefore( comment, m_activeItem->element() );
      m_comments.append( comment );
      emitGcCurrentItemChanged( m_activeItem, 0 );
    }
  }

  emitGcCurrentItemChanged( m_activeItem, 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::getIncludedTreeWidgetItems( QList< GCTreeWidgetItem* > &includedItems ) const
{
  for( int i = 0; i < m_items.size(); ++i )
  {
    GCTreeWidgetItem* localItem = m_items.at( i );

    if( !localItem->elementExcluded() )
    {
      includedItems.append( localItem );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

const QList< GCTreeWidgetItem* > &GCDomTreeWidget::allTreeWidgetItems() const
{
  return m_items;
}

/*--------------------------------------------------------------------------------------*/

int GCDomTreeWidget::findItemPositionAmongDuplicates( const QString &nodeText, int itemIndex ) const
{
  QList< int > indices;

  if( !m_isEmpty )
  {
    /* If there are multiple nodes with the same element name (more likely than not), check which
    of these nodes are exact duplicates with regards to attributes, values, etc. */
    for( int i = 0; i < m_items.size(); ++i )
    {
      GCTreeWidgetItem *treeItem = m_items.at( i );

      if( treeItem->toString() == nodeText )
      {
        indices.append( treeItem->index() );
      }
    }
  }

  /* Now that we have a list of all the indices matching identical nodes (indices are a rough
    indication of an element's position in the DOM and closely matches the "line numbers" of the
    items in the tree widget), we can determine the position of the selected DOM element relative
    to its doppelgangers and highlight its text representation in the text edit area. */
  qSort( indices.begin(), indices.end() );

  return indices.indexOf( itemIndex );
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

bool GCDomTreeWidget::isCurrentItemRoot() const
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

bool GCDomTreeWidget::isBatchProcessSuccess() const
{
  return GCDataBaseInterface::instance()->batchProcessDomDocument( m_domDoc );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::updateItemNames( const QString &oldName, const QString &newName )
{
  for( int i = 0; i < m_items.size(); ++i )
  {
    if( m_items.at( i )->name() == oldName )
    {
      GCTreeWidgetItem* item = const_cast< GCTreeWidgetItem* >( m_items.at( i ) );
      item->rename( newName );
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
  invisibleRootItem()->addChild( item );  // takes ownership
  m_items.append( item );
  m_isEmpty = false;

  processNextElement( item, item->element().firstChildElement() );

  m_comments.clear();
  populateCommentList( m_domDoc->documentElement() );

  emitGcCurrentItemSelected( item, 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::appendSnippet( GCTreeWidgetItem *parentItem, QDomElement childElement )
{
  parentItem->element().appendChild( childElement );
  processNextElement( parentItem, childElement );
  populateCommentList( childElement );
  updateIndices();
  emitGcCurrentItemSelected( currentItem(), 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::replaceItemsWithComment( const QList< int > &indices, const QString &comment )
{  
  QList< GCTreeWidgetItem* > itemsToDelete;
  GCTreeWidgetItem *commentParentItem = NULL;

  for( int i = 0; i < m_items.size(); ++i )
  {
    GCTreeWidgetItem *item = m_items.at( i );

    for( int j = 0; j < indices.size(); ++j )
    {
      if( item->index() == indices.at( j ) )
      {
        /* This works because the indices are always sorted from small to big, i.e.
          the item corresponding to the lowest index in indices will be the furthest up
          the node hierarchy. */
        if( j == 0 && item->gcParent() )
        {
          commentParentItem = item->gcParent();
        }

        /* Remove the element from the DOM first. */
        QDomNode parentNode = item->element().parentNode();
        parentNode.removeChild( item->element() );

        /* Now whack it. */
        if( item->gcParent() )
        {
          GCTreeWidgetItem *parentItem = item->gcParent();
          parentItem->removeChild( item );
        }
        else
        {
          invisibleRootItem()->removeChild( item );
        }

        /* Removing an item from another's child list does not delete it. */
        itemsToDelete.append( item );
      }
    }
  }

  /* Delete the items here so that we may be sure that they are actually removed
    from the items list as well (deleting items in the loop above resulted in parent
    items deleting all their children, but obviously not updating the items list in
    the process). */
  for( int i = 0; i < itemsToDelete.size(); ++ i )
  {
    GCTreeWidgetItem *item = itemsToDelete.at( i );
    removeFromList( item );

    if( item )
    {
      delete item;
      item = NULL;
    }
  }

  /* Create a comment node with the combined text of all the item nodes that were removed
    and insert it in the correct position in the DOM document. */
  QDomComment newComment = m_domDoc->createComment( comment );

  if( commentParentItem )
  {
    commentParentItem->element().appendChild( newComment );
  }
  else
  {
    m_domDoc->appendChild( newComment );
  }

  m_comments.append( newComment );
  m_isEmpty = m_items.isEmpty();
  updateIndices();
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::replaceCommentWithItems( const QString &comment )
{
  QDomComment commentNode;

  for( int i = 0; i < m_comments.size(); ++i )
  {
    if( m_comments.at( i ).nodeValue() == comment )
    {
      commentNode = m_comments.at( i );
      break;
    }
  }

  if( !commentNode.isNull() )
  {
    /* Ensure that we get the previous element parent (e.g. the comment may have another comment
      node as parent). */
    QDomNode parentElement = commentNode.parentNode();

    while( !parentElement.isNull() &&
           !parentElement.isElement() )
    {
      parentElement = parentElement.parentNode();
    }

    GCTreeWidgetItem *parentItem = gcItemFromNode( parentElement );

    if( parentItem )
    {
      /* What happens when setting a bunch of sibling items as DOM content is that the DOM document
        only recognises the first item, assigning it to the root position.  This is a workaround
        to ensure that these cases are catered for and that sibling items wrapped in comment tags
        are added correctly. */
      QString parentItemText;
      QTextStream stream( &parentItemText );
      parentItem->element().save( stream, 2 );
      parentItemText = parentItemText.replace( QString( "<!--%1-->").arg( comment ), comment );

      /* We can't remove the comment node before we've obtained the text above (or else it's obviously
        excluded from its parent and won't show). */
      commentNode.parentNode().removeChild( commentNode );
      m_comments.removeAll( commentNode );

      if( parentItem->gcParent() )
      {
        GCTreeWidgetItem *oldItem = parentItem;
        parentItem = parentItem->gcParent();
        parentItem->removeChild( oldItem );

        parentElement = parentItem->element();
        parentElement.removeChild( oldItem->element() );

        removeFromList( oldItem );
      }

      QDomDocument doc;

      if( doc.setContent( parentItemText ) )
      {
        /* Ensure that everything in the new snippet is known to the database. */
        GCDataBaseInterface::instance()->batchProcessDomDocument( &doc );
        appendSnippet( parentItem, doc.documentElement().cloneNode().toElement() );
      }

      updateIndices();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::processNextElement( GCTreeWidgetItem *parentItem, QDomElement element )
{
  if( parentItem )
  {
    while( !element.isNull() )
    {
      GCTreeWidgetItem *item = new GCTreeWidgetItem( element, m_items.size() );
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

  setCurrentItem( m_activeItem->parent() );  // required to enforce sibling relationships
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::addItem( const QString &element, bool toParent )
{
  if( m_activeItem )
  {
    if( toParent )
    {
      insertItem( element, m_activeItem->parent()->indexOfChild( m_activeItem ), toParent );
    }
    else
    {
      insertItem( element, m_activeItem->childCount() - 1, toParent );
    }
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
      m_activeItem->insertGcChild( index, item );
    }
    else
    {
      m_activeItem->gcParent()->insertGcChild( index, item );
    }
  }

  /* I will have to rethink this approach if it turns out that it is too expensive to
    iterate through the tree on each and every addition...for now, this is the easiest
    solution, even if not the best. */
  updateIndices();

  setCurrentItem( item );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::setCurrentItemToMatchIndex( int index )
{
  index = ( index < 0 ) ? 0 : index;

  for( int i = 0; i < m_items.size(); ++i )
  {
    if( m_items.at( i )->index() == index )
    {
      emitGcCurrentItemSelected( m_items.at( i ), 0, false );
      break;
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::setAllCheckStates( Qt::CheckState state )
{
  m_busyIterating = true;

  QTreeWidgetItemIterator iterator( this );

  while( *iterator )
  {
    ( *iterator )->setCheckState( 0, state );
    ++iterator;
  }

  m_busyIterating = false;
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::setShowTreeItemsVerbose( bool verbose )
{
  m_busyIterating = true;

  QTreeWidgetItemIterator iterator( this );

  while( *iterator )
  {
    GCTreeWidgetItem* treeItem = dynamic_cast< GCTreeWidgetItem* >( *iterator );
    treeItem->setVerbose( verbose );
    ++iterator;
  }

  m_busyIterating = false;
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::updateIndices()
{
  m_busyIterating = true;

  QTreeWidgetItemIterator iterator( this );
  int index = 0;

  while( *iterator )
  {
    GCTreeWidgetItem* treeItem = dynamic_cast< GCTreeWidgetItem* >( *iterator );
    treeItem->setIndex( index );
    ++index;
    ++iterator;
  }

  m_busyIterating = false;
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem *GCDomTreeWidget::gcItemFromNode( QDomNode element )
{
  GCTreeWidgetItem *parentItem = NULL;

  if( !element.isNull() )
  {
    for( int i = 0; i < m_items.size(); ++i )
    {
      if( m_items.at( i )->element() == element )
      {
        parentItem = m_items.at( i );
        break;
      }
    }
  }

  return parentItem;
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::removeFromList( GCTreeWidgetItem *item )
{
  for( int i = 0; i < item->childCount(); ++i )
  {
    GCTreeWidgetItem *childItem = item->gcChild( i );
    removeFromList( childItem );
  }

  m_items.removeAll( item );
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::populateCommentList( QDomNode node )
{
  QDomNode childNode = node.firstChild();

  while( !childNode.isNull() )
  {
    if( childNode.isComment() )
    {
      m_comments.append( childNode.toComment() );
    }

    populateCommentList( childNode );
    childNode = childNode.nextSibling();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::dropEvent( QDropEvent *event )
{
  m_busyDropping = true;

  QTreeWidget::dropEvent( event );
  DropIndicatorPosition indicatorPos = dropIndicatorPosition();

  if( m_activeItem )
  {
    QDomElement previousParent = m_activeItem->element().parentNode().toElement();
    previousParent.removeChild( m_activeItem->element() );

    GCTreeWidgetItem *parent = m_activeItem->gcParent();

    if( parent )
    {
      if( indicatorPos == QAbstractItemView::OnItem )
      {
        parent->element().appendChild( m_activeItem->element() );
      }
      else if( indicatorPos == QAbstractItemView::AboveItem ||
               indicatorPos == QAbstractItemView::BelowItem )
      {
        GCTreeWidgetItem *sibling = NULL;
        int pos = parent->indexOfChild( m_activeItem );

        if( pos > 0 )
        {
          sibling = parent->gcChild( pos - 1 );

          if( sibling )
          {
            parent->element().insertAfter( m_activeItem->element(), sibling->element() );
          }
        }
        else if( pos == 0 )
        {
          sibling = parent->gcChild( pos + 1 );

          if( sibling )
          {
            parent->element().insertBefore( m_activeItem->element(), sibling->element() );
          }
        }
        else
        {
          parent->element().appendChild( m_activeItem->element() );
        }
      }

      /* Update the database so reflect the re-parenting. */
      GCDataBaseInterface::instance()->updateElementChildren( parent->name(), QStringList( m_activeItem->name() ) );
    }

    expandItem( parent );
  }

  updateIndices();
  emitGcCurrentItemChanged( m_activeItem, 0 );
  m_busyDropping = false;
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::keyPressEvent( QKeyEvent *event )
{
  if( event->key() == Qt::Key_Delete )
  {
    removeItem();
  }
  else if( event->key() == Qt::Key_Up ||
           event->key() == Qt::Key_Down )
  {
    QTreeWidget::keyPressEvent( event );
    emitGcCurrentItemSelected( m_activeItem, 0 );
  }
  else
  {
    QTreeWidget::keyPressEvent( event );
  }
}
/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::currentGcItemChanged( QTreeWidgetItem *current, QTreeWidgetItem *previous )
{
  Q_UNUSED( previous );

  if( !m_busyDropping )
  {
    m_activeItem = dynamic_cast< GCTreeWidgetItem* >( current );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::emitGcCurrentItemSelected( QTreeWidgetItem *item, int column, bool highlightElement )
{
  if( !m_busyIterating )
  {
    setCurrentItem( item, column );

    if( m_activeItem )
    {
      /* Returns NULL object if not a comment. */
      m_commentNode = m_activeItem->element().previousSibling().toComment();
    }

    emit gcCurrentItemSelected( m_activeItem, column, highlightElement );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::emitGcCurrentItemChanged( QTreeWidgetItem *item, int column )
{
  if( !m_busyIterating )
  {
    setCurrentItem( item, column );

    if( m_activeItem )
    {
      /* Returns NULL object if not a comment. */
      m_commentNode = m_activeItem->element().previousSibling().toComment();
    }

    emit gcCurrentItemChanged( m_activeItem, column );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::renameItem()
{
  QString newName = QInputDialog::getText( this, "Change element name", "Enter the element's new name:" );

  if( !newName.isEmpty() && m_activeItem )
  {
    QString oldName = m_activeItem->name();
    m_activeItem->rename( newName );
    updateItemNames( oldName, newName );

    /* The name change may introduce a new element too so we can safely call "addElement" below as
       it doesn't do anything if the element already exists in the database, yet it will obviously
       add the element if it doesn't.  In the latter case, the children  and attributes associated with
       the old name will be assigned to the new element in the process. */
    QStringList attributes = GCDataBaseInterface::instance()->attributes( oldName );
    QStringList children = GCDataBaseInterface::instance()->children( oldName );

    if( !GCDataBaseInterface::instance()->addElement( newName, children, attributes ) )
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    }

    /* If we are, in fact, dealing with a new element, we also want the "new" element's associated attributes
      to be updated with the known values of these attributes. */
    foreach( QString attribute, attributes )
    {
      QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( oldName, attribute );

      if( !GCDataBaseInterface::instance()->updateAttributeValues( newName, attribute, attributeValues ) )
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
      }
    }

    emitGcCurrentItemChanged( m_activeItem, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomTreeWidget::removeItem()
{
  if( m_activeItem )
  {
    /* I think it is safe to assume that comment nodes will exist just above an element
      although it might not always be the case that a multi-line comment exists within
      a single set of comment tags.  However, for those cases, it's the user's responsibility
      to clean them up. */
    if( !m_commentNode.isNull() )
    {
      /* Check if the comment is an actual comment or if it's valid XML that's been
        commented out. */
      QDomDocument doc;

      if( !doc.setContent( m_commentNode.nodeValue() ) )
      {
        m_comments.removeAll( m_commentNode );
        m_commentNode.parentNode().removeChild( m_commentNode );
      }
    }

    /* Remove the element from the DOM first. */
    QDomNode parentNode = m_activeItem->element().parentNode();
    parentNode.removeChild( m_activeItem->element() );

    /* Now whack it. */
    if( m_activeItem->gcParent() )
    {
      GCTreeWidgetItem *parentItem = m_activeItem->gcParent();
      parentItem->removeChild( m_activeItem );
    }
    else
    {
      invisibleRootItem()->removeChild( m_activeItem );
    }

    removeFromList( m_activeItem );
    m_isEmpty = m_items.isEmpty();

    delete m_activeItem;
    m_activeItem = gcCurrentItem();

    updateIndices();
    emitGcCurrentItemChanged( m_activeItem, 0 );
  }
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
