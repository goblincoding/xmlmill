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

#ifndef BATCHPROCESSHELPER_H
#define BATCHPROCESSHELPER_H

#include <QVariantList>

class QDomDocument;
class QDomElement;
class QSqlQuery;

/** Helper class assisting with batch updates to the database.  This class
 * simply traverses an XML DOM document, extracting database record information
 * from it in the process to be made available to the DB interface for whole
 * document batch updates to the XML table. */
class BatchProcessHelper {
public:
  /*! Constructor @param domDoc - the DOM document from which all information
   * will be extracted. */
  BatchProcessHelper(const QDomDocument &domDoc);

  /*! Binds the batch values to 'query' (which must have been prepared
   * before hand).*/
  void bindValues(QSqlQuery &query) const;

private:
  /*! Walk the DOM tree.*/
  void traverseDocument(const QDomElement &parentElement);

  /*! Processes an element, creating one or more (depending on whether or not
   * the element in question has associated attirbutes) XmlRecords and inserting
   * these records into the records list. */
  void processElement(const QDomElement &element);

  /*! Creates the QVariantLists representing the new records (the batch) from
   * the records list. */
  void createVariantLists();

private:
  const QDomDocument &m_domDoc;
  QVariant m_rootName;

  QVariantList m_attributeValues;
  QVariantList m_attributes;
  QVariantList m_elements;
  QVariantList m_parents;
  QVariantList m_root;

  struct XmlRecord {
    QVariant m_value;
    QVariant m_attribute;
    QVariant m_element;
    QVariant m_parent;
    QVariant m_root;

    XmlRecord()
        : m_value(""), m_attribute(""), m_element(""), m_parent(""),
          m_root("") {}
  };

  QList<XmlRecord> m_records;
};

#endif // BATCHPROCESSHELPER_H
