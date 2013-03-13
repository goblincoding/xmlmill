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

#ifndef GCDOMTREEWIDGET_H
#define GCDOMTREEWIDGET_H

#include <QTreeWidget>

class GCTreeWidgetItem;
class QDomDocument;
class QDomElement;
class QDomNode;

class GCDomTreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  /*! Constructor. */
  GCDomTreeWidget( QWidget *parent = 0 );

  /*! Destructor. */
  ~GCDomTreeWidget();

  /*! Returns the current item as a GCTreeWidgetItem. */
  GCTreeWidgetItem* gcCurrentItem() const;

  /*! Returns a list of all the included GCTreeWidgetItems in the tree (in other
      words those items with the "include" flags set).
      \sa allTreeWidgetItems */
  QList< GCTreeWidgetItem* > includedGcTreeWidgetItems() const;

  /*! Returns a list of ALL the GCTreeWidgetItems currently in the tree.
      \sa includedGcTreeWidgetItems */
  const QList< GCTreeWidgetItem* > &allTreeWidgetItems() const;

  /*! Returns the position of "itemIndex" relative to that of ALL item's matching "nodeText"
      (this is is not as odd as it sounds, it is possible that a DOM document may have
      multiple elements of the same name with matching attributes and attribute values). */
  int findItemPositionAmongDuplicates( const QString &nodeText, int itemIndex ) const;

  /*! Returns a deep copy of the underlying DOM document. */
  QDomNode cloneDocument() const;

  /*! Returns the DOM content as string. */
  QString toString() const;

  /*! Returns the name of the DOM document's root.
      \sa currentItemIsRoot
      \sa matchesRootName */
  QString rootName() const;

  /*! Sets the underlying DOM document's content.  If successful, a recursive DOM tree traversal
      is kicked off in order to populate the tree widget with the information contained in the
      active DOM document.
      \sa processNextElement */
  bool setContent( const QString & text, QString * errorMsg = 0, int * errorLine = 0, int * errorColumn = 0 );

  /*! Returns true if the widget and DOM is currently empty. */
  bool isEmpty() const;

  /*! Returns true if the current item is the one corresponding to the DOM document's root element.
      \sa matchesRootName
      \sa rootName */
  bool currentItemIsRoot() const;

  /*! Returns true if "elementName" matches that of the DOM document's root.
      \sa currentItemIsRoot
      \sa rootName */
  bool matchesRootName( const QString &elementName ) const;

  /*! Returns true if the underlying DOM document is compatible with the active DB session. */
  bool isDocumentCompatible() const;

  /*! Returns true if batch processing of DOM content to the active DB was successful. */
  bool batchProcessSuccess() const;

  /*! Rebuild the tree in accordance with updated DOM content.
      \sa processNextElement */
  void rebuildTreeWidget();

  /*! Creates and adds tree widget items for each element in the parameter element hierarchy.
      The process starts by appending "childElement" to "parentItem's" corresponding element
      and then recursively creates and adds items with associated elements corresponding to
      "childElement's" element hierarchy.
      \sa processNextElement */
  void appendSnippet( GCTreeWidgetItem *parentItem, QDomElement childElement );

  /*! Update all the tree widget items with text "oldName" to text "newName" */
  void updateItemNames( const QString &oldName, const QString &newName );

  /*! This function starts the recursive process of populating the tree widget with items
      consisting of the element hierarchy starting at "baseElementName". If "baseElementName"
      is empty, a complete hierarchy of the current active profile will be constructed. This
      method also automatically clears and resets GCDomTreeWidget's state, expands the
      entire tree, sets the first top level item as current and emits the "gcCurrentItemSelected"
      signal.
      \sa processNextElement */
  void populateFromDatabase( const QString &baseElementName = QString() );

  /*! Adds a new item and corresponding DOM element node named "element". If the tree
      is empty, the new item will be added to the invisible root (i.e. as header item),
      otherwise it will be added as a child of the current item.  The new item is also
      set as the current item. If "toParent" is true, the new item will be added as a
      child to the current item's parent (i.e. as a sibling to the current item).
      \sa insertItem
      \sa addComment */
  void addItem( const QString &element, bool toParent = false );

  /*! Adds a new item and corresponding DOM element node named "elementName" and inserts
      the new tree widget item into position "index" of the current item. If the tree
      is empty, the new item will be added to the invisible root. The new item is also set
      as the current item. If "toParent" is true, the new item will be added as a child to
      the current item's parent (i.e. as a sibling to the current item).
      \sa addItem
      \sa addComment */
  void insertItem( const QString &elementName, int index, bool toParent = false );

  /*! Adds a DOM comment to the element corresponding to "item".
      \sa addItem
      \sa insertItem */
  void addComment( GCTreeWidgetItem* item, const QString &text );

  /*! Iterates through the tree and sets all items' check states to "state". */
  void setAllCheckStates( Qt::CheckState state );

  /*! Iterates through the tree and set all items' "verbose" flags to "show". */
  void setShowTreeItemsVerbose( bool verbose );

  /*! Clears and resets the tree as well as the underlying DOM document. */
  void clearAndReset();

public slots:
  /*! Finds the item with index matching "index" and sets it as the current tree item. */
  void setCurrentItemWithIndexMatching( int index );

signals:
  /*! \sa emitGcCurrentItemSelected */
  void gcCurrentItemSelected( GCTreeWidgetItem*,int,bool );

  /*! \sa emitGcCurrentItemChanged */
  void gcCurrentItemChanged( GCTreeWidgetItem*, int );

protected:
  /*! Re-implemented from QTreeWidget. */
  void mousePressEvent( QMouseEvent *event );

  /*! Re-implemented from QTreeWidget. */
  void dropEvent( QDropEvent *event );

private slots:
  /*! Connected to "itemClicked" and "itemActivated". Re-emits the clicked item
      as a GCTreeWidgetItem.
      \sa gcCurrentItemSelected */
  void emitGcCurrentItemSelected( QTreeWidgetItem* item, int column, bool highlightElement = true );

  /*! Connected to "itemChanged". Re-emits the changed item as a GCTreeWidgetItem.
      \sa gcCurrentItemChanged */
  void emitGcCurrentItemChanged( QTreeWidgetItem* item, int column );

  /*! Renames the element on which the context menu action was invoked. An element with
      the new name will be added to the DB (if it doesn't yet exist) with the same associated
      attributes and attribute values as the element name it is replacing (the "old" element
      will not be removed from the DB). All occurrences of the old name throughout the current
      DOM will be replaced with the new name and the tree widget will be updated accordingly. */
  void renameItem();

  /*! Removes the item and it's corresponding element from the tree and underlying DOM. */
  void removeItem();

private:
  /*! Creates a new GCTreeWidgetItem item with corresponding "element" and adds it
      as a child to the "parentItem".
      \sa setContent
      \sa appendSnippet
      \sa rebuildTreeWidget */
  void processNextElement( GCTreeWidgetItem *parentItem, QDomElement element );

  /*! Processes individual elements.  This function is called recursively from within
      "populateFromDatabase", creating a representative tree widget item (and corresponding
      DOM element) named "element" and adding it (the item) to the correct parent.
      @param element - the name of the element for which a tree widget item must be created.
      \sa populateFromDatabase */
  void processNextElement( const QString &element );

  /*! Iterates through the tree and updates all items' indices (useful when new items
      are added to ensure that indices correspond roughly to "row numbers"). */
  void updateIndices();

  GCTreeWidgetItem *m_activeItem;
  QDomDocument     *m_domDoc;
  bool              m_isEmpty;
  bool              m_busyIterating;

  QList< GCTreeWidgetItem* > m_items;
};

#endif // GCDOMTREEWIDGET_H
