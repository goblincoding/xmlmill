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
#include "domitem.h"
#include "dommodel.h"

#include <QDomDocument>

//----------------------------------------------------------------------

DomModel::DomModel(QDomDocument document, QObject *parent)
    : QAbstractItemModel(parent), m_domDocument(document) {
  m_rootItem = new DomItem(m_domDocument, 0);
}

//----------------------------------------------------------------------

DomModel::~DomModel() { delete m_rootItem; }

//----------------------------------------------------------------------

int DomModel::columnCount(const QModelIndex & /*parent*/) const {
  int column = columnNumber(ModelColumns::ColumnCount);
  return columnNumber(ModelColumns::ColumnCount);
}

//----------------------------------------------------------------------

int DomModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > columnNumber(ModelColumns::Xml)) {
    return 0;
  }

  DomItem *parentItem = itemFromIndex(parent);
  return parentItem ? parentItem->childCount() : 0;
}

//----------------------------------------------------------------------

bool DomModel::hasChildren(const QModelIndex &parent) const {
  DomItem *parentItem = itemFromIndex(parent);
  return parentItem ? parentItem->hasChildren() : false;
}

//----------------------------------------------------------------------

bool DomModel::canFetchMore(const QModelIndex &parent) const {
  if (parent.isValid()) {
    DomItem *parentItem = itemFromIndex(parent);

    if (parentItem) {
      return parentItem->hasChildren() && !parentItem->hasFetchedChildren();
    }
  }

  return false;
}

//----------------------------------------------------------------------

void DomModel::fetchMore(const QModelIndex &parent) {
  if (canFetchMore(parent)) {
    DomItem *parentItem = itemFromIndex(parent);

    if (parentItem) {
      int childCount = parentItem->childCount();
      int childrenFetched = parentItem->childrenFetched();

      int remainder = childCount - childrenFetched;
      int childrenToFetch = qMin(100, remainder);
      beginInsertRows(parent, childrenFetched,
                      childrenFetched + childrenToFetch - 1);
      endInsertRows();
    }
  }
}

//----------------------------------------------------------------------

DomItem *DomModel::itemFromIndex(const QModelIndex &index) const {
  if (index.isValid()) {
    DomItem *item = static_cast<DomItem *>(index.internalPointer());
    return item;
  }

  return m_rootItem;
}

//----------------------------------------------------------------------

constexpr int DomModel::columnNumber(DomModel::ModelColumns col) {
  return static_cast<int>(col);
}

//----------------------------------------------------------------------

bool DomModel::setData(const QModelIndex &index, const QVariant &value,
                       int role) {
  if (index.isValid() && role == Qt::EditRole) {
    DomItem *item = itemFromIndex(index);
    bool result = item ? item->setData(index, value) : false;

    if (result) {
      emit dataChanged(index, index);
    }

    return result;
  }

  return false;
}

//----------------------------------------------------------------------

QVariant DomModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  DomItem *item = itemFromIndex(index);
  return item ? item->data(index, role) : QVariant();
}

//----------------------------------------------------------------------

Qt::ItemFlags DomModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return 0;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

//----------------------------------------------------------------------

QVariant DomModel::headerData(int section, Qt::Orientation orientation,
                              int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
    case columnNumber(ModelColumns::Xml) :
      return tr("Node");
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------

QModelIndex DomModel::index(int row, int column,
                            const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  DomItem *parentItem = itemFromIndex(parent);
  DomItem *childItem = parentItem ? parentItem->child(row) : nullptr;

  if (childItem) {
    return createIndex(row, column, childItem);
  }

  return QModelIndex();
}

//----------------------------------------------------------------------

QModelIndex DomModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  DomItem *childItem = itemFromIndex(child);
  DomItem *parentItem = childItem ? childItem->parent() : nullptr;

  if (!parentItem || parentItem == m_rootItem) {
    return QModelIndex();
  }

  return createIndex(parentItem->row(), columnNumber(ModelColumns::Xml),
                     parentItem);
}

//----------------------------------------------------------------------
