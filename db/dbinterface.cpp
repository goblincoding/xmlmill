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
#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

#include <assert.h>

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

DB::DB() : m_db() {
  openConnection();

  /* Create tables if this is the first time the app is run. */
  if (m_db.tables().isEmpty()) {
    createTables();
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchProcessDomDocument(const QDomDocument *domDoc) {
  assert(domDoc && !domDoc->isNull());

  if (domDoc && !domDoc->isNull()) {
    addRootElement(domDoc->documentElement().tagName());
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    BatchProcessHelper helper(domDoc, SEPARATOR, knownElements(),
                              knownAttributeKeys());

    /* Batch insert all the new elements. */
    batchProcessNewElements(helper);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    /* Batch update all the existing elements. */
    batchUpdateExistingElementChildren(helper);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    batchUpdateExistingElementAttributes(helper);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    /* Batch insert all the new attribute values. */
    batchProcessNewAttributeValues(helper);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    /* Batch update all the existing attribute values. */
    batchUpdateExistingAttributeValues(helper);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    removeDuplicatesFromFields();
  }
}

/*----------------------------------------------------------------------------*/

void DB::addElement(const QString &element, const QStringList &children,
                    const QStringList &attributes) {
  if (!element.isEmpty()) {
    QSqlQuery query = selectElement(element);

    /* If we don't have an existing record, add it. */
    if (!query.first()) {
      if (!query.prepare(INSERT_ELEMENT)) {
        QString error =
            QString("Prepare INSERT element failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      query.addBindValue(element);
      query.addBindValue(cleanAndJoinListElements(children));
      query.addBindValue(cleanAndJoinListElements(attributes));

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

void DB::batchProcessNewElements(BatchProcessHelper &helper) {
  QSqlQuery query = createQuery();

  if (!query.prepare(INSERT_ELEMENT)) {
    QString error = QString("Prepare batch INSERT elements failed: [%1]")
                        .arg(query.lastError().text());

    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(helper.newElementsToAdd());
  query.addBindValue(helper.newElementChildrenToAdd());
  query.addBindValue(helper.newElementAttributesToAdd());

  if (!query.execBatch()) {
    QString error = QString("Batch INSERT elements failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchUpdateExistingElementChildren(BatchProcessHelper &helper) {
  QSqlQuery query = createQuery();
  /* Concatenate new values to existing. The second '?' represents our string
   * SEPARATOR. */
  if (!query.prepare("UPDATE xmlelements "
                     "SET children = ( IFNULL( ?, \"\" ) || IFNULL( ?, \"\" ) "
                     "|| IFNULL( children, \"\" ) ) "
                     "WHERE element = ?")) {
    QString error =
        QString("Prepare batch UPDATE element children failed: [%1]")
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
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
    QString error = QString("Batch UPDATE element children failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchUpdateExistingElementAttributes(BatchProcessHelper &helper) {
  QSqlQuery query = createQuery();

  if (!query.prepare("UPDATE xmlelements "
                     "SET attributes = ( IFNULL( ?, \"\" ) || IFNULL( ?, \"\" "
                     ") || IFNULL( attributes, \"\" )  ) "
                     "WHERE element = ?")) {
    QString error =
        QString("Prepare batch UPDATE element attributes failed: [%1]")
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  /* Since we're doing batch updates we need to ensure that all the variant
    lists we provide have exactly the same size.  We furthermore require that
    the concatenation of new and old values are done in a way that includes
    our SEPARATOR string (which is why the separator list below). */
  QVariantList separatorList;

  for (int i = 0; i < helper.elementsToUpdate().size(); ++i) {
    separatorList << SEPARATOR;
  }

  query.addBindValue(helper.elementAttributesToUpdate());
  query.addBindValue(separatorList);
  query.addBindValue(helper.elementsToUpdate());

  if (!query.execBatch()) {
    QString error = QString("Batch UPDATE element attributes failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchProcessNewAttributeValues(BatchProcessHelper &helper) {
  QSqlQuery query = createQuery();

  if (!query.prepare(INSERT_ATTRIBUTEVALUES)) {
    QString error =
        QString("Prepare batch INSERT attribute values failed: [%1]")
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(helper.newAttributeKeysToAdd());
  query.addBindValue(helper.newAssociatedElementsToAdd());
  query.addBindValue(helper.newAttributeValuesToAdd());

  if (!query.execBatch()) {
    QString error = QString("Batch INSERT attribute values failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::batchUpdateExistingAttributeValues(BatchProcessHelper &helper) {
  QSqlQuery query = createQuery();

  if (!query.prepare("UPDATE xmlattributes "
                     "SET attributeValues = ( IFNULL( ?, \"\" ) || IFNULL( ?, "
                     "\"\" ) || IFNULL( attributeValues, \"\" ) ) "
                     "WHERE attribute = ? "
                     "AND associatedElement = ?")) {
    QString error =
        QString("Prepare batch UPDATE attribute values failed: [%1]")
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  /* Since we're doing batch updates we need to ensure that all the variant
    lists we provide have exactly the same size.  We furthermore require that
    the concatenation of new and old values are done in a way that includes
    our SEPARATOR string (which is why the separator list below). */
  QVariantList separatorList;

  for (int i = 0; i < helper.attributeKeysToUpdate().size(); ++i) {
    separatorList << SEPARATOR;
  }

  query.addBindValue(helper.attributeValuesToUpdate());
  query.addBindValue(separatorList);
  query.addBindValue(helper.attributeKeysToUpdate());
  query.addBindValue(helper.associatedElementsToUpdate());

  if (!query.execBatch()) {
    QString error = QString("Batch UPDATE attribute values failed: [%1]")
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::updateElementChildren(const QString &element,
                               const QStringList &children, bool replace) {
  if (!element.isEmpty()) {

    QSqlQuery query = selectElement(element);

    /* Update the existing record (if we have one). */
    if (query.first()) {
      QStringList allChildren(children);

      if (!replace) {
        allChildren.append(
            query.record().field("children").value().toString().split(
                SEPARATOR));
      }

      if (!query.prepare(UPDATE_CHILDREN)) {
        QString error = QString("Prepare UPDATE element children failed for "
                                "element \"%1\": [%2]")
                            .arg(element)
                            .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
        return;
      }

      query.addBindValue(cleanAndJoinListElements(allChildren));
      query.addBindValue(element);

      if (!query.exec()) {
        QString error =
            QString("UPDATE children failed for element \"%1\": [%2]")
                .arg(element)
                .arg(query.lastError().text());
        emit dbActionStatus(ActionStatus::Failed, error);
      }
    } else {
      QString error =
          QString("No knowledge of element \"%1\", add it first.").arg(element);
      emit dbActionStatus(ActionStatus::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::updateElementAttributes(const QString &element,
                                 const QStringList &attributes, bool replace) {
  if (!element.isEmpty()) {

    QSqlQuery query = selectElement(element);

    /* Update the existing record (if we have one). */
    if (query.first()) {
      QStringList allAttributes;

      if (!replace) {
        allAttributes.append(
            query.record().field("attributes").value().toString().split(
                SEPARATOR));
      }

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

      query.addBindValue(cleanAndJoinListElements(allAttributes));
      query.addBindValue(element);

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
      query.addBindValue(cleanAndJoinListElements(attributeValues));

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

      if (!replace) {
        existingValues.append(
            query.record().field("attributeValues").value().toString().split(
                SEPARATOR));
      }

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

      query.addBindValue(cleanAndJoinListElements(existingValues));
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

void DB::removeElement(const QString &element) {
  QSqlQuery query = selectElement(element);

  /* Only continue if we have an existing record. */
  if (query.first()) {
    if (!query.prepare("DELETE FROM xmlelements WHERE element = ?")) {
      QString error =
          QString("Prepare DELETE element failed for element \"%1\": [%3]")
              .arg(element)
              .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
      return;
    }

    query.addBindValue(element);

    if (!query.exec()) {
      QString error = QString("DELETE element failed for element \"%1\": [%3]")
                          .arg(element)
                          .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Failed, error);
    }
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeChildElement(const QString &element, const QString &child) {
  QSqlQuery query = selectElement(element);

  /* Update the existing record (if we have one). */
  if (query.first()) {
    QStringList allChildren(
        query.record().field("children").value().toString().split(SEPARATOR));
    allChildren.removeAll(child);
    updateElementChildren(element, allChildren, true);
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeAttribute(const QString &element, const QString &attribute) {
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

  query = selectElement(element);
  QStringList allAttributes = attributes(element);
  allAttributes.removeAll(attribute);
  updateElementAttributes(element, allAttributes, true);
}

/*----------------------------------------------------------------------------*/

void DB::removeRootElement(const QString &element) const {
  QSqlQuery query(m_db);

  if (!query.prepare("DELETE FROM rootelements WHERE root = ?")) {
    QString error = QString("Prepare DELETE failed for root \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(element);

  if (!query.exec()) {
    QString error = QString("DELETE root element failed for root \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

bool DB::isProfileEmpty() const { return knownRootElements().isEmpty(); }

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

bool DB::isDocumentCompatible(const QDomDocument *doc) {
  BatchProcessHelper helper(doc, SEPARATOR, knownElements(),
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
    Also remember that the lists returned by BatchProcessHelper are all
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
  QStringList elementNames;

  while (query.next()) {
    elementNames.append(query.record().field("element").value().toString());
  }

  cleanList(elementNames);
  elementNames.sort();
  return elementNames;
}

/*----------------------------------------------------------------------------*/

QStringList DB::children(const QString &element) {
  QSqlQuery query = selectElement(element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("ActionStatus::Failed to obtain the list of "
                            "children for element \"%1\"").arg(element);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList children =
      query.record().value("children").toString().split(SEPARATOR);
  cleanList(children);
  children.sort();
  return children;
}

/*----------------------------------------------------------------------------*/

QStringList DB::attributes(const QString &element) {
  QSqlQuery query = selectElement(element);

  /* There should be only one record corresponding to this element. */
  if (!query.first()) {
    QString error = QString("ActionStatus::Failed to obtain the list of "
                            "attributes for element \"%1\"").arg(element);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

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
    QString error =
        QString("ActionStatus::Failed to obtain the list of attribute values "
                "for attribute \"%1\"").arg(attribute);
    emit dbActionStatus(ActionStatus::Failed, error);
    return QStringList();
  }

  QStringList attributeValues =
      query.record().value("attributeValues").toString().split(SEPARATOR);
  cleanList(attributeValues);
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

  cleanList(rootElements);
  return rootElements;
}

/*----------------------------------------------------------------------------*/

bool DB::containsKnownRootElement(const QString &root) const {
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

QSqlQuery DB::selectElement(const QString &element) {
  /* See if we already have this element in the DB. */
  QSqlQuery query = createQuery();

  if (!query.prepare("SELECT * FROM xmlelements WHERE element = ?")) {
    QString error = QString("Prepare SELECT failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  query.addBindValue(element);

  if (!query.exec()) {
    QString error = QString("SELECT element failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }

  return query;
}

/*----------------------------------------------------------------------------*/

QSqlQuery DB::selectAllElements() const {
  QSqlQuery query(m_db);

  if (!query.exec("SELECT * FROM xmlelements")) {
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

  return QSqlQuery(m_db);
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicatesFromFields() {
  /* Remove duplicates and update the element records. */
  QStringList elementNames = knownElements();

  for (int i = 0; i < elementNames.size(); ++i) {
    const QString &element = elementNames.at(i);
    QSqlQuery query = selectElement(element);

    /* Not checking for query validity since the table may still be empty when
      this funciton gets called (i.e. there is a potentially valid reason for
      cases where no valid records exist). */
    if (query.first()) {
      removeDuplicateElementChildren(query, element);
      removeDuplicateElementAttributes(query, element);
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

void DB::removeDuplicateElementChildren(QSqlQuery &query,
                                        const QString &element) {
  QStringList allChildren(
      query.record().field("children").value().toString().split(SEPARATOR));

  if (!query.prepare(UPDATE_CHILDREN)) {
    QString error = QString("Prepare UPDATE element children failed for "
                            "element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(cleanAndJoinListElements(allChildren));
  query.addBindValue(element);

  if (!query.exec()) {
    QString error = QString("UPDATE children failed for element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/

void DB::removeDuplicateElementAttributes(QSqlQuery &query,
                                          const QString &element) {
  QStringList allAttributes(
      query.record().field("attributes").value().toString().split(SEPARATOR));

  if (!query.prepare(UPDATE_ATTRIBUTES)) {
    QString error = QString("Prepare UPDATE element attributes failed for "
                            "element \"%1\": [%2]")
                        .arg(element)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(cleanAndJoinListElements(allAttributes));
  query.addBindValue(element);

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
  QStringList allValues(
      query.record().field("attributeValues").value().toString().split(
          SEPARATOR));

  if (!query.prepare(UPDATE_ATTRIBUTEVALUES)) {
    QString error = QString("Prepare UPDATE attribute values failed for "
                            "element \"%1\" and attribute \"%2\": [%3]")
                        .arg(associatedElement)
                        .arg(attribute)
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
    return;
  }

  query.addBindValue(cleanAndJoinListElements(allValues));
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
  /* DB connection will be open from openConnection() above so no need to do any
   * checks here. */
  QSqlQuery query = createQuery();

  if (!query.exec("CREATE TABLE xmlelements( element QString primary key, "
                  "children QString, attributes QString )")) {
    QString error =
        QString(
            "ActionStatus::Failed to create elements table for \"%1\": [%2].")
            .arg(m_db.connectionName())
            .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Critical, error);
    return;
  }

  if (!query.exec(
          "CREATE TABLE xmlattributes( attribute QString, associatedElement "
          "QString, attributeValues QString, "
          "UNIQUE(attribute, associatedElement), "
          "FOREIGN KEY(associatedElement) REFERENCES xmlelements(element) )")) {
    QString error = QString("Failed to create attribute values "
                            "table for \"%1\": [%2]")
                        .arg(m_db.connectionName())
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Critical, error);
    return;
  } else {
    if (!query.exec("CREATE UNIQUE INDEX attributeKey ON xmlattributes( "
                    "attribute, associatedElement)")) {
      QString error =
          QString(
              "Failed to create unique index for \"%1\": [%2]")
              .arg(m_db.connectionName())
              .arg(query.lastError().text());
      emit dbActionStatus(ActionStatus::Critical, error);
      return;
    }
  }

  if (!query.exec("CREATE TABLE rootelements( root QString primary key )")) {
    QString error = QString("Failed to create root elements "
                            "table for \"%1\": [%2]")
                        .arg(m_db.connectionName())
                        .arg(query.lastError().text());
    emit dbActionStatus(ActionStatus::Failed, error);
  }
}

/*----------------------------------------------------------------------------*/
