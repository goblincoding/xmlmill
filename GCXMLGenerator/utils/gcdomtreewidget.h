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

class GCDOMTreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  GCDOMTreeWidget( QWidget *parent = 0 );
  ~GCDOMTreeWidget();

  GCTreeWidgetItem* currentGCItem();

  /*! This function starts the recursive process of populating the tree widget with items
      consisting of the element hierarchy starting at "baseElementName". If "baseElementName"
      is empty, a complete hierarchy of the current active profile will be constructed. This
      method also automatically clears and resets GCDOMTreeWidget's state and expands the
      entire tree.
      \sa processNextElement */
  void constructElementHierarchy( const QString &baseElementName = QString() );

  /*! Adds a new item and corresponding DOM element node named "element". If the tree
      is empty, the new item will be added to the invisible root, otherwise it will be
      added as a child of the current item. */
  void addItem( const QString &element );

  /*! Adds a new item and inserts it into position "index" of the current item. */
  void insertItem( const QString &element, int index );

  void clearAndReset();

signals:
  void gcItemClicked( GCTreeWidgetItem*,int );

private slots:
  void emitGCItemClicked( QTreeWidgetItem* item, int column );

private:
  /*! Processes individual elements.  This function is called recursively from within
      "constructElementHierarchy", creating a representative tree widget item for the
      element and adding it (the item) to the correct parent.
      @param element - the name of the element for which a tree widget item must be created.
      \sa constructElementHierarchy */
  void processNextElement( const QString &element );

  QDomDocument *m_domDoc;
  bool          m_isEmpty;
};

#endif // GCDOMTREEWIDGET_H
