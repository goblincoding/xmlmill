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
    : QAbstractItemModel(parent), domDocument(document) {
  rootItem = new DomItem(domDocument, 0);
}

//----------------------------------------------------------------------

DomModel::~DomModel() { delete rootItem; }

//----------------------------------------------------------------------

int DomModel::columnCount(const QModelIndex & /*parent*/) const { return 1; }

//----------------------------------------------------------------------

DomItem *DomModel::domItem(const QModelIndex &index) {
  if (index.isValid()) {
    DomItem *item = static_cast<DomItem *>(index.internalPointer());
    if (item) {
      return item;
    }
  }
  return rootItem;
}

//----------------------------------------------------------------------

bool DomModel::setData(const QModelIndex &index, const QVariant &value,
                       int role) {
  if (index.isValid() && role == Qt::EditRole) {
    DomItem *item = domItem(index);
    bool result = item->setData(index, value);

    if (result) {
      emit dataChanged(index, index);
    }

    return result;
  }
  return false;
}

//----------------------------------------------------------------------

QVariant DomModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  DomItem *item = static_cast<DomItem *>(index.internalPointer());
  return item->data(index, role);
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
    case 0:
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

  DomItem *parentItem;

  if (!parent.isValid()) {
    parentItem = rootItem;
  } else {
    parentItem = static_cast<DomItem *>(parent.internalPointer());
  }

  DomItem *childItem = parentItem->child(row);
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

  DomItem *childItem = static_cast<DomItem *>(child.internalPointer());
  DomItem *parentItem = childItem->parent();

  if (!parentItem || parentItem == rootItem) {
    return QModelIndex();
  }

  return createIndex(parentItem->row(), 0, parentItem);
}

//----------------------------------------------------------------------

int DomModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0) {
    return 0;
  }

  DomItem *parentItem;

  if (!parent.isValid()) {
    parentItem = rootItem;
  } else {
    parentItem = static_cast<DomItem *>(parent.internalPointer());
  }

  return parentItem->node().childNodes().count();
}

//----------------------------------------------------------------------
