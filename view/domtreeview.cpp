/* Copyright (c) 2012 - 2015 by William Hallatt.
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
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */
#include "domtreeview.h"

#include <QAction>

/*----------------------------------------------------------------------------*/

DomTreeView::DomTreeView(QWidget *parent) : QTreeView(parent) {
  //setSelectionMode(QAbstractItemView::SingleSelection);
  //setDragDropMode(QAbstractItemView::InternalMove);
  setUniformRowHeights(true);

  QAction *expand = new QAction("Expand", this);
  addAction(expand);
  connect(expand, SIGNAL(triggered()), this, SLOT(expandSelection()));

  QAction *collapse = new QAction("Collapse", this);
  addAction(collapse);
  connect(collapse, SIGNAL(triggered()), this, SLOT(collapseSelection()));

  QAction *separator = new QAction(this);
  separator->setSeparator(true);
  addAction(separator);

  QAction *remove = new QAction("Remove element", this);
  addAction(remove);
  connect(remove, SIGNAL(triggered()), this, SLOT(removeItem()));

  separator = new QAction(this);
  separator->setSeparator(true);
  addAction(separator);

  QAction *stepUp = new QAction("Move up one level", this);
  addAction(stepUp);
  connect(stepUp, SIGNAL(triggered()), this, SLOT(stepUp()));

  QAction *stepDown = new QAction("Move down one level", this);
  addAction(stepDown);
  connect(stepDown, SIGNAL(triggered()), this, SLOT(stepDown()));

  setContextMenuPolicy(Qt::ActionsContextMenu);
}

/*----------------------------------------------------------------------------*/

void DomTreeView::removeItem() {
  //  if (m_activeItem) {
  //    m_itemBeingManipulated = true;

  //    /* I think it is safe to assume that comment nodes will exist just above
  // an
  //     * element although it might not always be the case that a multi-line
  //     * comment exists within a single set of comment tags.  However, for
  // those
  //     * cases, it's the user's responsibility to clean them up as we cannot
  //     * assume to know which of these comments will go with the element being
  //     * removed. */
  //    if (!m_commentNode.isNull()) {
  //      /* Check if the comment is an actual comment or if it's valid XML
  // that's
  //       * been commented out (we don't want to remove snippets). */
  //      QDomDocument doc;

  //      if (!doc.setContent(m_commentNode.nodeValue())) {
  //        m_comments.removeAll(m_commentNode);
  //        m_commentNode.parentNode().removeChild(m_commentNode);
  //      }
  //    }

  //    /* Remove the element from the DOM first. */
  //    QDomNode parentNode = m_activeItem->element().parentNode();
  //    parentNode.removeChild(m_activeItem->element());

  //    /* Now whack it. */
  //    if (m_activeItem->Parent()) {
  //      TreeWidgetItem *parentItem = m_activeItem->Parent();
  //      parentItem->removeChild(m_activeItem);
  //    } else {
  //      invisibleRootItem()->removeChild(m_activeItem);
  //    }

  //    removeFromList(m_activeItem);
  //    m_isEmpty = m_items.isEmpty();

  //    delete m_activeItem;
  //    m_activeItem = CurrentItem();

  //    updateIndices();
  //    emitGcCurrentItemChanged(m_activeItem, 0);
  //    m_itemBeingManipulated = false;
  //  }
}

/*----------------------------------------------------------------------------*/

void DomTreeView::stepUp() {
  //  if (m_activeItem) {
  //    m_itemBeingManipulated = true;

  //    TreeWidgetItem *parentItem = m_activeItem->Parent();

  //    if (parentItem) {
  //      TreeWidgetItem *grandParent = parentItem->Parent();

  //      if (grandParent) {
  //        parentItem->removeChild(m_activeItem);
  //        grandParent->insertChild(grandParent->indexOfChild(parentItem),
  //                                 m_activeItem);
  //        grandParent->element().insertBefore(m_activeItem->element(),
  //                                            parentItem->element());

  //        /* Update the database to reflect the re-parenting. */
  //        //        DB::instance()->updateElementChildren(
  //        //            grandParent->name(),
  // QStringList(m_activeItem->name()));
  //      }

  //      updateIndices();
  //      emitGcCurrentItemChanged(m_activeItem, 0);
  //    }

  //    m_itemBeingManipulated = false;
  //  }
}

/*----------------------------------------------------------------------------*/

void DomTreeView::stepDown() {
  //  if (m_activeItem) {
  //    m_itemBeingManipulated = true;

  //    TreeWidgetItem *parentItem = m_activeItem->Parent();
  //    TreeWidgetItem *siblingItem =
  //        ItemFromNode(m_activeItem->element().previousSiblingElement());

  //    /* Try again in the opposite direction. */
  //    if (!siblingItem) {
  //      siblingItem =
  // ItemFromNode(m_activeItem->element().nextSiblingElement());
  //    }

  //    if (siblingItem && parentItem) {
  //      parentItem->removeChild(m_activeItem);
  //      siblingItem->insertChild(0, m_activeItem);
  //      siblingItem->element().insertBefore(m_activeItem->element(),
  //                                          siblingItem->element().firstChild());

  //      /* Update the database to reflect the re-parenting. */
  //      //      DB::instance()->updateElementChildren(
  //      //          siblingItem->name(), QStringList(m_activeItem->name()));

  //      updateIndices();
  //      emitGcCurrentItemChanged(m_activeItem, 0);
  //    }

  //    m_itemBeingManipulated = false;
  //  }
}

/*----------------------------------------------------------------------------*/

void DomTreeView::expandSelection() {
  for (const auto &index : selectedIndexes()) {
    expand(index);
  }
}

/*----------------------------------------------------------------------------*/

void DomTreeView::collapseSelection() {
  for (const auto &index : selectedIndexes()) {
    collapse(index);
  }
}

/*----------------------------------------------------------------------------*/
