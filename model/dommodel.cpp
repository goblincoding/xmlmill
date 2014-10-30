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

using Column = DomItem::Column;

//----------------------------------------------------------------------

DomModel::DomModel(QObject *parent)
    : QAbstractItemModel(parent), m_domDocument(), m_rootItem() {}

//----------------------------------------------------------------------

DomModel::~DomModel() {}

//----------------------------------------------------------------------

void DomModel::setDomDocument(QDomDocument document) {
  beginResetModel();
  m_domDocument = document;
  m_rootItem =
      std::move(std::unique_ptr<DomItem>(new DomItem(m_domDocument, 0)));
  endResetModel();
}

/*----------------------------------------------------------------------------*/

void DomModel::dataChangedExternally(const QModelIndex &index) {
  DomItem *item = itemFromIndex(index);

  if (item) {
    item->updateState();
    emit dataChanged(index, index);
  }
}

//----------------------------------------------------------------------

QVariant DomModel::data(const QModelIndex &index, int role) const {
  DomItem *item = itemFromIndex(index);
  return item ? item->data(index, role) : QVariant();
}

//----------------------------------------------------------------------

bool DomModel::setData(const QModelIndex &index, const QVariant &value,
                       int role) {
  DomItem *item = itemFromIndex(index);
  bool result = item ? item->setData(index, value, role) : false;

  if (result) {
    emit dataChanged(index, index);
  }

  return result;
}

//----------------------------------------------------------------------

QVariant DomModel::headerData(int section, Qt::Orientation orientation,
                              int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
    case DomItem::columnNumber(Column::Xml):
      return tr("DOM Node");
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------

Qt::ItemFlags DomModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return 0;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
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
  DomItem *childItem = itemFromIndex(child);
  DomItem *parentItem = childItem ? childItem->parent() : nullptr;

  if (!parentItem || parentItem == m_rootItem.get()) {
    return QModelIndex();
  }

  return createIndex(parentItem->row(), DomItem::columnNumber(Column::Xml),
                     parentItem);
}

//----------------------------------------------------------------------

bool DomModel::hasChildren(const QModelIndex &parent) const {
  DomItem *parentItem = itemFromIndex(parent);
  return parentItem ? parentItem->hasChildren() : false;
}

//----------------------------------------------------------------------

int DomModel::rowCount(const QModelIndex &parent) const {
  DomItem *parentItem = itemFromIndex(parent);
  return parentItem ? parentItem->childCount() : 0;
}

//----------------------------------------------------------------------

int DomModel::columnCount(const QModelIndex & /*parent*/) const {
  return DomItem::columnNumber(Column::ColumnCount);
}

//----------------------------------------------------------------------

DomItem *DomModel::itemFromIndex(const QModelIndex &index) const {
  if (index.isValid()) {
    DomItem *item = static_cast<DomItem *>(index.internalPointer());
    return item;
  }

  return m_rootItem.get();
}

//----------------------------------------------------------------------
