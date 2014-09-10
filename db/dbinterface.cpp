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

#include "dbinterface.h"
#include "batchprocesshelper.h"
#include "../utils/globalsettings.h"

#include <QDomDocument>
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>

#include <assert.h>

/*----------------------------------------------------------------------------*/

/* The second '?' represents our string SEPARATOR. And the IFNULL checks are to
 * ensure that we correctly apply the update to potentially empty lists. */
static const QLatin1String UPDATE_CHILDREN("UPDATE xml "
                                           "SET children = ( IFNULL( ?, \"\" ) "
                                           "|| IFNULL( ?, \"\" ) "
                                           "|| IFNULL( children, \"\" ) ) "
                                           "WHERE element = ? "
                                           "AND associatedRoot = ?");

static const QLatin1String
UPDATE_ATTRIBUTES("UPDATE xml "
                  "SET attributes = ( IFNULL( ?, \"\" ) "
                  "|| IFNULL( ?, \"\" ) "
                  "|| IFNULL( attributes, \"\" )  ) "
                  "WHERE element = ? "
                  "AND associatedRoot = ?");

static const QLatin1String INSERT_ATTRIBUTEVALUES("INSERT INTO xmlattributes( "
                                                  "attribute, "
                                                  "associatedElement, "
                                                  "attributeValues ) "
                                                  "VALUES( ?, ?, ? )");

static const QLatin1String
UPDATE_ATTRIBUTEVALUES("UPDATE xmlattributes "
                       "SET attributeValues = ( IFNULL( ?, \"\" ) "
                       "|| IFNULL( ?, \"\" ) "
                       "|| IFNULL( attributeValues, \"\" ) ) "
                       "WHERE attribute = ? "
                       "AND associatedElement = ?");

/*----------------------------------------------------------------------------*/

DB::DB() : m_db() {
  openConnection();

  /* Create tables if this is the first time the app is run. */
  if (m_db.tables().isEmpty()) {
    createTables();
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchProcessDomDocument(const QDomDocument *domDoc) {
  assert(domDoc && !domDoc->isNull() &&
         "DB: Attempting to batch process a NULL DOM Document");

  if (domDoc && !domDoc->isNull()) {
    QString root = domDoc->documentElement().tagName();
    addRootElement(root);

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    QSqlQuery query = createQuery();

    if (!query.prepare("INSERT OR REPLACE INTO xml( "
                       "value, attribute, element, parent, root ) "
                       "VALUES( ?, ?, ?, ?, ? )")) {
      QString error = QString("Prepare batch INSERT elements failed: [%1]")
                          .arg(query.lastError().text());

      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    BatchProcessHelper helper(domDoc);
    helper.bindValues(query);

    if (!query.execBatch()) {
      QString error = QString("Batch INSERT or REPLACE elements failed: [%1]")
                          .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::addElement(const QString &element, const QString &parent,
                    const QString &root) {
  assert(!element.isEmpty() && "DB::addElement - element string empty");
  assert(!parent.isEmpty() && "DB::addElement - parent string empty");
  assert(!root.isEmpty() && "DB::addElement - root string empty");

  if (!element.isEmpty() && !parent.isEmpty() && !root.isEmpty()) {
    QSqlQuery query = selectElement(root, element);

    /* If we don't have an existing record, add it. */
    if (!query.first()) {
      if (!query.prepare(
              "INSERT INTO xml( value, attribute, element, parent, root ) "
              "VALUES( \"\", \"\", ?, ?, ? )")) {
        QString error =
            QString("Prepare INSERT element failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      query.addBindValue(element);
      query.addBindValue(parent);
      query.addBindValue(root);

      if (!query.exec()) {
        QString error =
            QString("INSERT element failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::addRootElement(const QString &root) {
  if (!root.isEmpty()) {
    QSqlQuery query = createQuery();

    /* First see if we perhaps know of this root element already. */
    if (!query.prepare("SELECT * FROM rootelements WHERE root = ? ")) {
      QString error =
          QString("Prepare SELECT root element failed for root \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    query.addBindValue(root);

    if (!query.exec()) {
      QString error =
          QString("SELECT root element failed for root \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    /* Make sure we aren't trying to insert a known root element. */
    if (!query.first()) {
      if (!query.prepare("INSERT INTO rootelements ( root ) VALUES( ? )")) {
        QString error =
            QString("Prepare INSERT root element failed for root \"%1\": [%2]")
                .arg(root)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      query.addBindValue(root);

      if (!query.exec()) {
        QString error =
            QString("INSERT root element failed for element \"%1\": [%2]")
                .arg(root)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::updateElementAttributes(const QString &associatedRoot,
                                 const QString &element,
                                 const QStringList &attributes, bool replace) {
  if (!element.isEmpty()) {
    QSqlQuery query = selectElement(associatedRoot, element);

    /* Update the existing record (if we have one). */
    if (query.first()) {
      QStringList allAttributes;

//      if (!replace) {
//        allAttributes.append(
//            query.record().field("attributes").value().toString().split(
//                SEPARATOR));
//      }

      /* Add it here so that we append the new attributes to the end of the
       * list.
       */
      allAttributes.append(attributes);

      if (!query.prepare(UPDATE_ATTRIBUTES)) {
        QString error = QString("Prepare UPDATE element attribute failed for "
                                "element \"%1\": [%2]")
                            .arg(element)
                            .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      //query.addBindValue(cleanAndJoinListElements(allAttributes));
      query.addBindValue(element);
      query.addBindValue(associatedRoot);

      if (!query.exec()) {
        QString error =
            QString("UPDATE attribute failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
      }
    } else {
      QString error = QString("No element \"%1\" exists.").arg(element);
      emit dbActionStatus(ActionStatus::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::updateAttributeValues(const QString &element, const QString &attribute,
                               const QStringList &attributeValues,
                               bool replace) const {
  if (!element.isEmpty() && !attribute.isEmpty()) {

    QSqlQuery query = selectAttribute(attribute, element);

    /* If we don't have an existing record, add it, otherwise update the
     * existing
     * one. */
    if (!query.first()) {
      if (!query.prepare(INSERT_ATTRIBUTEVALUES)) {
        QString error = QString("Prepare INSERT attribute value failed for "
                                "element \"%1\": [%2]")
                            .arg(element)
                            .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      query.addBindValue(attribute);
      query.addBindValue(element);
      //query.addBindValue(cleanAndJoinListElements(attributeValues));

      if (!query.exec()) {
        QString error =
            QString("INSERT attribute failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }
    } else {
      QStringList existingValues(attributeValues);

//      if (!replace) {
//        existingValues.append(
//            query.record().field("attributeValues").value().toString().split(
//                SEPARATOR));
//      }

      /* The reason for not using concatenated values here is that we don't
        simply want to add all the supposed new values, we want to make sure
        they are all unique by removing all duplicates before sticking it all
        back into the DB. */
      if (!query.prepare(UPDATE_ATTRIBUTEVALUES)) {
        QString error = QString("Prepare UPDATE attribute values failed for "
                                "element \"%1\" and attribute \"%2\": [%3]")
                            .arg(element)
                            .arg(attribute)
                            .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      //query.addBindValue(cleanAndJoinListElements(existingValues));
      query.addBindValue(attribute);
      query.addBindValue(element);

      if (!query.exec()) {
        QString error = QString("UPDATE attribute values failed for element "
                                "\"%1\" and attribute [%2]: [%3]")
                            .arg(element)
                            .arg(attribute)
                            .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeElement(const QString &associatedRoot, const QString &element) {
  QSqlQuery query = selectElement(associatedRoot, element);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare("DELETE FROM xml WHERE element = ?")) {
      QString error =
          QString("Prepare DELETE element failed for element \"%1\": [%3]")
              .arg(element)
              .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    query.addBindValue(element);
    query.addBindValue(associatedRoot);

    if (!query.exec()) {
      QString error = QString("DELETE element failed for element \"%1\": [%3]")
                          .arg(element)
                          .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeChildElement(const QString &associatedRoot,
                            const QString &element, const QString &child) {
  QSqlQuery query = selectElement(associatedRoot, element);

  /* Update the existing record (if we have one). */
//  if (query.first()) {
//    QStringList allChildren(
//        query.record().field("children").value().toString().split(SEPARATOR));
//    allChildren.removeAll(child);
//    updateElementChildren(associatedRoot, element, allChildren, true);
//  }
}

/*----------------------------------------------------------------------------*/

void DB::removeAttribute(const QString &associatedRoot, const QString &element,
                         const QString &attribute) {
  QSqlQuery query = selectAttribute(attribute, element);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare("DELETE FROM xmlattributes "
                       "WHERE attribute = ? "
                       "AND associatedElement = ?")) {
      QString error = QString("Prepare DELETE attribute failed for element "
                              "\"%1\" and attribute \"%2\": [%3]")
                          .arg(element)
                          .arg(attribute)
                          .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    query.addBindValue(attribute);
    query.addBindValue(element);

    if (!query.exec()) {
      QString error = QString("DELETE attribute failed for element \"%1\" and "
                              "attribute [%2]: [%3]")
                          .arg(element)
                          .arg(attribute)
                          .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }
  }

  query = selectElement(associatedRoot, element);
  QStringList allAttributes = attributes(associatedRoot, element);
  allAttributes.removeAll(attribute);
  updateElementAttributes(associatedRoot, element, allAttributes, true);
}

/*----------------------------------------------------------------------------*/

void DB::removeRootElement(const QString &root) const {
  QSqlQuery query(m_db);

  if (!query.prepare("DELETE FROM rootelements WHERE root = ?")) {
    QString error = QString("Prepare DELETE failed for root \"%1\": [%2]")
                        .arg(root)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(root);

  if (!query.exec()) {
    QString error = QString("DELETE root element failed for root \"%1\": [%2]")
                        .arg(root)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

bool DB::isProfileEmpty() const { return knownRootElements().isEmpty(); }

/*----------------------------------------------------------------------------*/

bool DB::isUniqueChildElement(const QString &associatedRoot,
                              const QString &parentElement,
                              const QString &element) const {
  QSqlQuery query = selectAllElements(associatedRoot);

  while (query.next()) {
//    if (query.record().field("element").value().toString() != parentElement &&
//        query.record().value("children").toString().split(SEPARATOR).contains(
//            element)) {
//      return false;
//    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownElements(const QString &associatedRoot) const {
  QSqlQuery query = selectAllElements(associatedRoot);
  QStringList elementNames;

  while (query.next()) {
    elementNames.append(query.record().field("element").value().toString());
  }

  //cleanList(elementNames);
  elementNames.sort();
  return elementNames;
}

/*----------------------------------------------------------------------------*/

QStringList DB::children(const QString &associatedRoot,
                         const QString &element) {
  QSqlQuery query = selectElement(associatedRoot, element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("ActionStatus::Failed to obtain the list of "
                            "children for element \"%1\"").arg(element);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList children; /*=
      query.record().value("children").toString().split(SEPARATOR);
  cleanList(children);*/
  children.sort();
  return children;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributes(const QString &associatedRoot,
                           const QString &element) {
  QSqlQuery query = selectElement(associatedRoot, element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("ActionStatus::Failed to obtain the list of "
                            "attributes for element \"%1\"").arg(element);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList attributes; /*=
      query.record().value("attributes").toString().split(SEPARATOR);
  cleanList(attributes);*/
  return attributes;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributeValues(const QString &element,
                                const QString &attribute) const {
  QSqlQuery query = selectAttribute(attribute, element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error =
        QString("ActionStatus::Failed to obtain the list of attribute values "
                "for attribute \"%1\"").arg(attribute);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList attributeValues; /*=
      query.record().value("attributeValues").toString().split(SEPARATOR);
  cleanList(attributeValues);*/
  attributeValues.sort();
  return attributeValues;
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownRootElements() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM rootelements")) {
    QString error = QString("SELECT all root elements failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList rootElements;

  while (query.next()) {
    rootElements.append(query.record().field("root").value().toString());
  }

  //cleanList(rootElements);
  return rootElements;
}

/*----------------------------------------------------------------------------*/

bool DB::hasRootElement(const QString &root) const {
  return knownRootElements().contains(root);
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownAttributeKeys() const {
  QSqlQuery query = selectAllAttributes();
  QStringList attributeNames;

  while (query.next()) {
    /* Concatenate the attribute name and associated element into a single
      string so that it is easier to determine whether a record already
      exists for that particular combination (this is used in
      BatchProcessHelper). */
    attributeNames.append(
        query.record().field("attribute").value().toString() + "!" +
        query.record().field("associatedElement").value().toString());
  }

  return attributeNames;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectElement(const QString &associatedRoot,
                            const QString &element) {
  /* See if we already have this element in the DB. */
  QSqlQuery query = createQuery();

  if (!query.prepare("SELECT * FROM xml WHERE element = ? AND root = ?")) {
    QString error = QString("Prepare SELECT failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  query.addBindValue(element);
  query.addBindValue(associatedRoot);

  if (!query.exec()) {
    QString error = QString("SELECT element failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllElements(const QString &associatedRoot) const {
  QSqlQuery query(m_db);

  if (!query.prepare("SELECT * FROM xml WHERE root = ?")) {
    QString error = QString("Prepare SELECT failed for root \"%1\": [%2]")
                        .arg(associatedRoot)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  query.addBindValue(associatedRoot);

  if (!query.exec()) {
    QString error = QString("SELECT all root elements failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAttribute(const QString &attribute,
                              const QString &associatedElement) const {
  QSqlQuery query(m_db);

  if (!query.prepare("SELECT * FROM xmlattributes "
                     "WHERE attribute = ? "
                     "AND associatedElement = ?")) {
    QString error = QString("Prepare SELECT attribute failed for attribute "
                            "\"%1\" and element \"%2\": [%3]")
                        .arg(attribute)
                        .arg(associatedElement)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  query.addBindValue(attribute);
  query.addBindValue(associatedElement);

  if (!query.exec()) {
    QString error = QString("SELECT attribute failed for attribute \"%1\" and "
                            "element \"%2\": [%3]")
                        .arg(attribute)
                        .arg(associatedElement)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllAttributes() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM xmlattributes")) {
    QString error = QString("SELECT all attribute values failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::createQuery() {
  if (!m_db.isValid() || !m_db.isOpen()) {
    openConnection();
  }

  QSqlQuery query(m_db);
  query.setForwardOnly(true);
  return query;
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicatesFromFields(const QString &root) {
  /* Remove duplicates and update the element records. */
  QStringList elementNames = knownElements(root);

  for (int i = 0; i < elementNames.size(); ++i) {
    const QString &element = elementNames.at(i);
    QSqlQuery query = selectElement(root, element);

    /* Not checking for query validity since the table may still be empty when
      this funciton gets called (i.e. there is a potentially valid reason for
      cases where no valid records exist). */
    if (query.first()) {
      removeDuplicateElementChildren(query, root, element);
      removeDuplicateElementAttributes(query, root, element);
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }

  /* Remove duplicates and update the attribute values records. */
  QStringList attributeKeys = knownAttributeKeys();

  for (int i = 0; i < attributeKeys.size(); ++i) {
    QString attribute = attributeKeys.at(i).split("!").at(0);
    QString associatedElement = attributeKeys.at(i).split("!").at(1);
    QSqlQuery query = selectAttribute(attribute, associatedElement);

    /* Not checking for query validity since the table may still be empty when
      this funciton gets called (i.e. there is a potentially valid reason for
      cases where no valid records exist). So, does a record for this attribute
      exist? */
    if (query.first()) {
      removeDuplicateAttributeValues(query, associatedElement, attribute);
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicateElementChildren(QSqlQuery &query, const QString &root,
                                        const QString &element) {
//  QStringList allChildren(
//      query.record().field("children").value().toString().split(SEPARATOR));

  if (!query.prepare(UPDATE_CHILDREN)) {
    QString error = QString("Prepare UPDATE element children failed for "
                            "element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  //query.addBindValue(cleanAndJoinListElements(allChildren));
  query.addBindValue(element);
  query.addBindValue(root);

  if (!query.exec()) {
    QString error = QString("UPDATE children failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicateElementAttributes(QSqlQuery &query, const QString &root,
                                          const QString &element) {
//  QStringList allAttributes(
//      query.record().field("attributes").value().toString().split(SEPARATOR));

  if (!query.prepare(UPDATE_ATTRIBUTES)) {
    QString error = QString("Prepare UPDATE element attributes failed for "
                            "element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  //query.addBindValue(cleanAndJoinListElements(allAttributes));
  query.addBindValue(element);
  query.addBindValue(root);

  if (!query.exec()) {
    QString error = QString("UPDATE attributes failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicateAttributeValues(QSqlQuery &query,
                                        const QString &associatedElement,
                                        const QString &attribute) {
//  QStringList allValues(
//      query.record().field("attributeValues").value().toString().split(
//          SEPARATOR));

  if (!query.prepare(UPDATE_ATTRIBUTEVALUES)) {
    QString error = QString("Prepare UPDATE attribute values failed for "
                            "element \"%1\" and attribute \"%2\": [%3]")
                        .arg(associatedElement)
                        .arg(attribute)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  //query.addBindValue(cleanAndJoinListElements(allValues));
  query.addBindValue(attribute);
  query.addBindValue(associatedElement);

  if (!query.exec()) {
    QString error = QString("UPDATE attribute values failed for element "
                            "\"%1\" and attribute \"%2\": [%3]")
                        .arg(associatedElement)
                        .arg(attribute)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::openConnection() {
  const QString &dbName = GlobalSettings::DB_NAME;

  if (!QSqlDatabase::contains(dbName)) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", dbName);

    /* We have to set the database name before we open it. */
    if (m_db.isValid()) {
      m_db.setDatabaseName(dbName);
    }
  } else {
    m_db = QSqlDatabase::database(dbName);
  }

  if (!m_db.open()) {
    QString error =
        QString("ActionStatus::Failed to open database \"%1\": [%2].")
            .arg(dbName)
            .arg(m_db.lastError().text());

    emit dbActionStatus(ActionStatus::Critical, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::createTables() {
  /* DB connection will be open from openConnection() so no need to do any
   * checks here. */
  createRootTable();
  createXmlTable();
}

/*----------------------------------------------------------------------------*/

void DB::createRootTable() {
  /* The rootelements table is used as a unique DOM document type
   * identifier, the assumption being that a specific root element
   * refers to a specific XML document type/style. */
  QSqlQuery query = createQuery();

  if (!query.exec(
          "CREATE TABLE rootelements( root VARCHAR(50) PRIMARY KEY )")) {
    QString error = QString("Failed to create root elements "
                            "table for \"%1\": [%2]")
                        .arg(m_db.connectionName())
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::createXmlTable() {
  QSqlQuery query = createQuery();

  if (!query.exec("CREATE TABLE xml( "
                  "value BLOB NOT NULL, "
                  "attribute VARCHAR(50) NOT NULL, "
                  "element VARCHAR(50) NOT NULL, "
                  "parent VARCHAR(50) NOT NULL, "
                  "root VARCHAR(50) NOT NULL, "
                  "UNIQUE(value, attribute, element, parent, root), "
                  "FOREIGN KEY( root ) REFERENCES rootelements( root ) )")) {
    QString error =
        QString("ActionStatus::Failed to create XML table for \"%1\": [%2].")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Critical, error);
  }
}

/*----------------------------------------------------------------------------*/
