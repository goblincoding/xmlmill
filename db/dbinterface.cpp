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

void DB::processDocumentXml(const QString &xml) {
  if (!xml.isEmpty()) {
    QDomDocument doc;
    QString xmlErr("");
    int line(-1);
    int col(-1);

    if (doc.setContent(xml, &xmlErr, &line, &col)) {
      QString root = doc.documentElement().tagName();
      addRootElement(root);

      /* Have to start transaction before we create the query. */
      m_db.transaction();
      QSqlQuery query = createQuery();

      if (query.prepare("INSERT OR REPLACE INTO xml( "
                        "value, attribute, element, parent, root ) "
                        "VALUES( ?, ?, ?, ?, ? )")) {
        BatchProcessHelper helper(doc);
        helper.bindValues(query);

        if (!query.execBatch()) {
          QString error =
              QString("Batch INSERT or REPLACE elements failed: [%1]")
                  .arg(query.lastError().text());
          emit result(Result::Failed, error);
        }

        m_db.commit();
        emit result(Result::ImportSuccess, "");
      } else {
        QString error = QString("Prepare batch INSERT elements failed: [%1]")
                            .arg(query.lastError().text());

        m_db.rollback();
        emit result(Result::Failed, error);
      }
    } else {
      QString error = QString("XML is broken: [%1], line %2, column %3")
                          .arg(xmlErr)
                          .arg(line)
                          .arg(col);
      emit result(Result::Failed, error);
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
    QSqlQuery query = selectElement(root, parent, element);

    /* If we don't have an existing record, add it. */
    if (!query.first()) {
      if (!query.prepare(
              "INSERT INTO xml( value, attribute, element, parent, root ) "
              "VALUES( \"\", \"\", ?, ?, ? )")) {
        QString error =
            QString("Prepare INSERT element failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit result(Result::Failed, error);
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
        emit result(Result::Failed, error);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::addRootElement(const QString &root) {
  assert(!root.isEmpty() &&
         "DB::addRootElement - attempting to add an empty root name");

  /* Since we are dealing with a single column table, it is faster (and much
   * less complicated) to simply overwrite an existing record if it exists, than
   * to first query its existence before inserting a new record. */
  if (!root.isEmpty()) {
    QSqlQuery query = createQuery();

    if (!query.prepare(
            "INSERT or REPLACE INTO rootelements ( root ) VALUES( ? )")) {
      QString error =
          QString("Prepare INSERT root element failed for root \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      emit result(Result::Failed, error);
      return;
    }

    query.addBindValue(root);

    if (!query.exec()) {
      QString error =
          QString("INSERT root element failed for element \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      emit result(Result::Failed, error);
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
        emit result(Result::Failed, error);
        return;
      }

      query.addBindValue(attribute);
      query.addBindValue(element);
      // query.addBindValue(cleanAndJoinListElements(attributeValues));

      if (!query.exec()) {
        QString error =
            QString("INSERT attribute failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit result(Result::Failed, error);
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
        emit result(Result::Failed, error);
        return;
      }

      // query.addBindValue(cleanAndJoinListElements(existingValues));
      query.addBindValue(attribute);
      query.addBindValue(element);

      if (!query.exec()) {
        QString error = QString("UPDATE attribute values failed for element "
                                "\"%1\" and attribute [%2]: [%3]")
                            .arg(element)
                            .arg(attribute)
                            .arg(query.lastError().text());
        emit result(Result::Failed, error);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeElement(const QString &element, const QString &parent,
                       const QString &root) {
  QSqlQuery query = selectElement(element, parent, root);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare(
            "DELETE FROM xml WHERE element = ? AND parent = ? AND root = ?")) {
      QString error =
          QString("Prepare DELETE element failed for element \"%1\": [%3]")
              .arg(element)
              .arg(query.lastError().text());
      emit result(Result::Failed, error);
      return;
    }

    query.addBindValue(element);
    query.addBindValue(parent);
    query.addBindValue(root);

    if (!query.exec()) {
      QString error = QString("DELETE element failed for element \"%1\": [%3]")
                          .arg(element)
                          .arg(query.lastError().text());
      emit result(Result::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeRootElement(const QString &root) const {
  QSqlQuery query(m_db);

  if (!query.prepare("DELETE FROM rootelements WHERE root = ?")) {
    QString error = QString("Prepare DELETE failed for root \"%1\": [%2]")
                        .arg(root)
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
    return;
  }

  query.addBindValue(root);

  if (!query.exec()) {
    QString error = QString("DELETE root element failed for root \"%1\": [%2]")
                        .arg(root)
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
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
    //    if (query.record().field("element").value().toString() !=
    // parentElement &&
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

  // cleanList(elementNames);
  elementNames.sort();
  return elementNames;
}

/*----------------------------------------------------------------------------*/

QStringList DB::children(const QString &element, const QString &parent,
                         const QString &root) {
  QSqlQuery query = selectElement(element, parent, root);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("Result::Failed to obtain the list of "
                            "children for element \"%1\"").arg(element);
    emit result(Result::Failed, error);
    return QStringList();
  }

  QStringList children; /*=
      query.record().value("children").toString().split(SEPARATOR);
  cleanList(children);*/
  children.sort();
  return children;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributes(const QString &element, const QString &parent,
                           const QString &root) {
  QSqlQuery query = selectElement(element, parent, root);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("Result::Failed to obtain the list of "
                            "attributes for element \"%1\"").arg(element);
    emit result(Result::Failed, error);
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
        QString("Result::Failed to obtain the list of attribute values "
                "for attribute \"%1\"").arg(attribute);
    emit result(Result::Failed, error);
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
    emit result(Result::Failed, error);
    return QStringList();
  }

  QStringList rootElements;

  while (query.next()) {
    rootElements.append(query.record().field("root").value().toString());
  }

  // cleanList(rootElements);
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

QSqlQuery DB::selectElement(const QString &element, const QString &parent,
                            const QString &root) {
  QSqlQuery query = createQuery();

  if (!query.prepare(
          "SELECT * FROM xml WHERE element = ? AND parent = ? AND root = ?")) {
    QString error = QString("Prepare SELECT failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
  }

  query.addBindValue(element);
  query.addBindValue(parent);
  query.addBindValue(root);

  if (!query.exec()) {
    QString error = QString("SELECT element failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
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
    emit result(Result::Failed, error);
  }

  query.addBindValue(associatedRoot);

  if (!query.exec()) {
    QString error = QString("SELECT all root elements failed: [%1]")
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
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
    emit result(Result::Failed, error);
  }

  query.addBindValue(attribute);
  query.addBindValue(associatedElement);

  if (!query.exec()) {
    QString error = QString("SELECT attribute failed for attribute \"%1\" and "
                            "element \"%2\": [%3]")
                        .arg(attribute)
                        .arg(associatedElement)
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllAttributes() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM xmlattributes")) {
    QString error = QString("SELECT all attribute values failed: [%1]")
                        .arg(query.lastError().text());
    emit result(Result::Failed, error);
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
    QString error = QString("Result::Failed to open database \"%1\": [%2].")
                        .arg(dbName)
                        .arg(m_db.lastError().text());

    emit result(Result::Critical, error);
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
    emit result(Result::Failed, error);
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
        QString("Result::Failed to create XML table for \"%1\": [%2].")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    emit result(Result::Critical, error);
  }
}

/*----------------------------------------------------------------------------*/
