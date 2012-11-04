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
  ELEMENT (Key) | COMMENTS | ATTRIBUTES

  Table - XMLATTRIBUTES
  ATTRIBUTEKEY (concatenate element and attribute name) (Key) | ATTRIBUTEVALUES
*/

static const QLatin1String SELECT_ALL_ELEMENTS       ( "SELECT * FROM xmlelements" );
static const QLatin1String PREPARE_INSERT_ELEMENT    ( "INSERT INTO xmlelements( element, comments, attributes ) VALUES( ?, ?, ? )" );
static const QLatin1String PREPARE_DELETE_ELEMENT    ( "DELETE FROM xmlelements WHERE element = ?" );

static const QLatin1String SELECT_ALL_ATTRIBUTEVALUES     ( "SELECT * FROM xmlattributevalues" );
static const QLatin1String PREPARE_INSERT_ATTRIBUTEVALUES ( "INSERT INTO xmlattributevalues( attributeKey, attributeValues ) VALUES( ?, ? )" );
static const QLatin1String PREPARE_DELETE_ATTRIBUTEVALUES ( "DELETE FROM xmlattributevalues WHERE attributeKey = ?" );

/* Concatenate the new values to the existing values. The first '?' represents our string SEPARATOR. */
static const QLatin1String PREPARE_UPDATE_COMMENTS   ( "UPDATE xmlelements SET comments   = ( comments || ? || ? )   WHERE element = ?" );
static const QLatin1String PREPARE_UPDATE_ATTRIBUTES ( "UPDATE xmlelements SET attributes = ( attributes || ? || ? ) WHERE element = ?" );
static const QLatin1String PREPARE_UPDATE_ATTRIBUTEVALUES ( "UPDATE xmlattributevalues SET attributeValues = ( attributeValues || ? || ? ) WHERE attributeKey = ?" );

//static const QLatin1String UPSERT_ELEMENT ( " CASE EXISTS (SELECT * FROM xmlelements WHERE element = ? ) "
//                                            " THEN "
//                                            "   UPDATE xmlelements SET comments = CONCAT( comments, ? ), attributes = CONCAT( attributes, ? ) WHERE element = ? "
//                                            " ELSE"
//                                            "   INSERT INTO xmlelements( element, comments, attributes ) VALUES( ?, ?, ? ) "
//                                            " END" );



/*--------------------------------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );


/*--------------------------- NON-MEMBER UTILITY FUNCTIONS ----------------------------*/

QString joinListElements( QStringList list )
{
  list.removeDuplicates();
  list.removeAll( "" );
  return list.join( SEPARATOR );
}

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCDataBaseInterface::GCDataBaseInterface( QObject *parent ) :
  QObject           ( parent ),
  m_sessionDBName   ( "" ),
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

    QSqlDatabase::removeDatabase( dbConName );
    m_dbMap.remove( dbConName );
    saveDBFile();

    if( m_sessionDBName == dbConName )
    {
      m_sessionDBName = "";
      m_hasActiveSession = false;
    }

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

  if( QSqlDatabase::contains( dbConName ) )
  {
    if( openDBConnection( dbConName, m_lastErrorMsg ) )
    {
      m_lastErrorMsg = "";
      m_sessionDBName = dbConName;
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
      if( openDBConnection( dbConName, m_lastErrorMsg ) )
      {
        m_lastErrorMsg = "";
        m_sessionDBName = dbConName;
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

bool GCDataBaseInterface::openDBConnection( QString dbConName, QString &errMsg ) const
{
  /* If we have a previous connection open, close it. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( db.isValid() )
  {
    db.close();
  }

  /* Open the new connection. */
  db = QSqlDatabase::database( dbConName );

  if ( !db.open() )
  {
    errMsg = QString( "Failed to open database \"%1\": [%2]." ).arg( m_dbMap.value( dbConName ) )
                                                               .arg( db.lastError().text() );
    return false;
  }

  /* If the DB has not yet been initialised. */
  QStringList tables = db.tables();

  if ( !tables.contains( "xmlelements", Qt::CaseInsensitive ) )
  {
    return initialiseDB( dbConName, errMsg );
  }

  errMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::initialiseDB( QString dbConName, QString &errMsg ) const
{
  QSqlDatabase db = QSqlDatabase::database( dbConName );

  if( !db.isValid() )
  {
    errMsg = QString( "Couldn't establish a valid connection to \"%1\".").arg( dbConName );
    return false;
  }

  /* DB connection should be open from openDBConnection() above. */
  QSqlQuery query( db );

  if( !query.exec( "CREATE TABLE xmlelements( element QString primary key, comments QString, attributes QString )" ) )
  {
    errMsg = QString( "Failed to create elements table for \"%1\": [%2]." ).arg( dbConName )
                                                                           .arg( query.lastError().text() );
    return false;
  }

  if( !query.exec( "CREATE TABLE xmlattributevalues( attributeKey QString primary key, attributeValues QString )" ) )
  {
    errMsg = QString( "Failed to create attributes values table for \"%1\": [%2]" ).arg( dbConName )
                                                                            .arg( query.lastError().text() );
    return false;
  }

  errMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::batchProcessDOMDocument( const QDomDocument &domDoc ) const
{
  GCBatchProcessorHelper helper( domDoc );
  helper.setKnownElements  ( knownElements() );
  helper.setKnownAttributes( knownAttributes() );
  helper.createVariantLists();

  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    return false;
  }

  QSqlQuery query( db );

  /* Batch insert all the new elements. */
  if( !query.prepare( PREPARE_INSERT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare batch INSERT elements failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( helper.newElementsToAdd() );
  query.addBindValue( helper.newElementCommentsToAdd() );
  query.addBindValue( helper.newElementAttributesToAdd() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch INSERT elements failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  /* Batch update all the existing elements. */
  if( !query.prepare( PREPARE_UPDATE_COMMENTS ) )
  {
    m_lastErrorMsg = QString( "Prepare batch UPDATE element comments failed - [%1]" ).arg( query.lastError().text() );
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
  query.addBindValue( helper.elementCommentsToUpdate() );
  query.addBindValue( helper.elementsToUpdate() );

  if( !query.execBatch() )
  {
    m_lastErrorMsg = QString( "Batch UPDATE element comments failed - [%1]" ).arg( query.lastError().text() );
    return false;
  }

  if( !query.prepare( PREPARE_UPDATE_ATTRIBUTES ) )
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
  if( !query.prepare( PREPARE_INSERT_ATTRIBUTEVALUES ) )
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
  if( !query.prepare( PREPARE_UPDATE_ATTRIBUTEVALUES ) )
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

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectElement( const QString &element, bool &success ) const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    success = false;
  }

  /* See if we already have this element in the DB. */
  QSqlQuery query( db );

  if( !query.prepare( "SELECT * FROM xmlelements WHERE element = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT failed for element [%2], error: [%3]" ).arg( element )
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

bool GCDataBaseInterface::addElement( const QString &element, const QStringList &comments, const QStringList &attributes ) const
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
    if( !query.prepare( PREPARE_INSERT_ELEMENT ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT element failed for element \"%1\" - [%2]" ).arg( element )
                                                                                           .arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and comments. */
    query.addBindValue( element );
    query.addBindValue( joinListElements( comments ) );
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

bool GCDataBaseInterface::updateElementComments( const QString &element, const QStringList &comments ) const
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
    QStringList allComments( query.record().field( "comments" ).value().toString().split( SEPARATOR ) );
    allComments.append( comments );

    if( !query.prepare( PREPARE_UPDATE_COMMENTS ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element comment failed for element \"%1\" - [%2]" ).arg( element )
                                                                                                   .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( SEPARATOR );
    query.addBindValue( joinListElements( allComments ) );
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

    if( !query.prepare( PREPARE_UPDATE_ATTRIBUTES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element attribute failed for element \"%1\" - [%2]" ).arg( element )
                                                                                                     .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( SEPARATOR );
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
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    success = false;
  }

  QSqlQuery query( db );

  if( !query.prepare( "SELECT * FROM xmlattributevalues WHERE attributeKey = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT attribute failed for element [%2], error: [%3]" ).arg( element )
                                                                                               .arg( query.lastError().text() );
    success = false;
  }

  /* The DB Key for this table is attributeKey (concatenated). */
  query.addBindValue( element + attribute );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT attribute failed for element \"%1\" - [%2]" ).arg( element )
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
    if( !query.prepare( PREPARE_INSERT_ATTRIBUTEVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT attribute failed for element \"%1\" - [%2]" ).arg( element )
                                                                                             .arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and comments. */
    query.addBindValue( element + attribute );
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

    if( !query.prepare( PREPARE_UPDATE_ATTRIBUTEVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element )
                                                                                             .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( SEPARATOR );
    query.addBindValue( joinListElements( existingValues ) );
    query.addBindValue( element + attribute );

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

bool GCDataBaseInterface::removeElementComment( const QString &element, const QString &comment ) const
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

QStringList GCDataBaseInterface::knownElements() const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    return QStringList();
  }

  QSqlQuery query( db );

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

  elementNames.sort();
  return elementNames;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownAttributes() const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    return QStringList();
  }

  QSqlQuery query( db );

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

QStringList GCDataBaseInterface::attributes( const QString &element, bool &success ) const
{
  QSqlQuery query = selectElement( element, success );

  /* There should be only one record corresponding to this element. */
  if( !success || !query.first() )
  {
    return QStringList();
  }

  QStringList attributes = query.record().value( "attributes" ).toString().split( SEPARATOR );
  attributes.sort();
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
