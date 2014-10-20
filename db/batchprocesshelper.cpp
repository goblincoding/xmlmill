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

#include "batchprocesshelper.h"

#include <QDomDocument>
#include <QSqlQuery>
#include <assert.h>

/*----------------------------------------------------------------------------*/

BatchProcessHelper::BatchProcessHelper(const QDomDocument &domDoc)
    : m_domDoc(domDoc), m_rootName(""), m_attributeValues(), m_attributes(),
      m_elements(), m_parents(), m_root(), m_records() {
  assert(!m_domDoc.isNull() && "BatchProcessHelper: DOM Doc is NULL");

  QDomElement root = m_domDoc.documentElement();
  m_rootName = root.tagName();
  traverseDocument(root);
  createVariantLists();
}

/*----------------------------------------------------------------------------*/

void BatchProcessHelper::bindValues(QSqlQuery &query) const {
  query.addBindValue(m_attributeValues);
  query.addBindValue(m_attributes);
  query.addBindValue(m_elements);
  query.addBindValue(m_parents);
  query.addBindValue(m_root);
}

/*----------------------------------------------------------------------------*/

/* Inline helper function. */
QList<QDomElement> children(const QDomElement &element) {
  QList<QDomElement> list;
  QDomElement child = element.firstChildElement();

  while (!child.isNull()) {
    list.append(child);
    child = child.nextSiblingElement();
  }

  return list;
}

/*----------------------------------------------------------------------------*/

void BatchProcessHelper::traverseDocument(const QDomElement &parentElement) {
  assert(!parentElement.isNull() &&
         "BatchProcessHelper: parent element is NULL");

  QList<QDomElement> toVisit = children(parentElement);

  while (!toVisit.isEmpty()) {
    QDomElement element = toVisit.takeFirst();
    processElement(element);
    toVisit << children(element);
  }
}

/*----------------------------------------------------------------------------*/

void BatchProcessHelper::processElement(const QDomElement &element) {
  assert(!element.isNull() &&
         "BatchProcessHelper: attempting to process a NULL element");

  QVariant elementName = element.tagName();
  QVariant parentName = element.parentNode().toElement().tagName();
  QDomNamedNodeMap attributeNodes = element.attributes();

  if (attributeNodes.isEmpty()) {
    XmlRecord record;
    /* Value and attribute fields are already defaulted to "" */
    record.m_element = elementName;
    record.m_parent = parentName;
    record.m_root = m_rootName;
    m_records.append(record);
  } else {
    for (int i = 0; i < attributeNodes.size(); ++i) {
      QDomAttr attribute = attributeNodes.item(i).toAttr();

      if (!attribute.isNull()) {
        XmlRecord record;
        record.m_value = attribute.value();
        record.m_attribute = attribute.name();
        record.m_element = elementName;
        record.m_parent = parentName;
        record.m_root = m_rootName;
        m_records.append(record);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void BatchProcessHelper::createVariantLists() {
  for (const XmlRecord &record : m_records) {
    m_attributeValues << record.m_value;
    m_attributes << record.m_attribute;
    m_elements << record.m_element;
    m_parents << record.m_parent;
    m_root << record.m_root;
  }
}

/*----------------------------------------------------------------------------*/
