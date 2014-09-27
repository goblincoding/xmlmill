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
#ifndef DOMITEM_H
#define DOMITEM_H

#include <QDomNode>
#include <QHash>
#include <QVariant>

//----------------------------------------------------------------------

class DomItem {
public:
  DomItem(QDomNode &node, int row, DomItem *parent = 0);
  ~DomItem();

  DomItem *child(int i) const;
  DomItem *parent() const;

  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value);

  void fetchMore();
  bool canFetchMore() const;

  int row() const;
  int childCount() const;

  int firstRowToInsert() const;
  int lastRowToInsert() const;

  bool hasChildren() const;

private:
  QString toString() const;
  QString elementString() const;
  QString commentString() const;

private:
  QDomNode m_domNode;
  int m_rowNumber;
  bool m_finishedLoading;

  DomItem *m_parent;
  QList<DomItem *> m_children;
};

#endif // DOMITEM_H
