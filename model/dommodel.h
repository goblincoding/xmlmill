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

/*! A model representation of a DOM document. */
class DomModel : public QAbstractItemModel {
  Q_OBJECT

public:
  /*! Constructor. */
  explicit DomModel(QObject *parent = 0);

  /*! Destructor. */
  virtual ~DomModel();

  /*! Implicit sharing of QDomNodes means that all modifications to the set
   * QDomDocument made by the model will be shared by the original document
   * (wherever it may live). Calling setDocument resets the underlying DOM
   * document */
  void setDomDocument(QDomDocument document);

public:
  /*! Returns the data corresponding to "index" and "role" */
  virtual QVariant data(const QModelIndex &index, int role) const override;

  /*! Sets the data corresponding to "index" and "role" to "value" */
  virtual bool setData(const QModelIndex &index, const QVariant &value,
                       int role) override;

  /*! Returns the header data corresponding to "section" and "role". Since
   * DomModel is intended for a hierarchical tree view, only horizontal
   * orientations are catered for. */
  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;

  /*! Returns the flags corresponding to "index" */
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

  /*! Returns the index corresponding to "row" and "columnNumber" relative to
   * "parent" */
  virtual QModelIndex
  index(int row, int columnNumber,
        const QModelIndex &parent = QModelIndex()) const override;

  /*! Returns the index corresponding to "child"'s parent */
  virtual QModelIndex parent(const QModelIndex &child) const override;

  /*! Returns "true" if "parent" has children. */
  virtual bool hasChildren(const QModelIndex &parent) const override;

  /*! Returns the number of rows under "parent" (direct child count) */
  virtual int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;

  /*! Returns the number of columns corresponding to "parent" */
  virtual int
  columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
  /*! Returns the underlying DomItem if "index" is valid, if invalid, we return
   * the root item by default (note that this item could be NULL if the model is
   * empty). */
  DomItem *itemFromIndex(const QModelIndex &index) const;

private:
  QDomDocument m_domDocument;
  std::unique_ptr<DomItem> m_rootItem;
};

#endif // DOMMODEL_H
