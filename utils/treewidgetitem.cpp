/* Copyright (c) 2012 - 2013 by William Hallatt.
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
 *details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */
#include "treewidgetitem.h"
#include "globalsettings.h"

/*----------------------------------------------------------------------------*/

TreeWidgetItem::TreeWidgetItem(QDomElement element) { init(element, -1); }

/*----------------------------------------------------------------------------*/

TreeWidgetItem::TreeWidgetItem(QDomElement element, int index) {
  init(element, index);
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::init(QDomElement element, int index) {
  m_element = element;
  m_elementExcluded = false;
  m_index = index;
  m_verbose = GlobalSettings::showTreeItemsVerbose();

  QDomNamedNodeMap attributes = m_element.attributes();

  for (int i = 0; i < attributes.size(); ++i) {
    m_includedAttributes.append(attributes.item(i).toAttr().name());
  }

  setDisplayText();
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::setDisplayText() {
  if (m_verbose) {
    setText(0, toString());
    setToolTip(0, "");
  } else {
    setText(0, m_element.tagName());
    setToolTip(0, toString());
  }
}

/*----------------------------------------------------------------------------*/

TreeWidgetItem *TreeWidgetItem::Parent() const {
  return dynamic_cast<TreeWidgetItem *>(parent());
}

/*----------------------------------------------------------------------------*/

TreeWidgetItem *TreeWidgetItem::Child(int index) const {
  return dynamic_cast<TreeWidgetItem *>(child(index));
}

/*----------------------------------------------------------------------------*/

QDomElement TreeWidgetItem::element() const { return m_element; }

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::setExcludeElement(bool exclude) {
  if (Parent()) {
    if (exclude) {
      Parent()->element().removeChild(m_element);
    } else {
      Parent()->element().appendChild(m_element);
    }
  }

  m_elementExcluded = exclude;
}

/*----------------------------------------------------------------------------*/

bool TreeWidgetItem::elementExcluded() const { return m_elementExcluded; }

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::excludeAttribute(const QString &attribute) {
  m_element.removeAttribute(attribute);
  m_includedAttributes.removeAll(attribute);
  m_includedAttributes.sort();
  setDisplayText();
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::includeAttribute(const QString &attribute,
                                      const QString &value) {
  m_element.setAttribute(attribute, value);
  m_includedAttributes.append(attribute);
  m_includedAttributes.removeDuplicates();
  m_includedAttributes.sort();
  setDisplayText();
}

/*----------------------------------------------------------------------------*/

bool TreeWidgetItem::attributeIncluded(const QString &attribute) const {
  return m_includedAttributes.contains(attribute);
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::setIncrementAttribute(const QString &attribute,
                                           bool increment) {
  if (increment) {
    m_incrementedAttributes.append(attribute);
  } else {
    m_incrementedAttributes.removeAll(attribute);
  }

  m_incrementedAttributes.sort();
}

/*----------------------------------------------------------------------------*/

bool TreeWidgetItem::incrementAttribute(const QString &attribute) const {
  return m_incrementedAttributes.contains(attribute);
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::fixAttributeValues() {
  m_fixedValues.clear();

  QDomNamedNodeMap attributes = m_element.attributes();

  for (int i = 0; i < attributes.size(); ++i) {
    QDomAttr attribute = attributes.item(i).toAttr();
    m_fixedValues.insert(attribute.name(), attribute.value());
  }
}

/*----------------------------------------------------------------------------*/

QString TreeWidgetItem::fixedValue(const QString &attribute) const {
  return m_fixedValues.value(attribute, QString());
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::revertToFixedValues() {
  foreach(QString attribute, m_fixedValues.keys()) {
    m_element.setAttribute(attribute, m_fixedValues.value(attribute));
  }
}

/*----------------------------------------------------------------------------*/

QString TreeWidgetItem::toString() const {
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
      text += " ";

      QString attribute = attributes.item(i).toAttr().name();
      text += attribute;
      text += "=\"";

      QString attributeValue = attributes.item(i).toAttr().value();
      text += attributeValue;
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

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::setIndex(int index) { m_index = index; }

/*----------------------------------------------------------------------------*/

int TreeWidgetItem::index() const { return m_index; }

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::rename(const QString &newName) {
  m_element.setTagName(newName);
  setDisplayText();
}

/*----------------------------------------------------------------------------*/

QString TreeWidgetItem::name() const {
  if (!m_element.isNull()) {
    return m_element.tagName();
  }

  return QString("");
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::setVerbose(bool verbose) {
  m_verbose = verbose;
  setDisplayText();
}

/*----------------------------------------------------------------------------*/

void TreeWidgetItem::insertGcChild(int index, TreeWidgetItem *item) {
  QTreeWidgetItem::insertChild(index + 1, item);

  QDomElement siblingElement = m_element.firstChildElement();
  int counter = 0;

  while (!siblingElement.isNull()) {
    if (counter == index) {
      break;
    }

    counter++;
    siblingElement = siblingElement.nextSiblingElement();
  }

  m_element.insertAfter(item->element(), siblingElement);
}

/*----------------------------------------------------------------------------*/
