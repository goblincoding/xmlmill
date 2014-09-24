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
