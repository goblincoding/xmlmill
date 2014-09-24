#ifndef DOMTREEVIEW_H
#define DOMTREEVIEW_H

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
#include <QTreeView>

class DomTreeView : public QTreeView
{
  Q_OBJECT
public:
  explicit DomTreeView(QWidget *parent = 0);

private slots:
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
  void expandSelection();

  /*! Connected to a context menu action.  Collapses the active (selected) item.
      \sa expand */
  void collapseSelection();


};

#endif // DOMTREEVIEW_H
