#include "gcdatabaseinterface.h"
#include "gcbatchprocessorhelper.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

/*--------------------------------------------------------------------------------------*/

/* The database tables have fields containing strings of strings. For example, the
  "xmlelements" table maps a unique element against a list of all associated attribute
  values.  Since these values have to be entered into a single record, the easiest way
  is to insert a single (possibly massive) string containing all the associated attributes.
  To ensure that we can later extract the individual attributes again, we separate them with
  a sequence that should (theoretically) never be encountered.  This is that sequence. */
static const QString SEPARATOR( "~!@" );

/*--------------------------------------------------------------------------------------*/

/* Have a look at "createDBTables" to see how the DB is set up. */
static const QLatin1String INSERT_ELEMENT(
    "INSERT INTO xmlelements( element, children, attributes ) VALUES( ?, ?, ? )" );

static const QLatin1String DELETE_ELEMENT(
    "DELETE FROM xmlelements WHERE element = ?" );

static const QLatin1String INSERT_ATTRIBUTEVALUES(
    "INSERT INTO xmlattributes( attribute, associatedElement, attributeValues ) VALUES( ?, ?, ? )" );

static const QLatin1String DELETE_ATTRIBUTEVALUES(
    "DELETE FROM xmlattributes WHERE attribute = ? AND associatedElement = ?" );

/* Concatenate the new values to the existing values. The first '?' represents our string SEPARATOR. */
static const QLatin1String UPDATE_CHILDREN_CONCAT(
    "UPDATE xmlelements SET children   = ( children || ? || ? )   WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTES_CONCAT(
    "UPDATE xmlelements SET attributes = ( attributes || ? || ? ) WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTEVALUES_CONCAT(
    "UPDATE xmlattributes SET attributeValues = ( attributeValues || ? || ? ) WHERE attribute = ? AND associatedElement = ?" );

static const QLatin1String UPDATE_CHILDREN(
    "UPDATE xmlelements SET children   = ? WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTES(
    "UPDATE xmlelements SET attributes = ? WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTEVALUES(
    "UPDATE xmlattributes SET attributeValues = ? WHERE attribute = ? AND associatedElement = ?" );


//static const QLatin1String UPSERT_ELEMENT ( " CASE EXISTS (SELECT * FROM xmlelements WHERE element = ? ) "
//                                            " THEN "
//                                            "   UPDATE xmlelements SET children = CONCAT( children, ? ), attributes = CONCAT( attributes, ? ) WHERE element = ? "
//                                            " ELSE"
//                                            "   INSERT INTO xmlelements( element, children, attributes ) VALUES( ?, ?, ? ) "
//                                            " END" );



/*--------------------------------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );


/*--------------------------- NON-MEMBER UTILITY FUNCTIONS ----------------------------*/

void cleanList( QStringList &list )
{
  list.removeDuplicates();
  list.removeAll( "" );
}

/*--------------------------------------------------------------------------------------*/

QString cleanAndJoinListElements( QStringList list )
{
  cleanList( list );
  return list.join( SEPARATOR );
}

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCDataBaseInterface::GCDataBaseInterface( QObject *parent ) :
  QObject           ( parent ),
  m_sessionDB       (),
  m_lastErrorMsg    ( "" ),
  m_hasActiveSession( false ),
  m_dbMap           ()
{
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::initialise()
{
  QFile flatFile( DB_FILE );

  /* ReadWrite mode is required to create the file if it doesn't exist. */
  if( flatFile.open( QIODevice::ReadWrite | QIODevice::Text ) )
  {
    QTextStream inStream( &flatFile );
    QString fileContent = inStream.readAll();
    flatFile.close();

    /* Split the input into separate lines (path/to/file lines). */
    QStringList list = fileContent.split( "\n", QString::SkipEmptyParts );

    foreach( QString str, list )
    {
      if( !addDatabase( str ) )
      {
        m_lastErrorMsg = QString( "Failed to load existing connection: \n %1" ).arg( str );
        return false;
      }
    }

    m_lastErrorMsg = "";
    return true;
  }

  m_lastErrorMsg = QString( "Failed to access list of databases, file open error: [%1]." ).arg( flatFile.errorString() );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::addDatabase( const QString &dbName )
{
  if( !dbName.isEmpty() )
  {
    /* The DB name passed in will most probably consist of a path/to/file string. */
    QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

    if( !m_dbMap.contains( dbConName ) )
    {
      QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", dbConName );

      if( db.isValid() )
      {
        db.setDatabaseName( dbName );
        m_dbMap.insert( dbConName, dbName );
        saveDBFile();

        m_lastErrorMsg = "";
        return true;
      }

      m_lastErrorMsg = QString( "Failed to add database \"%1\": [%2]." ).arg( dbConName ).arg( db.lastError().text() );
      return false;
    }
    else
    {
      m_lastErrorMsg = QString( "Connection \"%1\" already exists." ).arg( dbConName );
      return false;
    }
  }

  m_lastErrorMsg = QString( "Database name is empty." );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeDatabase( const QString &dbName )
{
  if( !dbName.isEmpty() )
  {
    /* The DB name passed in will most probably consist of a path/to/file string. */
    QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

    if( m_sessionDB.connectionName() == dbConName )
    {
      m_sessionDB.close();
      m_hasActiveSession = false;
    }

    QFile file( m_dbMap.value( dbConName ) );

    if( !file.remove() )
    {
      m_lastErrorMsg = QString( "Failed to remove database file: [%1]")
          .arg( dbName );
      return false;
    }

    /* If the DB connection being removed was also the active one, "removeDatabase" will output
      a warning.  This is purely because we have a DB member variable and isn't cause for concern
      as there seems to be no way around it with the current QtSQL modules. */
    QSqlDatabase::removeDatabase( dbConName );
    m_dbMap.remove( dbConName );
    saveDBFile();

    m_lastErrorMsg = "";
    return true;
  }

  m_lastErrorMsg = QString( "Database name is empty." );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::setSessionDB( const QString &dbName )
{
  /* The DB name passed in will most probably consist of a path/to/file string. */
  QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

  if( QSqlDatabase::contains( dbConName ) )
  {
    if( openDBConnection( dbConName ) )
    {
      m_hasActiveSession = true;
      return true;
    }
  }
  else
  {
    /* If we set a DB for the session that doesn't exist (new, unknown),
      then we'll automatically try to add it and set it as active. */
    if( addDatabase( dbName ) )
    {
      if( openDBConnection( dbConName ) )
      {
        m_hasActiveSession = true;
        return true;
      }
    }

    m_hasActiveSession = false;
    return false;
  }

  m_hasActiveSession = false;
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::openDBConnection( const QString &dbConName )
{
  /* If we have a previous connection open, close it. */
  if( m_sessionDB.isValid() && m_sessionDB.isOpen() )
  {
    m_sessionDB.close();
  }

  /* Open the new connection. */
  m_sessionDB = QSqlDatabase::database( dbConName );

  if( m_sessionDB.isValid() )
  {
    if ( !m_sessionDB.open() )
    {
      m_lastErrorMsg = QString( "Failed to open database \"%1\": [%2]." )
          .arg( m_dbMap.value( dbConName ) )
          .arg( m_sessionDB.lastError().text() );
      return false;
    }

    /* If the DB has not yet been initialised. */
    QStringList tables = m_sessionDB.tables();

    if ( !tables.contains( "xmlelements", Qt::CaseInsensitive ) )
    {
      return createDBTables();
    }
  }
  else
  {
    m_lastErrorMsg = QString( "Failed to open a valid session connection \"%1\": [%2]" )
        .arg( dbConName )
        .arg( m_sessionDB.lastError().text() );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::createDBTables() const
{
  /* DB connection will be open from openDBConnection() above so no need
    to do any checks here. */
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "CREATE TABLE xmlelements( element QString primary key, children QString, attributes QString )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create elements table for \"%1\": [%2]." )
        .arg( m_sessionDB.connectionName() )
        .arg( query.lastError().text() );
    return false;
  }

  if( !query.exec( "CREATE TABLE xmlattributes( attribute QString, associatedElement QString, attributeValues QString, "
                   "UNIQUE(attribute, associatedElement), "
                   "FOREIGN KEY(associatedElement) REFERENCES xmlelements(element) )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create attribute values table for \"%1\": [%2]" )
        .arg( m_sessionDB.connectionName() )
        .arg( query.lastError().text() );
    return false;
  }
  else
  {
    if( !query.exec( "CREATE UNIQUE INDEX attributeKey ON xmlattributes( attribute, associatedElement)") )
    {
      m_lastErrorMsg = QString( "Failed to create unique index for \"%1\": [%2]" )
          .arg( m_sessionDB.connectionName() )
          .arg( query.lastError().text() );
      return false;
    }
  }

  if( !query.exec( "CREATE TABLE rootelements( root QString primary key )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create root elements table for \"%1\": [%2]" )
        .arg( m_sessionDB.connectionName() )
        .arg( query.lastError().text() );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::batchProcessDOMDocument( const QDomDocument *domDoc ) const
{
  GCBatchProcessorHelper helper( domDoc, SEPARATOR );
  helper.setKnownElements  ( knownElements() );
  helper.setKnownAttributes( knownAttributeKeys() );
  helper.createVariantLists();

  /* Insert the document root element into the rootelements table. */
  if( !addRootElement( domDoc->documentElement().tagName() ) )
  {
    /* Last error message is set in "addRootElement". */
    return false;
  }

  QSqlQuery query( m_sessionDB );

  /* Batch insert all the new elements. */
  if( !query.prepare( INSERT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch INSERT elements failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( helper.newElementsToAdd() );
  query.addBindValue( helper.newElementChildrenToAdd() );
  query.addBindValue( helper.newElementAttributesToAdd() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch INSERT elements failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  /* Batch update all the existing elements. */
  if( !query.prepare( UPDATE_CHILDREN_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE element children failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  /* Since we're doing batch updates we need to ensure that all the variant lists we provide
    have exactly the same size.  We furthermore require that the concatenation of new and old values
    are done in a way that includes our SEPARATOR string (which is why the separator list below). */
  QVariantList separatorList;

  for( int i = 0; i < helper.elementsToUpdate().size(); ++i )
  {
    separatorList << SEPARATOR;
  }

  query.addBindValue( separatorList );
  query.addBindValue( helper.elementChildrenToUpdate() );
  query.addBindValue( helper.elementsToUpdate() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE element children failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  if( !query.prepare( UPDATE_ATTRIBUTES_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE element attributes failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( separatorList );
  query.addBindValue( helper.elementAttributesToUpdate() );
  query.addBindValue( helper.elementsToUpdate() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE element attributes failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  /* Batch insert all the new attribute values. */
  if( !query.prepare( INSERT_ATTRIBUTEVALUES ) )
  {
    m_lastErrorMsg = QString( "Prepare batch INSERT attribute values failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( helper.newAttributeKeysToAdd() );
  query.addBindValue( helper.newAssociatedElementsToAdd() );
  query.addBindValue( helper.newAttributeValuesToAdd() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch INSERT attribute values failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  /* Batch update all the existing attribute values. */
  if( !query.prepare( UPDATE_ATTRIBUTEVALUES_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE attribute values failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  separatorList.clear();

  for( int i = 0; i < helper.attributeKeysToUpdate().size(); ++i )
  {
    separatorList << SEPARATOR;
  }

  query.addBindValue( separatorList );
  query.addBindValue( helper.attributeValuesToUpdate() );
  query.addBindValue( helper.attributeKeysToUpdate() );
  query.addBindValue( helper.associatedElementsToUpdate() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE attribute values failed: [%1]" )
        .arg( query.lastError().text() );
    return false;
  }

  return removeDuplicatesFromFields();
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeDuplicatesFromFields() const
{
  bool success( false );

  /* Remove duplicates and update the element records. */
  QStringList elementNames = knownElements();
  QString element( "" );

  for( int i = 0; i < elementNames.size(); ++i )
  {
    element = elementNames.at( i );
    QSqlQuery query = selectElement( element, success );

    /* Check that we are working with a valid query. */
    if( success )
    {
      /* Does a record for this element exist? */
      if( query.first() )
      {
        QStringList allChildren  ( query.record().field( "children" ).value().toString().split( SEPARATOR ) );
        QStringList allAttributes( query.record().field( "attributes" ).value().toString().split( SEPARATOR ) );

        if( !query.prepare( UPDATE_CHILDREN ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE element children failed for element \"%1\": [%2]" )
              .arg( element )
              .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( cleanAndJoinListElements( allChildren ) );
        query.addBindValue( element );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE children failed for element \"%1\": [%2]" )
              .arg( element )
              .arg( query.lastError().text() );
          return false;
        }

        if( !query.prepare( UPDATE_ATTRIBUTES ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE element attributes failed for element \"%1\": [%2]" )
              .arg( element )
              .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( cleanAndJoinListElements( allAttributes ) );
        query.addBindValue( element );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE attributes failed for element \"%1\": [%2]" )
              .arg( element )
              .arg( query.lastError().text() );
          return false;
        }
      }
    }
    else
    {
      /* Last error message set in selectElement(). */
      return false;
    }
  }

  /* Remove duplicates and update the attribute values records. */
  QStringList attributeKeys = knownAttributeKeys();

  for( int i = 0; i < attributeKeys.size(); ++i )
  {
    QString attribute         = attributeKeys.at( i ).split( "!" ).at( 0 );
    QString associatedElement = attributeKeys.at( i ).split( "!" ).at( 1 );
    QSqlQuery query = selectAttribute( attribute, associatedElement, success );

    /* Check that we are working with a valid query. */
    if( success )
    {
      /* Does a record for this attribute exist? */
      if( query.first() )
      {
        QStringList allValues( query.record().field( "attributeValues" ).value().toString().split( SEPARATOR ) );

        if( !query.prepare( UPDATE_ATTRIBUTEVALUES ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE attribute values failed for element \"%1\" and attribute \"%2\": [%3]" )
              .arg( associatedElement )
              .arg( attribute )
              .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( cleanAndJoinListElements( allValues ) );
        query.addBindValue( attribute );
        query.addBindValue( associatedElement );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE attribute values failed for element \"%1\" and attribute \"%2\": [%3]" )
              .arg( associatedElement )
              .arg( attribute )
              .arg( query.lastError().text() );
          return false;
        }
      }
    }
    else
    {
      /* Last error message set in selectElement(). */
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectElement( const QString &element, bool &success ) const
{
  /* See if we already have this element in the DB. */
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM xmlelements WHERE element = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT failed for element \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
    success = false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
    success = false;
  }

  m_lastErrorMsg = "";
  success = true;
  return query;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::addElement( const QString &element, const QStringList &children, const QStringList &attributes ) const
{
  bool success( false );
  QSqlQuery query = selectElement( element, success );

  if( !success )
  {
    /* The last error message has been set in selectElement. */
    return false;
  }

  /* If we don't have an existing record, add it. */
  if( !query.first() )
  {
    if( !query.prepare( INSERT_ELEMENT ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT element failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( element );
    query.addBindValue( cleanAndJoinListElements( children ) );
    query.addBindValue( cleanAndJoinListElements( attributes ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT element failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::addRootElement( const QString &root ) const
{
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM rootelements WHERE root = ? ") )
  {
    m_lastErrorMsg = QString( "Prepare SELECT root element failed for root \"%1\": [%2]" )
        .arg( root )
        .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( root );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT root element failed for root \"%1\": [%2]" )
        .arg( root )
        .arg( query.lastError().text() );
    return false;
  }

  /* Make sure we aren't trying to insert a known root element. */
  if( !query.first() )
  {
    if( !query.prepare( "INSERT INTO rootelements ( root ) VALUES( ? )" ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT root element failed for root \"%1\": [%2]" )
          .arg( root )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( root );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT root element failed for element \"%1\": [%2]" )
          .arg( root )
          .arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::updateElementChildren( const QString &element, const QStringList &children ) const
{
  bool success( false );
  QSqlQuery query = selectElement( element, success );

  if( !success )
  {
    /* The last error message has been set in selectElement. */
    return false;
  }

  /* Update the existing record (if we have one). */
  if( query.first() )
  {
    QStringList allChildren( query.record().field( "children" ).value().toString().split( SEPARATOR ) );
    allChildren.append( children );

    if( !query.prepare( UPDATE_CHILDREN ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element comment failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( cleanAndJoinListElements( allChildren ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE comment failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    m_lastErrorMsg = QString( "No knowledge of element \"%1\", add it first." )
        .arg( element );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::updateElementAttributes( const QString &element, const QStringList &attributes ) const
{
  bool success( false );
  QSqlQuery query = selectElement( element, success );

  if( !success )
  {
    /* The last error message has been set in selectElement. */
    return false;
  }

  /* Update the existing record (if we have one). */
  if( query.first() )
  {
    QStringList allAttributes( query.record().field( "attributes" ).value().toString().split( SEPARATOR ) );
    allAttributes.append( attributes );

    if( !query.prepare( UPDATE_ATTRIBUTES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element attribute failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( cleanAndJoinListElements( allAttributes ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    m_lastErrorMsg = QString( "No element \"%1\" exists." )
        .arg( element );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAttribute( const QString &attribute, const QString &associatedElement, bool &success ) const
{
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM xmlattributes WHERE attribute = ? AND associatedElement = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT attribute failed for attribute \"%1\" and element \"%2\": [%3]" )
        .arg( attribute )
        .arg( associatedElement )
        .arg( query.lastError().text() );
    success = false;
  }

  query.addBindValue( attribute );
  query.addBindValue( associatedElement );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT attribute failed for attribute \"%1\" and element \"%2\": [%3]" )
        .arg( attribute )
        .arg( associatedElement )
        .arg( query.lastError().text() );
    success = false;
  }

  success = true;
  return query;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::updateAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const
{
  bool success( false );
  QSqlQuery query = selectAttribute( attribute, element, success );

  if( !success )
  {
    /* The last error message has been set in selectElement. */
    return false;
  }

  /* If we don't have an existing record, add it, otherwise update the existing one. */
  if( !query.first() )
  {
    if( !query.prepare( INSERT_ATTRIBUTEVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT attribute value failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( attribute );
    query.addBindValue( element );
    query.addBindValue( cleanAndJoinListElements( attributeValues ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT attribute failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    QStringList existingValues( query.record().field( "attributeValues" ).value().toString().split( SEPARATOR ) );
    existingValues.append( attributeValues );

    if( !query.prepare( UPDATE_ATTRIBUTEVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE attribute values failed for element \"%1\" and attribute \"%2\": [%3]" )
          .arg( element )
          .arg( attribute )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( cleanAndJoinListElements( existingValues ) );
    query.addBindValue( attribute );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute values failed for element \"%1\" and attribute [%2]: [%3]" )
          .arg( element )
          .arg( attribute )
          .arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeElement( const QString &element ) const
{
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeElementChild( const QString &element, const QString &children ) const
{
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeElementAttribute( const QString &element, const QString &attribute ) const
{
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeAttributeValue( const QString &element, const QString &attribute, const QString &attributeValue ) const
{
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeRootElement( const QString &element )
{
  return true;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownElements() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM xmlelements" ) )
  {
    m_lastErrorMsg = QString( "SELECT all elements failed: [%1]" )
        .arg( query.lastError().text() );
    return QStringList();
  }

  QStringList elementNames;

  while( query.next() )
  {
    elementNames.append( query.record().field( "element" ).value().toString() );
  }

  cleanList( elementNames );
  elementNames.sort();
  return elementNames;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownAttributeKeys() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM xmlattributes" ) )
  {
    m_lastErrorMsg = QString( "SELECT all attribute values failed: [%1]" )
        .arg( query.lastError().text() );
    return QStringList();
  }

  QStringList attributeNames;

  while( query.next() )
  {
    /* Concatenate the attribute name and associated element into a single string
      so that it is easier to determine whether a record already exists for that
      particular combination. */
    attributeNames.append( query.record().field( "attribute" ).value().toString() +
                           "!" +
                           query.record().field( "associatedElement" ).value().toString());
  }

  return attributeNames;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownRootElements() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM rootelements" ) )
  {
    m_lastErrorMsg = QString( "SELECT all root elements failed: [%1]" )
        .arg( query.lastError().text() );
    return QStringList();
  }

  QStringList rootElements;

  while( query.next() )
  {
    rootElements.append( query.record().field( "root" ).value().toString() );
  }

  cleanList( rootElements );
  return rootElements;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::children( const QString &element, bool &success ) const
{
  QSqlQuery query = selectElement( element, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of children for element \"%1\"" )
        .arg( element );
    return QStringList();
  }

  QStringList children = query.record().value( "children" ).toString().split( SEPARATOR );
  cleanList( children );
  children.sort();
  return children;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributes( const QString &element, bool &success ) const
{
  QSqlQuery query = selectElement( element, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of attributes for element \"%1\"" )
        .arg( element );
    return QStringList();
  }

  QStringList attributes = query.record().value( "attributes" ).toString().split( SEPARATOR );
  cleanList( attributes );
  return attributes;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributeValues( const QString &element, const QString &attribute, bool &success ) const
{
  QSqlQuery query = selectAttribute( attribute, element, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of attribute values for attribute \"%1\"" )
        .arg( attribute );
    return QStringList();
  }

  QStringList attributeValues = query.record().value( "attributeValues" ).toString().split( SEPARATOR );
  cleanList( attributeValues );
  attributeValues.sort();
  return attributeValues;
}

/*--------------------------------------------------------------------------------------*/

void GCDataBaseInterface::saveDBFile() const
{
  QFile flatFile( DB_FILE );

  if( flatFile.open( QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate ) )
  {
    QTextStream outStream( &flatFile );

    foreach( QString str, m_dbMap.values() )
    {
      outStream << str << "\n";
    }

    flatFile.close();
  }
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::getDBList() const
{
  return m_dbMap.keys();
}

/*--------------------------------------------------------------------------------------*/

QString GCDataBaseInterface::getLastError() const
{
  return m_lastErrorMsg;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::hasActiveSession() const
{
  return m_hasActiveSession;
}

/*--------------------------------------------------------------------------------------*/
