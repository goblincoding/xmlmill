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
#include <QVariant>

//----------------------------------------------------------------------

/*! A wrapper class for QDomNodes.  Each DomItem wraps a QDomNode in the DOM
 * document represented by a DomModel. Note: QDomNodes are implicitly shared, so
 * all modifications to a DomItem's QDomNodes will affect all copies.*/
class DomItem {
public:
  /*! Represents the columns in the model.  We'll probably only ever have one,
   * but in case the use case changes, the enumeration should make extension
   * easier. */
  enum class Column { Xml = 0, ColumnCount };

  /*! Convenience method returning the integer corresponding to "column" */
  static constexpr int columnNumber(Column column) {
    return static_cast<int>(column);
  }

  /*! Constructor.
   * @param node - the (implicitly shared) QDomNode corresponding to this item.
   * @param row - this item's row number (relative to its parent).
   * @param parent - this item's parent (if any). */
  DomItem(QDomNode node, int row, DomItem *parent = nullptr);

  /*! Destructor. */
  ~DomItem();

  /*! Returns this item's child at "row" or a nullptr if no such child exists.
   */
  DomItem *child(int row) const;

  /*! Returns this item's parent or a nullptr if no parent exists. */
  DomItem *parent() const;

  /*! Returns the data corresponding to "index" and "role". */
  QVariant data(const QModelIndex &index, int role) const;

  /*! Updates the item data (e.g. the underlying QDomNode) related to "index"
   * and "role" to "value". */
  bool setData(const QModelIndex &index, const QVariant &value, int role);

  /*! Returns "true" if this item has any children. */
  bool hasChildren() const;

  /*! Returns this item's number of children. */
  int childCount() const;

  /*! Returns this item's row number relative to its parent item. */
  int row() const;

private:
  void updateStringRepresentation();

private:
  QDomNode m_domNode;
  int m_rowNumber;

  DomItem *m_parent;
  QString m_stringRepresentation;
  QList<QDomNode> m_childNodes;
  QList<DomItem *> m_childItems;
};

#endif // DOMITEM_H
