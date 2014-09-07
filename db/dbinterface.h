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

#ifndef DATABASEINTERFACE_H
#define DATABASEINTERFACE_H

#include <QObject>
#include <QtSql/QSqlQuery>

/** Provides an interface to the SQLite database used to profile XML
  documents. This database consists of three tables:

    * "xmlelements"   - accepts an element name as primary key and references
  the unique root element it is known to be associated with as foreign key.
  Two fields are associates with each record: "children" represents all the
  first level children of the element in question and "attributes"
  contain all the attributes known to be associated with the element
  (these will be ALL the children and attribute names ever associated with
  any particular unique element name in any particular database so it is
  best not to mix vastly different XML profiles in the same database).

    * "xmlattributes" - accepts an attribute name as primary key and references
  the unique element it is known to be associated with as foreign key.  Only one
  additional field exists for each record: "attributevalues" contains all the
  values ever associated with this particular attribute when assigned to the
  specific element it references as foreign key.  In other words, if element "x"
  is known to have had attribute "y" associated with it, then "attributevalues"
  will contain all the values ever assigned to "y" when associated with "x"
  across all XML profiles stored in a particular database.

    * "rootelements" - consists of a single field containing all known root
  elements (representing unique document styles/types). */

class QDomDocument;
class BatchProcessHelper;

class DB : public QObject {
  Q_OBJECT

public:
  /*! Constructor. */
  DB();

  /*! There is no need for this class to be copyable. */
  DB(const DB &) = delete;

  /*! There is no need for this class to be assignable. */
  DB &operator=(const DB &) = delete;

  /*! Used to communicate DB errors via the \sa dbActionStatus signal. */
  enum class ActionStatus {
    Failed,  /*!< Generally used to communicate DB query failures. */
    Critical /*!< Used to communinicate DB creation failures. */
  };

  /*! Batch process an entire DOM document. This function processes an entire
     DOM document by adding new (or updating existing) elements with their
     corresponding first level children and associated attributes and known
     attribute values to the active database in batches.
     \sa batchProcessNewElements
     \sa batchUpdateExistingElementChildren
     \sa batchUpdateExistingElementAttributes
     \sa batchProcessNewAttributeValues
     \sa batchUpdateExistingAttributeValues */
  void batchProcessDomDocument(const QDomDocument *domDoc);

  /*! Marks an element as a known document root element. This function does
     nothing if the root already exists in the relevant table.
     @param root - the name of the unique root element */
  void addRootElement(const QString &root);

  /*! Adds a single new element to the active database. This function does
     nothing if an element with the same name already exists.
     @param root - the root element of the associated document type
     @param element - the unique element name
     @param children - a list of the element's first level child elements'
     names
     @param attributes - a list of all the element's associated attribute
     names. */
  void addElement(const QString &root, const QString &element,
                  const QStringList &children, const QStringList &attributes);

  /*! Updates the list of known first level children associated with "element"
     by appending the new children to the existing list (nothing is deleted). If
     "replace" is true, the existing values are replaced by those in the
     parameter list.
     @param root - the root element of the associated document type
     @param element - the unique name of the element to be updated
     @param children - a list of the element's first level child elements'
     names
     @param replace - if true, the child list is replaced, if false, "children"
     is merged with the existing list. */
  void updateElementChildren(const QString &root, const QString &element,
                             const QStringList &children, bool replace = false);

  /*! Updates the list of known attributes associated with "element" by
     appending the new attributes to the existing list (nothing is deleted). If
     "replace" is true, the existing values are replaced by those in the
     parameter list.
     @param root - the root element of the associated document type
     @param element - the unique name of the element to be updated
     @param attributes - a list of the attribute names associated with the
     element
     @param replace - if true, the attribute list is replaced, if false,
     "attributes" is merged with the existing list. */
  void updateElementAttributes(const QString &root, const QString &element,
                               const QStringList &attributes,
                               bool replace = false);

  /*! Updates the list of known attribute values that is associated with
     "element" and its corresponding "attribute" by appending the new attribute
     values to the existing list (nothing is deleted). If "replace" is true, the
     existing values are replaced by those in the parameter list
     @param element - the unique name of the element to be updated
     @param attribute - the name of the associated attribute to te updated
     @param attributeValues - a list of the attribute values associated with
     the attribute
     @param replace - if true, the attribute value list is replaced, if false,
     "attributeValues" is merged with the existing list. */
  void updateAttributeValues(const QString &element, const QString &attribute,
                             const QStringList &attributeValues,
                             bool replace = false) const;

  /*! Removes "element" from the active database. */
  void removeElement(const QString &root, const QString &element);

  /*! Removes "element" from the list of known root elements for the active
   * database. */
  void removeRootElement(const QString &element) const;

  /*! Removes "child" from the list of first level element children associated
     with "element" in the active database. */
  void removeChildElement(const QString &root, const QString &element,
                          const QString &child);

  /*! Removes "attribute" from the list of attributes associated with "element"
      in the active database. */
  void removeAttribute(const QString &root, const QString &element, const QString &attribute);

  /*! Returns "true" if the active database is empty, "false" if not. */
  bool isProfileEmpty() const;

  /*! Returns "true" if the database named "dbName" knows about "root". */
  bool hasRootElement(const QString &root) const;

  /*! Returns true if "element" is a child of "parentElement" only (i.e. it
     doesn't exist in any other first level child list). */
  bool isUniqueChildElement(const QString &root, const QString &parentElement,
                            const QString &element) const;

  /*! Recursively scans the "doc"'s element hierarchy to ensure that all the
     document's elements, element relationships and attributes are known to the
     active profile. */
  bool isDocumentCompatible(const QDomDocument *doc);

  /*! Returns a sorted (case sensitive, ascending) list of all the element names
     known to the current database connection (the active session) associated
     with "root" (the document "type"). */
  QStringList knownElements(const QString &root) const;

  /*! Returns a sorted (case sensitive, ascending) list of all the first level
     children associated with "element" in the active database, or an empty
     QStringList if unsuccessful/none exist. */
  QStringList children(const QString &root, const QString &element);

  /*! Returns an UNSORTED list of all the attribute names associated with
     "element" in the active database (the reason this list is unsorted is that
     all the other lists are used to populate combo boxes, where ordering makes
     sense, but this particular list is used to populate a table), or an empty
     QStringList if unsuccessful/none exist. */
  QStringList attributes(const QString &root, const QString &element);

  /*! Returns a sorted (case sensitive, ascending) list of all the attribute
     values associated with "element" and its corresponding "attribute" in the
     active database or, an empty QStringList if unsuccessful/none exist. */
  QStringList attributeValues(const QString &element,
                              const QString &attribute) const;

  /*! Returns a sorted (case sensitive, ascending) list of all the document root
     elements known to the the active database. */
  QStringList knownRootElements() const;

signals:
  void dbActionStatus(ActionStatus status, const QString &msg) const;

private:
  /*! Returns a list of known attributes. */
  QStringList knownAttributeKeys() const;

  /*! Selects "element" from the database.  The active query for the command is
     returned (the function does not care whether or not the record exists). */
  QSqlQuery selectElement(const QString &root, const QString &element);

  /*! Selects all the known elements from the database and returns the active
   * query. */
  QSqlQuery selectAllElements(const QString &root) const;

  /*! Selects the "attribute" corresponding to "associatedElement" from the
     database.  The active query for the command is returned (the function
     does not care whether or not the record exists). */
  QSqlQuery selectAttribute(const QString &attribute,
                            const QString &associatedElement) const;

  /*! Selects all the known attributes from the database and returns the active
   * query. */
  QSqlQuery selectAllAttributes() const;

  /*! Checks if DB is valid and open and creates an "empty" query. */
  QSqlQuery createQuery();

  /*! Removes all duplicates that may have been introduced during batch
     processing. After batch processing a DOM document, we concatenate new
     values to existing values in the record fields.  This function removes
     all duplicates that may have been introduced in this way by consolidating
     the values and updating the records. */
  void removeDuplicatesFromFields(const QString &root);

  void removeDuplicateElementChildren(QSqlQuery &query, const QString &root,
                                      const QString &element);
  void removeDuplicateElementAttributes(QSqlQuery &query, const QString &root,
                                        const QString &element);
  void removeDuplicateAttributeValues(QSqlQuery &query,
                                      const QString &associatedElement,
                                      const QString &attribute);

  /*! Opens the database connection corresponding to "dbConName".  This function
     will also close current sessions (if any) before opening the new one. */
  void openConnection();

  /*! Creates all the relevant database tables. */
  void createTables();

  void batchProcessNewElements(BatchProcessHelper &helper,
                               const QString &associatedRoot);
  void batchUpdateExistingElementChildren(BatchProcessHelper &helper,
                                          const QString &associatedRoot);
  void batchUpdateExistingElementAttributes(BatchProcessHelper &helper,
                                            const QString &associatedRoot);
  void batchProcessNewAttributeValues(BatchProcessHelper &helper);
  void batchUpdateExistingAttributeValues(BatchProcessHelper &helper);

private:
  QSqlDatabase m_db;
};

#endif // DATABASEINTERFACE_H
