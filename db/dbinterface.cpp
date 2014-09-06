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

#include "dbinterface.h"
#include "batchprocesshelper.h"
#include "../utils/globalsettings.h"

#include <QDomDocument>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

/*----------------------------------------------------------------------------*/

/* Have a look at "createTables" to see how the DB is set up. */
static const QLatin1String
INSERT_ELEMENT("INSERT INTO xmlelements( element, children, attributes ) "
               "VALUES( ?, ?, ? )");

static const QLatin1String INSERT_ATTRIBUTEVALUES(
    "INSERT INTO xmlattributes( attribute, associatedElement, attributeValues "
    ") VALUES( ?, ?, ? )");

static const QLatin1String
UPDATE_CHILDREN("UPDATE xmlelements SET children = ? WHERE element = ?");

static const QLatin1String
UPDATE_ATTRIBUTES("UPDATE xmlelements SET attributes = ? WHERE element = ?");

static const QLatin1String
UPDATE_ATTRIBUTEVALUES("UPDATE xmlattributes SET attributeValues = ? WHERE "
                       "attribute = ? AND associatedElement = ?");

/*----------------------------------------------------------------------------*/

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file
 * path. */
static const QString REGEXP_SLASHES("(\\\\|\\/)");

/* The database tables have fields containing strings of strings. For example,
  the "xmlelements" table maps a unique element against a list of all associated
  attribute values.  Since these values have to be entered into a single record,
  the easiest way is to insert a single (possibly massive) string containing all
  the associated attributes. To ensure that we can later extract the individual
  attributes again, we separate them with a sequence that should (theoretically)
  never be encountered.  This is that sequence. */
static const QString SEPARATOR("~!@");

/*--------------------------NON-MEMBER UTILITY FUNCTIONS---------------------*/

void cleanList(QStringList &list) {
  list.removeDuplicates();
  list.removeAll("");
}

/*----------------------------------------------------------------------------*/

QString cleanAndJoinListElements(QStringList list) {
  cleanList(list);
  return list.join(SEPARATOR);
}

/*--------------------------MEMBER FUNCTIONS---------------------------------*/

DB *DB::m_instance = NULL;

DB *DB::instance() {
  if (!m_instance) {
    m_instance = new DB;
  }

  return m_instance;
}

/*----------------------------------------------------------------------------*/

DB::DB() : m_db(), m_lastErrorMsg("") {

  if (!openConnection()) {
    m_lastErrorMsg = QString("Failed to load the database: \n %1")
                         .arg(GlobalSettings::DB_NAME);
  }
}

/*----------------------------------------------------------------------------*/

bool
DB::batchProcessDomDocument(const QDomDocument *domDoc) const {
  BatchProcessorHelper helper(domDoc, SEPARATOR, knownElements(),
                              knownAttributeKeys());

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  if (!addRootElement(domDoc->documentElement().tagName())) {
    /* Last error message is set in "addRootElement". */
    return false;
  }

  QSqlQuery query(m_db);

  /* Batch insert all the new elements. */
  if (!query.prepare(INSERT_ELEMENT)) {
    m_lastErrorMsg = QString("Prepare batch INSERT elements failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  query.addBindValue(helper.newElementsToAdd());
  query.addBindValue(helper.newElementChildrenToAdd());
  query.addBindValue(helper.newElementAttributesToAdd());

  if (!query.execBatch()) {
    m_lastErrorMsg = QString("Batch INSERT elements failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  /* Batch update all the existing elements by concatenating the new values to
    the
    existing values. The second '?' represents our string SEPARATOR. */
  if (!query.prepare("UPDATE xmlelements "
                     "SET children = ( IFNULL( ?, \"\" ) || IFNULL( ?, \"\" ) "
                     "|| IFNULL( children, \"\" ) ) "
                     "WHERE element = ?")) {
    m_lastErrorMsg =
        QString("Prepare batch UPDATE element children failed: [%1]")
            .arg(query.lastError().text());
    return false;
  }

  /* Since we're doing batch updates we need to ensure that all the variant
    lists we provide have exactly the same size.  We furthermore require that
    the concatenation of new and old values are done in a way that includes
    our SEPARATOR string (which is why the separator list below). */
  QVariantList separatorList;

  for (int i = 0; i < helper.elementsToUpdate().size(); ++i) {
    separatorList << SEPARATOR;
  }

  query.addBindValue(helper.elementChildrenToUpdate());
  query.addBindValue(separatorList);
  query.addBindValue(helper.elementsToUpdate());

  if (!query.execBatch()) {
    m_lastErrorMsg = QString("Batch UPDATE element children failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  if (!query.prepare("UPDATE xmlelements "
                     "SET attributes = ( IFNULL( ?, \"\" ) || IFNULL( ?, \"\" "
                     ") || IFNULL( attributes, \"\" )  ) "
                     "WHERE element = ?")) {
    m_lastErrorMsg =
        QString("Prepare batch UPDATE element attributes failed: [%1]")
            .arg(query.lastError().text());
    return false;
  }

  query.addBindValue(helper.elementAttributesToUpdate());
  query.addBindValue(separatorList);
  query.addBindValue(helper.elementsToUpdate());

  if (!query.execBatch()) {
    m_lastErrorMsg = QString("Batch UPDATE element attributes failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  /* Batch insert all the new attribute values. */
  if (!query.prepare(INSERT_ATTRIBUTEVALUES)) {
    m_lastErrorMsg =
        QString("Prepare batch INSERT attribute values failed: [%1]")
            .arg(query.lastError().text());
    return false;
  }

  query.addBindValue(helper.newAttributeKeysToAdd());
  query.addBindValue(helper.newAssociatedElementsToAdd());
  query.addBindValue(helper.newAttributeValuesToAdd());

  if (!query.execBatch()) {
    m_lastErrorMsg = QString("Batch INSERT attribute values failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  /* Batch update all the existing attribute values. */
  if (!query.prepare("UPDATE xmlattributes "
                     "SET attributeValues = ( IFNULL( ?, \"\" ) || IFNULL( ?, "
                     "\"\" ) || IFNULL( attributeValues, \"\" ) ) "
                     "WHERE attribute = ? "
                     "AND associatedElement = ?")) {
    m_lastErrorMsg =
        QString("Prepare batch UPDATE attribute values failed: [%1]")
            .arg(query.lastError().text());
    return false;
  }

  separatorList.clear();

  for (int i = 0; i < helper.attributeKeysToUpdate().size(); ++i) {
    separatorList << SEPARATOR;
  }

  query.addBindValue(helper.attributeValuesToUpdate());
  query.addBindValue(separatorList);
  query.addBindValue(helper.attributeKeysToUpdate());
  query.addBindValue(helper.associatedElementsToUpdate());

  if (!query.execBatch()) {
    m_lastErrorMsg = QString("Batch UPDATE attribute values failed: [%1]")
                         .arg(query.lastError().text());
    return false;
  }

  return removeDuplicatesFromFields();
}

/*----------------------------------------------------------------------------*/

bool DB::addElement(const QString &element,
                                   const QStringList &children,
                                   const QStringList &attributes) const {
  if (element.isEmpty()) {
    m_lastErrorMsg = QString("Trying to add an empty element name.");
    return false;
  }

  QSqlQuery query = selectElement(element);

  /* If we don't have an existing record, add it. */
  if (!query.first()) {
    if (!query.prepare(INSERT_ELEMENT)) {
      m_lastErrorMsg =
          QString("Prepare INSERT element failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(element);
    query.addBindValue(cleanAndJoinListElements(children));
    query.addBindValue(cleanAndJoinListElements(attributes));

    if (!query.exec()) {
      m_lastErrorMsg = QString("INSERT element failed for element \"%1\": [%2]")
                           .arg(element)
                           .arg(query.lastError().text());
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::addRootElement(const QString &root) const {
  if (root.isEmpty()) {
    m_lastErrorMsg = QString("Trying to add an empty root element name.");
    return false;
  }

  QSqlQuery query(m_db);

  if (!query.prepare("SELECT * FROM rootelements WHERE root = ? ")) {
    m_lastErrorMsg =
        QString("Prepare SELECT root element failed for root \"%1\": [%2]")
            .arg(root)
            .arg(query.lastError().text());
    return false;
  }

  query.addBindValue(root);

  if (!query.exec()) {
    m_lastErrorMsg = QString("SELECT root element failed for root \"%1\": [%2]")
                         .arg(root)
                         .arg(query.lastError().text());
    return false;
  }

  /* Make sure we aren't trying to insert a known root element. */
  if (!query.first()) {
    if (!query.prepare("INSERT INTO rootelements ( root ) VALUES( ? )")) {
      m_lastErrorMsg =
          QString("Prepare INSERT root element failed for root \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(root);

    if (!query.exec()) {
      m_lastErrorMsg =
          QString("INSERT root element failed for element \"%1\": [%2]")
              .arg(root)
              .arg(query.lastError().text());
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::updateElementChildren(const QString &element,
                                              const QStringList &children,
                                              bool replace) const {
  if (element.isEmpty()) {
    m_lastErrorMsg = QString("Invalid element name provided.");
    return false;
  }

  QSqlQuery query = selectElement(element);

  /* Update the existing record (if we have one). */
  if (query.first()) {
    QStringList allChildren(children);

    if (!replace) {
      allChildren.append(
          query.record().field("children").value().toString().split(SEPARATOR));
    }

    if (!query.prepare(UPDATE_CHILDREN)) {
      m_lastErrorMsg =
          QString(
              "Prepare UPDATE element children failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(cleanAndJoinListElements(allChildren));
    query.addBindValue(element);

    if (!query.exec()) {
      m_lastErrorMsg =
          QString("UPDATE children failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }
  } else {
    m_lastErrorMsg =
        QString("No knowledge of element \"%1\", add it first.").arg(element);
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::updateElementAttributes(const QString &element,
                                                const QStringList &attributes,
                                                bool replace) const {
  if (element.isEmpty()) {
    m_lastErrorMsg = QString("Invalid element name provided.");
    return false;
  }

  QSqlQuery query = selectElement(element);

  /* Update the existing record (if we have one). */
  if (query.first()) {
    QStringList allAttributes;

    if (!replace) {
      allAttributes.append(
          query.record().field("attributes").value().toString().split(
              SEPARATOR));
    }

    /* Add it here so that we append the new attributes to the end of the list.
     */
    allAttributes.append(attributes);

    if (!query.prepare(UPDATE_ATTRIBUTES)) {
      m_lastErrorMsg = QString("Prepare UPDATE element attribute failed for "
                               "element \"%1\": [%2]")
                           .arg(element)
                           .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(cleanAndJoinListElements(allAttributes));
    query.addBindValue(element);

    if (!query.exec()) {
      m_lastErrorMsg =
          QString("UPDATE attribute failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }
  } else {
    m_lastErrorMsg = QString("No element \"%1\" exists.").arg(element);
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::updateAttributeValues(
    const QString &element, const QString &attribute,
    const QStringList &attributeValues, bool replace) const {
  if (element.isEmpty() || attribute.isEmpty()) {
    m_lastErrorMsg = QString("Invalid element or attribute values provided.");
    return false;
  }

  QSqlQuery query = selectAttribute(attribute, element);

  /* If we don't have an existing record, add it, otherwise update the existing
   * one. */
  if (!query.first()) {
    if (!query.prepare(INSERT_ATTRIBUTEVALUES)) {
      m_lastErrorMsg =
          QString(
              "Prepare INSERT attribute value failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(attribute);
    query.addBindValue(element);
    query.addBindValue(cleanAndJoinListElements(attributeValues));

    if (!query.exec()) {
      m_lastErrorMsg =
          QString("INSERT attribute failed for element \"%1\": [%2]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }
  } else {
    QStringList existingValues(attributeValues);

    if (!replace) {
      existingValues.append(
          query.record().field("attributeValues").value().toString().split(
              SEPARATOR));
    }

    /* The reason for not using concatenating values here is that we don't
      simply want to add all the supposed new values, we want to make sure
      they are all unique by removing all duplicates before sticking it all
      back into the DB. */
    if (!query.prepare(UPDATE_ATTRIBUTEVALUES)) {
      m_lastErrorMsg = QString("Prepare UPDATE attribute values failed for "
                               "element \"%1\" and attribute \"%2\": [%3]")
                           .arg(element)
                           .arg(attribute)
                           .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(cleanAndJoinListElements(existingValues));
    query.addBindValue(attribute);
    query.addBindValue(element);

    if (!query.exec()) {
      m_lastErrorMsg = QString("UPDATE attribute values failed for element "
                               "\"%1\" and attribute [%2]: [%3]")
                           .arg(element)
                           .arg(attribute)
                           .arg(query.lastError().text());
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::removeElement(const QString &element) const {
  QSqlQuery query = selectElement(element);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare("DELETE FROM xmlelements WHERE element = ?")) {
      m_lastErrorMsg =
          QString("Prepare DELETE element failed for element \"%1\": [%3]")
              .arg(element)
              .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(element);

    if (!query.exec()) {
      m_lastErrorMsg = QString("DELETE element failed for element \"%1\": [%3]")
                           .arg(element)
                           .arg(query.lastError().text());
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::removeChildElement(const QString &element,
                                           const QString &child) const {
  QSqlQuery query = selectElement(element);

  /* Update the existing record (if we have one). */
  if (query.first()) {
    QStringList allChildren(
        query.record().field("children").value().toString().split(SEPARATOR));
    allChildren.removeAll(child);
    updateElementChildren(element, allChildren, true);
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::removeAttribute(const QString &element,
                                        const QString &attribute) const {
  QSqlQuery query = selectAttribute(attribute, element);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare("DELETE FROM xmlattributes "
                       "WHERE attribute = ? "
                       "AND associatedElement = ?")) {
      m_lastErrorMsg = QString("Prepare DELETE attribute failed for element "
                               "\"%1\" and attribute \"%2\": [%3]")
                           .arg(element)
                           .arg(attribute)
                           .arg(query.lastError().text());
      return false;
    }

    query.addBindValue(attribute);
    query.addBindValue(element);

    if (!query.exec()) {
      m_lastErrorMsg = QString("DELETE attribute failed for element \"%1\" and "
                               "attribute [%2]: [%3]")
                           .arg(element)
                           .arg(attribute)
                           .arg(query.lastError().text());
      return false;
    }
  }

  query = selectElement(element);
  QStringList allAttributes = attributes(element);
  allAttributes.removeAll(attribute);
  updateElementAttributes(element, allAttributes, true);

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::removeRootElement(const QString &element) const {
  QSqlQuery query(m_db);

  if (!query.prepare("DELETE FROM rootelements WHERE root = ?")) {
    m_lastErrorMsg = QString("Prepare DELETE failed for root \"%1\": [%2]")
                         .arg(element)
                         .arg(query.lastError().text());
    return false;
  }

  query.addBindValue(element);

  if (!query.exec()) {
    m_lastErrorMsg = QString("DELETE root element failed for root \"%1\": [%2]")
                         .arg(element)
                         .arg(query.lastError().text());
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::isProfileEmpty() const {
  return knownRootElements().isEmpty();
}

/*----------------------------------------------------------------------------*/

bool DB::isUniqueChildElement(const QString &parentElement,
                                             const QString &element) const {
  QSqlQuery query = selectAllElements();

  while (query.next()) {
    if (query.record().field("element").value().toString() != parentElement &&
        query.record().value("children").toString().split(SEPARATOR).contains(
            element)) {
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::isDocumentCompatible(const QDomDocument *doc) const {
  BatchProcessorHelper helper(doc, SEPARATOR, knownElements(),
                              knownAttributeKeys());

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  /* If there are any new elements or attributes to add, the document is
    incompatible (not checking for new attribute values since new values
    don't affect XML relationships, i.e. it isn't important enough to
    import entire documents each time an unknown value is encountered)
    */
  if (!helper.newAssociatedElementsToAdd().isEmpty() ||
      !helper.newAttributeKeysToAdd().isEmpty() ||
      !helper.newElementAttributesToAdd().isEmpty() ||
      !helper.newElementChildrenToAdd().isEmpty() ||
      !helper.newElementsToAdd().isEmpty()) {
    return false;
  }

  /* If there are no new items, we may still have previously unknown
    relationships which we need to check.  This is slightly more
    involved than simply checking for empty lists.
    Also remember that the lists returned by BatchProcessorHelper are all
    synchronised with regards to their indices (which is the only reason
    why we can loop through the lists like this). */
  for (int i = 0; i < helper.elementsToUpdate().size(); ++i) {
    /* If any new element children were added, we have an incompatible document.
     */
    QStringList knownChildren =
        children(helper.elementsToUpdate().at(i).toString());
    QStringList allChildren =
        QStringList()
        << knownChildren
        << helper.elementChildrenToUpdate().at(i).toString().split(SEPARATOR);
    cleanList(allChildren);

    /* Check for larger since we only care about added items. */
    if (allChildren.size() > knownChildren.size()) {
      return false;
    }

    /* If any new attributes were added, we also have an incompatible document.
     */
    QStringList knownAttributes =
        attributes(helper.elementsToUpdate().at(i).toString());
    QStringList allAttributes =
        QStringList()
        << knownAttributes
        << helper.elementAttributesToUpdate().at(i).toString().split(SEPARATOR);
    cleanList(allAttributes);

    if (allAttributes.size() > knownAttributes.size()) {
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownElements() const {
  QSqlQuery query = selectAllElements();

  m_lastErrorMsg = "";

  QStringList elementNames;

  while (query.next()) {
    elementNames.append(query.record().field("element").value().toString());
  }

  cleanList(elementNames);
  elementNames.sort();
  return elementNames;
}

/*----------------------------------------------------------------------------*/

QStringList DB::children(const QString &element) const {
  QSqlQuery query = selectElement(element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    m_lastErrorMsg =
        QString("Failed to obtain the list of children for element \"%1\"")
            .arg(element);
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList children =
      query.record().value("children").toString().split(SEPARATOR);
  cleanList(children);
  children.sort();
  return children;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributes(const QString &element) const {
  QSqlQuery query = selectElement(element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    m_lastErrorMsg =
        QString("Failed to obtain the list of attributes for element \"%1\"")
            .arg(element);
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList attributes =
      query.record().value("attributes").toString().split(SEPARATOR);
  cleanList(attributes);
  return attributes;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributeValues(const QString &element,
                                               const QString &attribute) const {
  QSqlQuery query = selectAttribute(attribute, element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    m_lastErrorMsg = QString("Failed to obtain the list of attribute values "
                             "for attribute \"%1\"").arg(attribute);
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList attributeValues =
      query.record().value("attributeValues").toString().split(SEPARATOR);
  cleanList(attributeValues);
  attributeValues.sort();
  return attributeValues;
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownRootElements() const {
  return knownRootElements(m_db);
}

/*----------------------------------------------------------------------------*/

QStringList DB::knownRootElements(QSqlDatabase db) const {
  QSqlQuery query(db);

  if (!query.exec("SELECT * FROM rootelements")) {
    m_lastErrorMsg = QString("SELECT all root elements failed: [%1]")
                         .arg(query.lastError().text());
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList rootElements;

  while (query.next()) {
    rootElements.append(query.record().field("root").value().toString());
  }

  cleanList(rootElements);
  return rootElements;
}

/*----------------------------------------------------------------------------*/

bool DB::containsKnownRootElement(const QString &dbName,
                                                 const QString &root) const {
  /* In case the db name passed in consists of a path/to/file string. */
  QString dbConName =
      dbName.split(QRegExp(REGEXP_SLASHES), QString::SkipEmptyParts).last();

  /* No error messages are logged for this specific query since we aren't
    necessarily concerned with
    the session we're querying (it may not be the active session). */
  if (QSqlDatabase::contains(dbConName)) {
    QSqlDatabase db = QSqlDatabase::database(dbConName);

    if (db.isValid() && db.open()) {
      if (knownRootElements(db).contains(root)) {
        return true;
      } else {
        return false;
      }
    }
  }

  return false;
}

/*----------------------------------------------------------------------------*/

const QString &DB::lastError() const { return m_lastErrorMsg; }

/*----------------------------------------------------------------------------*/

QStringList DB::knownAttributeKeys() const {
  QSqlQuery query = selectAllAttributes();

  m_lastErrorMsg = "";

  QStringList attributeNames;

  while (query.next()) {
    /* Concatenate the attribute name and associated element into a single
      string so that it is easier to determine whether a record already
      exists for that particular combination (this is used in
      BatchProcessorHelper). */
    attributeNames.append(
        query.record().field("attribute").value().toString() + "!" +
        query.record().field("associatedElement").value().toString());
  }

  return attributeNames;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectElement(const QString &element) const {
  /* See if we already have this element in the DB. */
  QSqlQuery query(m_db);

  if (!query.prepare("SELECT * FROM xmlelements WHERE element = ?")) {
    m_lastErrorMsg = QString("Prepare SELECT failed for element \"%1\": [%2]")
                         .arg(element)
                         .arg(query.lastError().text());
  }

  query.addBindValue(element);

  if (!query.exec()) {
    m_lastErrorMsg = QString("SELECT element failed for element \"%1\": [%2]")
                         .arg(element)
                         .arg(query.lastError().text());
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllElements() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM xmlelements")) {
    m_lastErrorMsg = QString("SELECT all root elements failed: [%1]")
                         .arg(query.lastError().text());
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery
DB::selectAttribute(const QString &attribute,
                                   const QString &associatedElement) const {
  QSqlQuery query(m_db);

  if (!query.prepare("SELECT * FROM xmlattributes "
                     "WHERE attribute = ? "
                     "AND associatedElement = ?")) {
    m_lastErrorMsg = QString("Prepare SELECT attribute failed for attribute "
                             "\"%1\" and element \"%2\": [%3]")
                         .arg(attribute)
                         .arg(associatedElement)
                         .arg(query.lastError().text());
  }

  query.addBindValue(attribute);
  query.addBindValue(associatedElement);

  if (!query.exec()) {
    m_lastErrorMsg = QString("SELECT attribute failed for attribute \"%1\" and "
                             "element \"%2\": [%3]")
                         .arg(attribute)
                         .arg(associatedElement)
                         .arg(query.lastError().text());
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllAttributes() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM xmlattributes")) {
    m_lastErrorMsg = QString("SELECT all attribute values failed: [%1]")
                         .arg(query.lastError().text());
  }

  return query;
}

/*----------------------------------------------------------------------------*/

bool DB::removeDuplicatesFromFields() const {
  /* Remove duplicates and update the element records. */
  QStringList elementNames = knownElements();
  QString element("");

  for (int i = 0; i < elementNames.size(); ++i) {
    element = elementNames.at(i);
    QSqlQuery query = selectElement(element);

    /* Not checking for query validity since the table may still be empty when
      this funciton gets called (i.e. there is a potentially valid reason for
      cases where no valid records exist). */
    if (query.first()) {
      QStringList allChildren(
          query.record().field("children").value().toString().split(SEPARATOR));
      QStringList allAttributes(
          query.record().field("attributes").value().toString().split(
              SEPARATOR));

      if (!query.prepare(UPDATE_CHILDREN)) {
        m_lastErrorMsg = QString("Prepare UPDATE element children failed for "
                                 "element \"%1\": [%2]")
                             .arg(element)
                             .arg(query.lastError().text());
        return false;
      }

      query.addBindValue(cleanAndJoinListElements(allChildren));
      query.addBindValue(element);

      if (!query.exec()) {
        m_lastErrorMsg =
            QString("UPDATE children failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        return false;
      }

      if (!query.prepare(UPDATE_ATTRIBUTES)) {
        m_lastErrorMsg = QString("Prepare UPDATE element attributes failed for "
                                 "element \"%1\": [%2]")
                             .arg(element)
                             .arg(query.lastError().text());
        return false;
      }

      query.addBindValue(cleanAndJoinListElements(allAttributes));
      query.addBindValue(element);

      if (!query.exec()) {
        m_lastErrorMsg =
            QString("UPDATE attributes failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        return false;
      }

      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }

  /* Remove duplicates and update the attribute values records. */
  QStringList attributeKeys = knownAttributeKeys();

  /* Not checking for query validity since the table may still be empty when
    this funciton gets called (i.e. there is a potentially valid reason for
    cases where no valid records exist). */
  for (int i = 0; i < attributeKeys.size(); ++i) {
    QString attribute = attributeKeys.at(i).split("!").at(0);
    QString associatedElement = attributeKeys.at(i).split("!").at(1);
    QSqlQuery query = selectAttribute(attribute, associatedElement);

    /* Does a record for this attribute exist? */
    if (query.first()) {
      QStringList allValues(
          query.record().field("attributeValues").value().toString().split(
              SEPARATOR));

      if (!query.prepare(UPDATE_ATTRIBUTEVALUES)) {
        m_lastErrorMsg = QString("Prepare UPDATE attribute values failed for "
                                 "element \"%1\" and attribute \"%2\": [%3]")
                             .arg(associatedElement)
                             .arg(attribute)
                             .arg(query.lastError().text());
        return false;
      }

      query.addBindValue(cleanAndJoinListElements(allValues));
      query.addBindValue(attribute);
      query.addBindValue(associatedElement);

      if (!query.exec()) {
        m_lastErrorMsg = QString("UPDATE attribute values failed for element "
                                 "\"%1\" and attribute \"%2\": [%3]")
                             .arg(associatedElement)
                             .arg(attribute)
                             .arg(query.lastError().text());
        return false;
      }

      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::openConnection() {
  const QString &dbName = GlobalSettings::DB_NAME;

  m_db = QSqlDatabase::addDatabase("QSQLITE", dbName);

  if (m_db.isValid()) {
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
      m_lastErrorMsg = QString("Failed to open database \"%1\": [%2].")
                           .arg(dbName)
                           .arg(m_db.lastError().text());
      return false;
    }
  }

  /* Check if no tables have been created yet */
  if (m_db.tables().isEmpty()) {
    return createTables();
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/

bool DB::createTables() const {
  /* DB connection will be open from openConnection() above so no need to do any
   * checks here. */
  QSqlQuery query(m_db);

  if (!query.exec("CREATE TABLE xmlelements( element QString primary key, "
                  "children QString, attributes QString )")) {
    m_lastErrorMsg =
        QString("Failed to create elements table for \"%1\": [%2].")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    return false;
  }

  if (!query.exec(
          "CREATE TABLE xmlattributes( attribute QString, associatedElement "
          "QString, attributeValues QString, "
          "UNIQUE(attribute, associatedElement), "
          "FOREIGN KEY(associatedElement) REFERENCES xmlelements(element) )")) {
    m_lastErrorMsg =
        QString("Failed to create attribute values table for \"%1\": [%2]")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    return false;
  } else {
    if (!query.exec("CREATE UNIQUE INDEX attributeKey ON xmlattributes( "
                    "attribute, associatedElement)")) {
      m_lastErrorMsg = QString("Failed to create unique index for \"%1\": [%2]")
                           .arg(m_db.connectionName())
                           .arg(query.lastError().text());
      return false;
    }
  }

  if (!query.exec("CREATE TABLE rootelements( root QString primary key )")) {
    m_lastErrorMsg =
        QString("Failed to create root elements table for \"%1\": [%2]")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*----------------------------------------------------------------------------*/
