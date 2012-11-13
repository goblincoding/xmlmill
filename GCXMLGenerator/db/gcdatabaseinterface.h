#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>
#include <QMap>
#include <QtSql/QSqlQuery>

/*--------------------------------------------------------------------------------------*/
/*
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
  explicit GCDataBaseInterface( QObject *parent = 0 );

  /* Read in the list of known DB connections and add them to the SQLite driver.  This
    function must be called shortly after an instance of this object is created. */
  bool initialise();

  /* Processes an entire DOM document, adding new or updating existing elements (with their
    corresponding first level children and associated attributes) and known attribute values. */
  bool batchProcessDOMDocument( const QDomDocument *domDoc ) const;

  /* Adds a single new element (this function does nothing if an element with the same name
    already exists). */
  bool addElement( const QString &element, const QStringList &children, const QStringList &attributes ) const;

  /* Marks an element as a known document root element. This function does nothing if the root
    already exists in the relevant table. */
  bool addRootElement( const QString &root ) const;

  /* Updates the list of known children associated with "element" by appending
    the new children to the existing list (nothing is deleted). */
  bool updateElementChildren( const QString &element, const QStringList &children ) const;

  /* Updates the list of known attributes associated with "element" by appending
    the new attributes to the existing list (nothing is deleted). */
  bool updateElementAttributes( const QString &element, const QStringList &attributes ) const;

  /* Updates the list of known attribute values that is associated with "element" and its
    corresponding "attribute" by appending the new attribute values to the existing list
    (nothing is deleted). */
  bool updateAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const;

  /* Remove single entries/values from the database. */
  bool removeElement         ( const QString &element ) const;
  bool removeElementChild    ( const QString &element, const QString &children ) const;
  bool removeElementAttribute( const QString &element, const QString &attribute ) const;
  bool removeAttributeValue  ( const QString &element, const QString &attribute, const QString &attributeValue ) const;
  bool removeRootElement     ( const QString &element );

  /* Getters. */
  QStringList getDBList() const;    // returns a list of known DB connections
  QString  getLastError() const;    // returns the last error message set
  bool hasActiveSession() const;    // returns "true" if an active DB session exists, "false" if not

  /* Returns a sorted (case sensitive, ascending) list of all the element names known to
    the current database connection (the active session). */
  QStringList knownElements() const;

  /* Returns a sorted (case sensitive, ascending) list of all the children associated with
    "element" in the active database. */
  QStringList children( const QString &element, bool &success ) const;

  /* Returns a sorted (case sensitive, ascending) list of all the attributes associated with
    "element" in the active database. */
  QStringList attributes( const QString &element, bool &success ) const;

  /* Returns a sorted (case sensitive, ascending) list of all the attribute values associated with
    "element" and its corresponding "attribute" in the active database. */
  QStringList attributeValues( const QString &element, const QString &attribute, bool &success ) const;

  /* Returns a sorted (case sensitive, ascending) list of all the document root elements
    known to the the active database. */
  QStringList knownRootElements() const;
  
public slots:
  bool addDatabase   ( const QString &dbName );
  bool removeDatabase( const QString &dbName );
  bool setSessionDB  ( const QString &dbName );

private:
  QStringList knownAttributeKeys() const;
  QSqlQuery selectElement  ( const QString &element, bool &success ) const;
  QSqlQuery selectAttribute( const QString &attribute, const QString &associatedElement, bool &success ) const;

  /* After batch processing a DOM document, we concatenate new values to existing values
    in the record fields.  This function removes all duplicates that may have been
    introduced in this way by consolidating the values and updating the records. */
  bool removeDuplicatesFromFields() const;

  bool openDBConnection( const QString &dbConName );
  bool createDBTables() const;
  void saveDBFile() const;

  QSqlDatabase    m_sessionDB;
  mutable QString m_lastErrorMsg;
  bool            m_hasActiveSession;
  QMap< QString /*connection name*/, QString /*file name*/ > m_dbMap;
};

#endif // GCDATABASEINTERFACE_H
