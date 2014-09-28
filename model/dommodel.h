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
#ifndef DOMMODEL_H
#define DOMMODEL_H

#include <QAbstractItemModel>
#include <QDomDocument>
#include <QModelIndex>

#include <memory>

//----------------------------------------------------------------------

class DomItem;

//----------------------------------------------------------------------

class DomModel : public QAbstractItemModel {
  Q_OBJECT

public:
  explicit DomModel(QObject *parent = 0);
  virtual ~DomModel();

  void setDomDocument(QDomDocument document);

  // QAbstractItemModel interface
public:
  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const;

  virtual QVariant data(const QModelIndex &index, int role) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value,
                       int role);

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QModelIndex index(int row, int columnNumber,
                            const QModelIndex &parent = QModelIndex()) const;

  virtual QModelIndex parent(const QModelIndex &child) const;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
  /*! Returns the relevant item if index is valid, if invalid, we return the
   * root item by default (note that this item could be NULL if the model is
   * empty). */
  DomItem *itemFromIndex(const QModelIndex &index) const;

private:
  QDomDocument m_domDocument;
  std::unique_ptr<DomItem> m_rootItem;
};

#endif // DOMMODEL_H
