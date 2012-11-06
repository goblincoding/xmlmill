#include "gcdatabaseinterface.h"
#include "gcbatchprocessorhelper.h"
#include "utils/gcglobals.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

/*--------------------------------------------------------------------------------------*/

/* DB tables are set up as follows:
  Table - XMLELEMENTS
  ELEMENT (Key) | CHILDREN (First Level Only) | ATTRIBUTES

  Table - XMLATTRIBUTES
  ATTRIBUTEKEY (concatenate element and attribute name) (Key) | ATTRIBUTEVALUES

  Table - ROOTELEMENTS
  ROOTS (Key, no other values associated with it for now)
*/

static const QLatin1String SELECT_ALL_ELEMENTS( "SELECT * FROM xmlelements" );
static const QLatin1String INSERT_ELEMENT     ( "INSERT INTO xmlelements( element, children, attributes ) VALUES( ?, ?, ? )" );
static const QLatin1String DELETE_ELEMENT     ( "DELETE FROM xmlelements WHERE element = ?" );

static const QLatin1String SELECT_ALL_ATTRIBUTEVALUES( "SELECT * FROM xmlattributevalues" );
static const QLatin1String INSERT_ATTRIBUTEVALUES    ( "INSERT INTO xmlattributevalues( attributeKey, attributeValues ) VALUES( ?, ? )" );
static const QLatin1String DELETE_ATTRIBUTEVALUES    ( "DELETE FROM xmlattributevalues WHERE attributeKey = ?" );

/* Concatenate the new values to the existing values. The first '?' represents our string SEPARATOR. */
static const QLatin1String UPDATE_CHILDREN_CONCAT        ( "UPDATE xmlelements SET children   = ( children || ? || ? )   WHERE element = ?" );
static const QLatin1String UPDATE_ATTRIBUTES_CONCAT      ( "UPDATE xmlelements SET attributes = ( attributes || ? || ? ) WHERE element = ?" );
static const QLatin1String UPDATE_ATTRIBUTEVALUES_CONCAT ( "UPDATE xmlattributevalues SET attributeValues = ( attributeValues || ? || ? ) WHERE attributeKey = ?" );

static const QLatin1String UPDATE_CHILDREN        ( "UPDATE xmlelements SET children   = ? WHERE element = ?" );
static const QLatin1String UPDATE_ATTRIBUTES      ( "UPDATE xmlelements SET attributes = ? WHERE element = ?" );
static const QLatin1String UPDATE_ATTRIBUTEVALUES ( "UPDATE xmlattributevalues SET attributeValues = ? WHERE attributeKey = ?" );


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
  list.sort();
  list.removeDuplicates();
  list.removeAll( "" );
}

/*--------------------------------------------------------------------------------------*/

QString joinListElements( QStringList list )
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
      /* Split the path/directory structure to use the file name as the key. */
      if( !addDatabase( str ) )
      {
        m_lastErrorMsg = QString( "Failed to load existing connection: \n %1" );
        return false;
      }
    }

    m_lastErrorMsg = "";
    return true;
  }

  m_lastErrorMsg = QString( "Failed to access list of databases, file open error - [%1]." ).arg( flatFile.errorString() );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::addDatabase( QString dbName )
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
      m_lastErrorMsg = QString( "Connection: \"%1\" already exists." ).arg( dbConName );
      return false;
    }
  }

  m_lastErrorMsg = QString( "Database name is empty." );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeDatabase( QString dbName )
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

bool GCDataBaseInterface::setSessionDB( QString dbName )
{
  /* The DB name passed in will most probably consist of a path/to/file string. */
  QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();
  QString previousConnectionName = m_sessionDB.connectionName();

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
    /* If we somehow tried to set a DB for the session that doesn't exist,
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

bool GCDataBaseInterface::openDBConnection( QString dbConName )
{
  /* If we have a previous connection open, close it. */
  if( m_sessionDB.isValid() )
  {
    m_sessionDB.close();
  }

  /* Open the new connection. */
  m_sessionDB = QSqlDatabase::database( dbConName );

  if( m_sessionDB.isValid() )
  {
    if ( !m_sessionDB.open() )
    {
      m_lastErrorMsg = QString( "Failed to open database \"%1\": [%2]." ).arg( m_dbMap.value( dbConName ) )
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
    m_lastErrorMsg = QString( "Failed to open a valid session connection \"%1\", error: %2" ).arg( dbConName );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::createDBTables() const
{
  /* DB connection should be open from openDBConnection() above. */
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "CREATE TABLE xmlelements( element QString primary key, children QString, attributes QString )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create elements table for \"%1\": [%2]." ).arg( m_sessionDB.connectionName() )
             .arg( query.lastError().text() );
    return false;
  }

  if( !query.exec( "CREATE TABLE xmlattributevalues( attributeKey QString primary key, attributeValues QString )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create attributes values table for \"%1\": [%2]" ).arg( m_sessionDB.connectionName() )
             .arg( query.lastError().text() );
    return false;
  }

  if( !query.exec( "CREATE TABLE rootelements( root QString primary key )" ) )
  {
    m_lastErrorMsg = QString( "Failed to create root elements table for \"%1\": [%2]" ).arg( m_sessionDB.connectionName() )
             .arg( query.lastError().text() );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::batchProcessDOMDocument( const QDomDocument &domDoc ) const
{
  GCBatchProcessorHelper helper( domDoc );
  helper.setKnownElements  ( knownElements() );
  helper.setKnownAttributes( knownAttributeKeys() );
  helper.createVariantLists();

  /* Insert the document root element into the rootelements table. */
  if( !addRootElement( domDoc.documentElement().tagName() ) )
  {
    /* Last error message is set in "addRootElement". */
    return false;
  }

  QSqlQuery query( m_sessionDB );

  /* Batch insert all the new elements. */
  if( !query.prepare( INSERT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch INSERT elements failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( helper.newElementsToAdd() );
  query.addBindValue( helper.newElementChildrenToAdd() );
  query.addBindValue( helper.newElementAttributesToAdd() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch INSERT elements failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  /* Batch update all the existing elements. */
  if( !query.prepare( UPDATE_CHILDREN_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE element children failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  /* Since we're doing batch updates, we need to ensure that all the variant lists we provide
    have exactly the same size.  We furthermore require that the concatenation of new to old values
    be separated by our SEPARATOR string, which is why we have a separator list below. */
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
    m_lastErrorMsg = QString( "Batch UPDATE element children failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  if( !query.prepare( UPDATE_ATTRIBUTES_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE element attributes failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( separatorList );
  query.addBindValue( helper.elementAttributesToUpdate() );
  query.addBindValue( helper.elementsToUpdate() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE element attributes failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  /* Batch insert all the new attribute values. */
  if( !query.prepare( INSERT_ATTRIBUTEVALUES ) )
  {
    m_lastErrorMsg = QString( "Prepare batch INSERT attribute values failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( helper.newAttributeKeysToAdd() );
  query.addBindValue( helper.newAttributeValuesToAdd() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch INSERT attribute values failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  /* Batch update all the existing attribute values. */
  if( !query.prepare( UPDATE_ATTRIBUTEVALUES_CONCAT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE attribute values failed - [%1]" ).arg( query.lastError().text() );
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

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE attribute values failed - [%1]" ).arg( query.lastError().text() );
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

    if( success )
    {
      if( query.first() )
      {
        QStringList allChildren  ( query.record().field( "children" ).value().toString().split( SEPARATOR ) );
        QStringList allAttributes( query.record().field( "attributes" ).value().toString().split( SEPARATOR ) );

        if( !query.prepare( UPDATE_CHILDREN ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE element children failed for element \"%1\" - [%2]" ).arg( element )
                           .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( joinListElements( allChildren ) );
        query.addBindValue( element );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE children failed for element \"%1\" - [%2]" ).arg( element )
                           .arg( query.lastError().text() );
          return false;
        }

        if( !query.prepare( UPDATE_ATTRIBUTES ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE element attributes failed for element \"%1\" - [%2]" ).arg( element )
                           .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( joinListElements( allAttributes ) );
        query.addBindValue( element );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE attributes failed for element \"%1\" - [%2]" ).arg( element )
                           .arg( query.lastError().text() );
          return false;
        }
      }
    }
    else
    {
      return false;
    }
  }

  /* Remove duplicates and update the attribute values records. */
  QStringList attributeKeys = knownAttributeKeys();

  for( int i = 0; i < attributeKeys.size(); ++i )
  {
    QString attributeKey = attributeKeys.at( i );
    QSqlQuery query = selectAttribute( attributeKey, success );

    if( success )
    {
      if( query.first() )
      {
        QStringList allValues( query.record().field( "attributeValues" ).value().toString().split( SEPARATOR ) );

        if( !query.prepare( UPDATE_ATTRIBUTEVALUES ) )
        {
          m_lastErrorMsg = QString( "Prepare UPDATE attribute values failed for attribute key \"%1\" - [%2]" ).arg( attributeKey )
                           .arg( query.lastError().text() );
          return false;
        }

        query.addBindValue( joinListElements( allValues ) );
        query.addBindValue( attributeKey );

        if( !query.exec() )
        {
          m_lastErrorMsg = QString( "UPDATE attribute values failed for attribute key \"%1\" - [%2]" ).arg( attributeKey )
                           .arg( query.lastError().text() );
          return false;
        }
      }
    }
    else
    {
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
    m_lastErrorMsg = QString( "Prepare SELECT failed for element [%1], error: [%2]" ).arg( element )
                     .arg( query.lastError().text() );
    success = false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element )
                     .arg( query.lastError().text() );
    success = false;
  }

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
      m_lastErrorMsg = QString( "Prepare INSERT element failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and children. */
    query.addBindValue( element );
    query.addBindValue( joinListElements( children ) );
    query.addBindValue( joinListElements( attributes ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT element failed for element \"%1\" - [%2]" ).arg( element )
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

  /* Make sure we aren't trying to insert an existing element. */
  if( !query.prepare( "SELECT * FROM rootelements WHERE root = ? ") )
  {
    m_lastErrorMsg = QString( "Prepare SELECT root element failed for root [%1], error: [%2]" ).arg( root )
                     .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( root );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT root element failed for root \"%1\" - [%2]" ).arg( root )
                     .arg( query.lastError().text() );
    return false;
  }

  if( !query.first() )
  {
    if( !query.prepare( "INSERT INTO rootelements ( root ) VALUES( ? )" ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT root element failed for root [%1], error: [%2]" ).arg( root )
                       .arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and children. */
    query.addBindValue( root );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT root element failed for element \"%1\" - [%2]" ).arg( root )
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
      m_lastErrorMsg = QString( "Prepare UPDATE element comment failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( joinListElements( allChildren ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE comment failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    m_lastErrorMsg = QString( "No knowledge of element \"%1\", add it first." ).arg( element );
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
      m_lastErrorMsg = QString( "Prepare UPDATE element attribute failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( joinListElements( allAttributes ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    m_lastErrorMsg = QString( "No element \"%1\" exists." ).arg( element );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAttribute( const QString &element, const QString &attribute, bool &success ) const
{
  return selectAttribute( element + "!" + attribute, success );
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAttribute( const QString &attributeKey, bool &success ) const
{
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM xmlattributevalues WHERE attributeKey = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT attribute failed for attribute key [%1], error: [%2]" ).arg( attributeKey )
                     .arg( query.lastError().text() );
    success = false;
  }

  /* The DB Key for this table is attributeKey (concatenated). */
  query.addBindValue( attributeKey );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT attribute failed for attribute key \"%1\" - [%2]" ).arg( attributeKey )
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
  QSqlQuery query = selectAttribute( element, attribute, success );

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
      m_lastErrorMsg = QString( "Prepare INSERT attribute failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    /* Create a list of all the associated attributes and children. */
    query.addBindValue( element + "!" + attribute );
    query.addBindValue( joinListElements( attributeValues ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT attribute failed for element \"%1\" - [%2]" ).arg( element )
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
      m_lastErrorMsg = QString( "Prepare UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( joinListElements( existingValues ) );
    query.addBindValue( element + "!" + attribute );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element )
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

  if( !query.exec( SELECT_ALL_ELEMENTS ) )
  {
    m_lastErrorMsg = QString( "SELECT all elements failed - [%1]" ).arg( query.lastError().text() );
    return QStringList();
  }

  QStringList elementNames;

  while( query.next() )
  {
    elementNames.append( query.record().field( "element" ).value().toString() );
  }

  cleanList( elementNames );
  return elementNames;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownAttributeKeys() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( SELECT_ALL_ATTRIBUTEVALUES ) )
  {
    m_lastErrorMsg = QString( "SELECT all attribute values failed - [%1]" ).arg( query.lastError().text() );
    return QStringList();
  }

  QStringList attributeNames;

  while( query.next() )
  {
    attributeNames.append( query.record().field( "attributeKey" ).value().toString() );
  }

  return attributeNames;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownRootElements() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM rootelements" ) )
  {
    m_lastErrorMsg = QString( "SELECT all root elements failed - [%1]" ).arg( query.lastError().text() );
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
    return QStringList();
  }

  QStringList children = query.record().value( "children" ).toString().split( SEPARATOR );
  cleanList( children );
  return children;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributes( const QString &element, bool &success ) const
{
  QSqlQuery query = selectElement( element, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    return QStringList();
  }

  QStringList attributes = query.record().value( "attributes" ).toString().split( SEPARATOR );
  cleanList( attributes );
  return attributes;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributeValues( const QString &element, const QString &attribute, bool &success ) const
{
  QSqlQuery query = selectAttribute( element, attribute, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    return QStringList();
  }

  QStringList attributeValues = query.record().value( "attributeValues" ).toString().split( SEPARATOR );
  cleanList( attributeValues );
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
