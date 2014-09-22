#include "domitem.h"

#include <QDomNode>
#include <QModelIndex>

//----------------------------------------------------------------------

DomItem::DomItem(QDomNode &node, int row, DomItem *parent)
    : m_domNode(node), m_rowNumber(row), m_parentItem(parent), m_childItems() {}

//----------------------------------------------------------------------

DomItem::~DomItem() { qDeleteAll(m_childItems); }

//----------------------------------------------------------------------

QVariant DomItem::data(const QModelIndex &index, int role) const {
  QStringList attributes;
  QDomNamedNodeMap attributeMap = m_domNode.attributes();

  switch (index.column()) {
  case 0:
    if (role == Qt::DisplayRole) {
      return m_rowNumber;
    }

    break;
  case 1:
    if (role == Qt::DisplayRole) {
      return toString();
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------

bool DomItem::setData(const QModelIndex &index, const QVariant &value) {
  switch (index.column()) {
  case 0:
    if (m_domNode.isElement()) {
      m_domNode.toElement().setTagName(value.toString());
    }
    return true;
  case 1:
    // nothing to do here for now, just testing the concepts
    return true;
  case 2:
    // nothing to do here for now, just testing the concepts
    return true;
  }

  return false;
}

//----------------------------------------------------------------------

QDomNode DomItem::node() const { return m_domNode; }

//----------------------------------------------------------------------

DomItem *DomItem::parent() { return m_parentItem; }

//----------------------------------------------------------------------

DomItem *DomItem::child(int i) {
  if (m_childItems.contains(i))
    return m_childItems[i];

  if (i >= 0 && i < m_domNode.childNodes().count()) {
    QDomNode childNode = m_domNode.childNodes().item(i);
    DomItem *childItem = new DomItem(childNode, i, this);
    m_childItems[i] = childItem;
    return childItem;
  }
  return nullptr;
}

//----------------------------------------------------------------------

int DomItem::row() { return m_rowNumber; }

//----------------------------------------------------------------------

QString DomItem::toString() const {
  if (m_domNode.isElement()) {
    return elementString();
  } else if (m_domNode.isComment()) {
    return m_domNode.nodeValue();
  }

  return QString();
}

//----------------------------------------------------------------------

QString DomItem::elementString() const {
  QString text("<");
  text += m_element.tagName();

  QDomNamedNodeMap attributes = m_element.attributes();

  /* For elements with no attributes (e.g. <element/>). */
  if (attributes.isEmpty() && m_element.childNodes().isEmpty()) {
    text += "/>";
    return text;
  }

  if (!attributes.isEmpty()) {
    for (int i = 0; i < attributes.size(); ++i) {
      QDomNode attribute = attributeMap.item(i);
      text += " ";
      text += attribute.nodeName();
      text += "=\"";
      text += attribute.nodeValue();
      text += "\"";
    }

    /* For elements without children but with attributes. */
    if (m_element.firstChild().isNull()) {
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
