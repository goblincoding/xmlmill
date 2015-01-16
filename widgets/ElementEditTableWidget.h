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
#ifndef DOMEDITWIDGET_H
#define DOMEDITWIDGET_H

#include <QTableWidget>
#include <memory>

//----------------------------------------------------------------------

class QPersistentModelIndex;
class QModelIndex;
class DomItem;
class ElementEditWidget;

//----------------------------------------------------------------------

class ElementEditTableWidget : public QTableWidget {
  Q_OBJECT
public:
  explicit ElementEditTableWidget(QWidget *parent = 0);

signals:
  void treeIndexDataChanged(const QModelIndex &);

public slots:
  void treeIndexSelected(const QModelIndex &index);
  void contentsChanged();

private slots:
  void processChildItems();

private:
  void processItem(DomItem *item, bool primary);
  void setupTable();
  void resetState();
  void createAddMoreButton();
  void removeAddMoreButton();  

private:
  using PersistentPtr = std::unique_ptr<QPersistentModelIndex>;
  PersistentPtr m_persistentTreeIndex;
  QList<ElementEditWidget *> m_nodeEdits;
  DomItem *m_currentItem;
  int m_childrenProcessed;
};

#endif // DOMEDITWIDGET_H
