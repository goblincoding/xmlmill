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

#include <QDomNode>
#include <QModelIndex>

//----------------------------------------------------------------------

DomItem::DomItem(QDomNode &node, int row, DomItem *parent)
    : m_domNode(node), m_rowNumber(row), m_finishedLoading(false),
      m_parent(parent), m_stringRepresentation(), m_childItems() {
  m_stringRepresentation = toString();

  for( int i = 0; i < m_domNode.childNodes().count(); ++i) {
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
  case columnNumber(Column::Xml) :
    if (role == Qt::DisplayRole) {
      return m_stringRepresentation;
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------

bool DomItem::setData(const QModelIndex &index, const QVariant &value) {
  switch (index.column()) {
  case columnNumber(Column::Xml) :
    if (m_domNode.isElement()) {
      m_domNode.toElement().setTagName(value.toString());
    }
    return true;
  }

  return false;
}

//----------------------------------------------------------------------

void DomItem::fetchMore() {
  for (int i = firstRowToInsert(); i < lastRowToInsert(); ++i) {
    QDomNode childNode = m_childNodes.at(i);
    m_childItems << new DomItem(childNode, i, this);
  }
}

//----------------------------------------------------------------------

bool DomItem::canFetchMore() const { return m_childItems.size() < childCount(); }

//----------------------------------------------------------------------

DomItem *DomItem::parent() const { return m_parent; }

//----------------------------------------------------------------------

DomItem *DomItem::child(int i) const { return m_childItems.value(i, nullptr); }

//----------------------------------------------------------------------

int DomItem::row() const { return m_rowNumber; }

//----------------------------------------------------------------------

int DomItem::childCount() const { return m_childNodes.size(); }

//----------------------------------------------------------------------

int DomItem::firstRowToInsert() const { return m_childItems.size(); }

//----------------------------------------------------------------------

int DomItem::lastRowToInsert() const {
  int firstRow = firstRowToInsert();
  int remainder = childCount() - firstRow;
  int childrenToFetch = qMin(100, remainder);
  return firstRow + childrenToFetch;
}

//----------------------------------------------------------------------

bool DomItem::hasChildren() const { return childCount() > 0; }

//----------------------------------------------------------------------

QString DomItem::toString() const {
  if (m_domNode.isElement()) {
    return elementString();
  } else if (m_domNode.isComment()) {
    return commentString();
  } else if (m_domNode.isProcessingInstruction()) {
    return "FIND THIS STRING AND FIX IT!";
  }

  return m_domNode.nodeValue();
}

//----------------------------------------------------------------------

QString DomItem::elementString() const {
  QString text("<");
  text += m_domNode.nodeName();

  QDomNamedNodeMap attributeMap = m_domNode.attributes();

  /* For elements with no attributes (e.g. <element/>). */
  if (attributeMap.isEmpty() && m_domNode.childNodes().isEmpty()) {
    text += "/>";
    return text;
  }

  if (!attributeMap.isEmpty()) {
    for (int i = 0; i < attributeMap.size(); ++i) {
      QDomNode attribute = attributeMap.item(i);
      text += " ";
      text += attribute.nodeName();
      text += "=\"";
      text += attribute.nodeValue();
      text += "\"";
    }

    /* For elements without children but with attributes. */
    if (m_domNode.firstChild().isNull()) {
      text += "/>";
    } else {
      /* For elements with children and attributes. */
      text += ">";
    }
  } else {
    text += ">";
  }

  return text;
}

//----------------------------------------------------------------------

QString DomItem::commentString() const {
  QString text("<!-- ");
  text += m_domNode.nodeValue();
  text += " -->";
  return text;
}

//----------------------------------------------------------------------
