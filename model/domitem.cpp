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
#include "utils/domnodeparser.h"

#include <QDomNode>
#include <QModelIndex>
#include <QApplication>

//----------------------------------------------------------------------

DomItem::DomItem(QDomNode node, int row, DomItem *parent)
    : m_domNode(node), m_rowNumber(row), m_parent(parent),
      m_stringRepresentation(), m_childItems() {
  updateStringRepresentation();
  qApp->processEvents();

  for (int i = 0; i < m_domNode.childNodes().count(); ++i) {
    QDomNode childNode = m_domNode.childNodes().at(i);
    m_childNodes.append(childNode);
    m_childItems << new DomItem(childNode, i, this);
  }
}

//----------------------------------------------------------------------

DomItem::~DomItem() { qDeleteAll(m_childItems); }

//----------------------------------------------------------------------

QVariant DomItem::data(const QModelIndex &index, int role) const {
  switch (index.column()) {
  case columnNumber(Column::Xml):
    if (role == Qt::DisplayRole) {
      return m_stringRepresentation;
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------

const QList<DomItem *> &DomItem::childItems() const { return m_childItems; }

//----------------------------------------------------------------------

bool DomItem::setData(const QModelIndex &index, const QVariant &value,
                      int role) {
  switch (index.column()) {
  case columnNumber(Column::Xml):
    if (m_domNode.isElement()) {
      m_domNode.toElement().setTagName(value.toString());
    }

    updateStringRepresentation();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------

DomItem *DomItem::parent() const { return m_parent; }

//----------------------------------------------------------------------

QDomNode DomItem::node() const { return m_domNode; }

//----------------------------------------------------------------------

DomItem *DomItem::child(int row) const {
  return m_childItems.value(row, nullptr);
}

//----------------------------------------------------------------------

int DomItem::row() const { return m_rowNumber; }

//----------------------------------------------------------------------

void DomItem::updateStringRepresentation() {
  DomNodeParser parser;
  m_stringRepresentation = parser.toString(m_domNode);
}

//----------------------------------------------------------------------

int DomItem::childCount() const { return m_childNodes.size(); }

//----------------------------------------------------------------------

bool DomItem::hasChildren() const { return !m_childNodes.empty(); }

//----------------------------------------------------------------------
