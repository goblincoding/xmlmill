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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef DOMTREEWIDGET_H
#define DOMTREEWIDGET_H

#include "db/dbinterface.h"

#include <QTreeWidget>
#include <QDomComment>

class TreeWidgetItem;
class QDomDocument;
class QDomElement;
class QDomNode;

/// Specialist tree widget class consisting of TreeWidgetItems.

/** This class wraps an underlying DOM document and manages its items
 * (TreeWidgetItems) based on changes to the DOM, but also manages the DOM based
 * on changes made to its items.  Perhaps a better way of describing this
 * relationship is to say that this class is, in effect, a non-textual visual
 * representation of a DOM document. */

class DomTreeWidget : public QTreeWidget {
  Q_OBJECT
public:
  /*! Constructor. */
  DomTreeWidget(QWidget *parent = 0);

  /*! Destructor. */
  ~DomTreeWidget();

  /*! Returns the current item as a TreeWidgetItem. */
  TreeWidgetItem *CurrentItem() const;

  /*! Returns a list of all the included \sa TreeWidgetItems in the tree (i.e.
     all the items that do not have their "exclude" flags set).
      \sa allTreeWidgetItems */
  QList<TreeWidgetItem *> includedTreeWidgetItems() const;

  /*! Returns a list of ALL the TreeWidgetItems currently in the tree.
      \sa getIncludedTreeWidgetItems */
  const QList<TreeWidgetItem *> &allTreeWidgetItems() const;

  /*! Returns the position of "itemIndex" relative to that of ALL items matching
   * "nodeText" (this is is not as odd as it sounds, it is possible that a DOM
   * document may have multiple elements of the same name with matching
   * attributes and attribute values). */
  int itemPositionRelativeToIdenticalSiblings(const QString &nodeText,
                                              int itemIndex) const;

  /*! Returns a deep copy of the underlying DOM document. */
  QDomNode cloneDocument() const;

  /*! Returns the DOM content as string. */
  QString toString() const;

  /*! Returns the name of the DOM document's root.
      \sa currentItemIsRoot
      \sa matchesRootName */
  QString rootName() const;

  /*! Returns the comment associated with the current element (if any).  If no
     comment precedes
      the current element, an empty string is returned.
      \sa setActiveCommentValue */
  QString activeCommentValue() const;

  /*! Sets the value of the active comment node to "value".  If the active
     element doesn't have
      an associated comment, a comment node is created.
      \sa activeCommentValue */
  void setActiveCommentValue(const QString &value);

  /*! Sets the underlying DOM document's content.  If successful, a recursive
     DOM tree traversal is kicked off in order to populate the tree widget with
     the information contained in the active DOM document.
      \sa processNextElement */
  bool setContent(const QString &text, QString *errorMsg = 0,
                  int *errorLine = 0, int *errorColumn = 0);

  /*! Returns true if the widget and DOM is currently empty. */
  bool empty() const;

  /*! Returns true if the current item is the one corresponding to the DOM
     document's root element.
      \sa matchesRootName
      \sa rootName */
  bool currentItemIsRoot() const;

  /*! Returns true if "elementName" matches that of the DOM document's root.
      \sa isCurrentItemRoot
      \sa rootName */
  bool matchesRootName(const QString &elementName) const;

  /*! Returns true if the underlying DOM document is compatible with the active
   * DB session. */
  bool documentCompatible();

  /*! Returns true if batch processing of DOM content to the active DB was
   * successful. */
  bool batchProcessSuccess();

  /*! Rebuild the tree to conform to updated DOM content.
      \sa processNextElement */
  void rebuildTreeWidget();

  /*! Creates and adds tree widget items for each element in the parameter
     element hierarchy. The process starts by appending "childElement" to
     "parentItem's" corresponding element and then recursively creates and adds
     items with associated elements corresponding to "childElement's" element
     hierarchy.
      \sa processNextElement */
  void appendSnippet(TreeWidgetItem *parentItem, QDomElement childElement);

  /*! Removes the items with indices matching those in the parameter list from
   * the tree as well as from the DOM document and replaces them with a new
   * QDomComment node representing "comment". */
  void replaceItemsWithComment(const QList<int> &indices,
                               const QString &comment);

  /*! Update all the tree widget items with text "oldName" to text "newName" */
  void updateItemNames(const QString &oldName, const QString &newName);

  /*! This function starts the recursive process of populating the tree widget
     with items consisting of the element hierarchy starting at
     "baseElementName". If "baseElementName" is empty, a complete hierarchy of
     the current active profile will be constructed. This method also
     automatically clears and resets DomTreeWidget's state, expands the entire
     tree, sets the first top level item as current and emits the
     "CurrentItemSelected" signal.
      \sa processNextElement */
  void populateFromDatabase(const QString &baseElementName = QString());

  /*! Adds a new item and corresponding DOM element node named "element". If the
     tree is empty, the new item will be added to the invisible root (i.e. as
     header item), otherwise it will be added as a child of the current item
     (the new item also becomes the current item). If "toParent" is true, the
     new item will be added as a child of the current item's parent (i.e. as a
     sibling to the current item).
      \sa insertItem */
  void addItem(const QString &element, bool toParent = false);

  /*! Adds a new item and corresponding DOM element node named "elementName" to
     the tree and DOM document and inserts the new tree widget item into
     position "index" of the current item. If the tree is empty, the new item
     will be added to the invisible root (the new item also becomes the current
     item). If "toParent" is true, the new item will be added as a child to the
     current item's parent (i.e. as a sibling to the current item).
      \sa addItem */
  void insertItem(const QString &elementName, int index, bool toParent = false);

  /*! Iterates through the tree and sets all items' check states to "state". */
  void setAllCheckStates(Qt::CheckState state);

  /*! Iterates through the tree and set all items' "verbose" flags to "show". */
  void setShowTreeItemsVerbose(bool verbose);

  /*! Clears and resets the tree as well as the underlying DOM document. */
  void clearAndReset();

public slots:
  /*! Finds the item with index matching "index" and sets it as the current tree
   * item ("index" is the item's position relative to the first active XML
   * element, i.e. excluding "non-active" comment nodes). */
  void setCurrentItemFromIndex(int index);

signals:
  /*! Emitted when the current active item changes.
      \sa emitGcCurrentItemSelected
      \sa CurrentItemChanged */
  void CurrentItemSelected(TreeWidgetItem *item, int column);

  /*! Emitted when the current item's content changes
      \sa emitGcCurrentItemChanged
      \sa CurrentItemSelected */
  void CurrentItemChanged(TreeWidgetItem *item, int column);

protected:
  /*! Re-implemented from QTreeWidget. */
  void dropEvent(QDropEvent *event);

  /*! Re-implemented from QTreeWidget. */
  void keyPressEvent(QKeyEvent *event);

private slots:
  /*! Connected to QTreeWidget::currentItemChanged(), sets the active
   * TreeWidgetItem. */
  void currentGcItemChanged(QTreeWidgetItem *current,
                            QTreeWidgetItem *previous);

  /*! Connected to "itemClicked" and "itemActivated". Re-emits the clicked item
     as a TreeWidgetItem.
      \sa CurrentItemSelected */
  void emitGcCurrentItemSelected(QTreeWidgetItem *item, int column);

  /*! Connected to "itemChanged". Re-emits the changed item as a TreeWidgetItem.
      \sa CurrentItemChanged */
  void emitGcCurrentItemChanged(QTreeWidgetItem *item, int column);

  /*! Connected to a context menu action.  Renames the element on which the
   * context menu action was invoked. An element with the new name will be added
   * to the DB (if it doesn't yet exist) with the same associated attributes and
   * attribute values as the element name it is replacing (the "old" element
   * will not be removed from the DB). All occurrences of the old name
   * throughout the current DOM will be replaced with the new name and the tree
   * widget will be updated accordingly. */
  void renameItem();

  /*! Connected to a context menu action. Removes the item (and it's
   * corresponding element) on which the context menu action was invoked from
   * the tree and underlying DOM. This function will furthermore remove all
   * comment nodes directly above the element node. */
  void removeItem();

  /*! Connected to a context menu action.  Moves the active (selected) item to
     the level of its parent.
      \sa stepDown */
  void stepUp();

  /*! Connected to a context menu action.  Moves the active (selected) item to
     the level of its children.
      \sa stepUp */
  void stepDown();

  /*! Connected to a context menu action.  Expands active (selected) item.
      \sa collapse */
  void expand();

  /*! Connected to a context menu action.  Collapses the active (selected) item.
      \sa expand */
  void collapse();

private:
  /*! Creates a new TreeWidgetItem item with corresponding "element" and adds it
      as a child to "parentItem".
      \sa setContent
      \sa appendSnippet
      \sa rebuildTreeWidget */
  void processElement(TreeWidgetItem *parentItem, QDomElement element);

  /*! Processes individual elements.  This function is called recursively from
     within "populateFromDatabase", creating a representative tree widget item
     (and corresponding DOM element) named "element" and adding it (the item) to
     the correct parent.
      @param element - the name of the element for which a tree widget item must
     be created.
      \sa populateFromDatabase */
  void processElementFromDatabase(const QString &element);

  /*! Used in "processElementFromDatabase" to prevent infinite recursion.
     Returns true when
      an element is part of a hierarchy containing itself (oddly allowed in
     XML). */
  bool parentTreeAlreadyContainsElement(const TreeWidgetItem *item,
                                        const QString &element);

  /*! Populates the comments list with all the comment nodes in the document. */
  void populateCommentList(QDomNode node);

  /*! Iterates through the tree and updates all items' indices (useful when new
   * items are added or items removed) to ensure that indices correspond roughly
   * to "row numbers" in the accompanying plain text representation of the
   * document's content. */
  void updateIndices();

  /*! Finds and returns the TreeWidget item that is linked to "element". */
  TreeWidgetItem *ItemFromNode(QDomNode element);

  /*! Recursively removes item and its children from the internal TreeWidgetItem
   * list. */
  void removeFromList(TreeWidgetItem *item);

  TreeWidgetItem *m_activeItem;
  DB m_db;
  QDomDocument *m_domDoc;
  QDomComment m_commentNode;
  bool m_isEmpty;
  bool m_busyIterating;
  bool m_itemBeingManipulated;

  QList<TreeWidgetItem *> m_items;
  QList<QDomComment> m_comments;
};

#endif // DOMTREEWIDGET_H
