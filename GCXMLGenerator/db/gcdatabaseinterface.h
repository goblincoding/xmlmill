/* Copyright (c) 2012 by William Hallatt.
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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>
#include <QMap>
#include <QtSql/QSqlQuery>

/// This class manages and interfaces with the SQLite databases used to profile XML documents.

/**
  This class is designed to set up and manage embedded SQLite databases used to profile
  XML documents.  Databases created by this class will consist of three tables:

    * "xmlelements"   - accepts element names as unique primary keys and associates two
                        fields with each record: "children" represents all the first level
                        children of the element in question and "attributes" contain all the
                        attributes known to be associated with the element (these will be ALL
                        the children and attribute names ever associated with any particular
                        unique element name in any particular database so it is best not to mix
                        vastly different XML profiles in the same database).

    * "xmlattributes" - accepts an attribute name as foreign key referencing the unique element it
                        is known to be associated with.  Only one additional field exists for each
                        record: "attributevalues" contains all the values ever associated with this
                        particular attribute when assigned to the specific element it references
                        as foreign key.  In other words, if element "x" is known to have had attribute
                        "y" associated with it, then "attributevalues" will contain all the values
                        ever assigned to "y" when associated with "x" across all XML profiles stored
                        in a particular database.

    * "rootelements"  - consists of a single field containing all known root elements stored in a
                        specific database.  If more than one XML profile has been loaded into the
                        database in question, the database will have all their root elements listed
                        in this table.
*/

class QDomDocument;

class GCDataBaseInterface : public QObject
{
  Q_OBJECT

public:
  /*! Singleton accessor. */
  static GCDataBaseInterface* instance();

  /*! Call this function before using this interface for the first time to ensure that
      the known databases were initialised successfully. */
  bool initialised();

  /*! Batch process an entire DOM document.  This function processes an entire DOM document by 
      adding new or updating existing elements with their corresponding first level children 
      and associated attributes and known attribute values to the active database in batches. */
  bool batchProcessDOMDocument( const QDomDocument *domDoc ) const;

  /*! Adds a single new element to the active database. This function does nothing if an element with the same name
      already exists. 
      @param element - the unique element name
      @param children - a list of the element's first level child elements
      @param attributes - a list of all the element's associated attributes. */
  bool addElement( const QString &element, const QStringList &children, const QStringList &attributes ) const;

  /*! Marks an element as a known document root element. This function does nothing if the root
      already exists in the relevant table. */
  bool addRootElement( const QString &root ) const;

  /*! Updates the list of known first level children associated with "element" by appending
      the new children to the existing list (nothing is deleted). */
  bool updateElementChildren( const QString &element, const QStringList &children ) const;

  /*! Updates the list of known attributes associated with "element" by appending
      the new attributes to the existing list (nothing is deleted). */
  bool updateElementAttributes( const QString &element, const QStringList &attributes ) const;

  /*! Updates the list of known attribute values that is associated with "element" and its
      corresponding "attribute" by appending the new attribute values to the existing list
      (nothing is deleted). */
  bool updateAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const;

  /*! Updates the list of known attribute values that is associated with "element" and its
      corresponding "attribute" by replacing all the existing values with those from the new list. */
  bool replaceAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const;

  /*! Removes "element" from the active database. */
  bool removeElement( const QString &element ) const;

  /*! Removes "element" from the list of known root elements for the active database. */
  bool removeRootElement( const QString &element ) const;

  /*! Removes "child" from the list of first level element children associated with "element" 
      in the active database. */
  bool removeChildElement( const QString &element, const QString &child ) const;

  /*! Removes "attribute" from the list of attributes associated with "element" 
      in the active database. */
  bool removeAttribute   ( const QString &element, const QString &attribute ) const;

  /*! Returns a list of known database connections. */
  QStringList getDBList() const;

  /*! Returns the last known error message. */
  QString getLastError() const;

  /*! Returns "true" if an active database session exists, "false" if not. */
  bool hasActiveSession() const;

  /*! Returns "true" if the active database is empty, "false" if not. */
  bool profileEmpty() const;

  /*! Returns a sorted (case sensitive, ascending) list of all the element names known to
      the current database connection (the active session). */
  QStringList knownElements() const;

  /*! Returns a sorted (case sensitive, ascending) list of all the children associated with
      "element" in the active database. */
  QStringList children( const QString &element, bool &success ) const;

  /*! Returns an UNSORTED list of all the attributes associated with "element" in the active
      database (the reason this list is unsorted is that all the other lists are used to populate 
      combo boxes, where ordering makes sense, but this particular list is used to populate a table). */
  QStringList attributes( const QString &element, bool &success ) const;

  /*! Returns a sorted (case sensitive, ascending) list of all the attribute values associated with
      "element" and its corresponding "attribute" in the active database. */
  QStringList attributeValues( const QString &element, const QString &attribute, bool &success ) const;

  /*! Returns a sorted (case sensitive, ascending) list of all the document root elements
      known to the the active database. */
  QStringList knownRootElements() const;

  /*! Returns true if the database named "dbName" knows about "root". */
  bool containsKnownRootElement( const QString &dbName, const QString &root ) const;
  
public slots:
  /*! Sets the database corresponding to "dbName" as the active database. */
  bool setActiveDatabase( const QString &dbName );

  /*! Adds "dbName" to the list of known database connections. */
  bool addDatabase( const QString &dbName );

  /*! Removes "dbName" from the list of known database connections. */
  bool removeDatabase( const QString &dbName );

private:
  static GCDataBaseInterface *m_instance;
  GCDataBaseInterface();

  QStringList knownAttributeKeys() const;
  QSqlQuery selectElement  ( const QString &element, bool &success ) const;
  QSqlQuery selectAttribute( const QString &attribute, const QString &associatedElement, bool &success ) const;

  /* Overloaded for private use. */
  QStringList knownRootElements( QSqlDatabase db ) const;

  /* After batch processing a DOM document, we concatenate new values to existing values
    in the record fields.  This function removes all duplicates that may have been
    introduced in this way by consolidating the values and updating the records. */
  bool removeDuplicatesFromFields() const;

  bool openConnection( const QString &dbConName );
  bool createTables() const;
  void saveDatabaseFile() const;

  QSqlDatabase    m_sessionDB;
  mutable QString m_lastErrorMsg;
  bool            m_hasActiveSession;
  bool            m_initialised;
  QMap< QString /*connection name*/, QString /*file name*/ > m_dbMap;
};

#endif // GCDATABASEINTERFACE_H
