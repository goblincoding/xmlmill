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

/* DB tables are set up as follows:
  Table - XMLELEMENTS
  ELEMENT (Key) | COMMENTS | ATTRIBUTES

  Table - XMLATTRIBUTES
  ELEMENTATTRIBUTE (concatenate element and attribute name) (Key) | ATTRIBUTEVALUES
*/

static const QLatin1String CREATE_TABLE_ELEMENTS     ( "CREATE TABLE xmlelements( element QString primary key, comments QString, attributes QString )" );
static const QLatin1String PREPARE_INSERT_ELEMENT    ( "INSERT INTO xmlelements( element, comments, attributes ) VALUES( ?, ?, ? )" );
static const QLatin1String PREPARE_DELETE_ELEMENT    ( "DELETE FROM xmlelements WHERE element = ?" );
static const QLatin1String PREPARE_SELECT_ELEMENT    ( "SELECT * FROM xmlelements WHERE element = ?" );
static const QLatin1String SELECT_ALL_ELEMENTS       ( "SELECT * FROM xmlelements" );

static const QLatin1String PREPARE_UPDATE_COMMENTS   ( "UPDATE xmlelements SET comments = ? WHERE element = ?" );
static const QLatin1String PREPARE_UPDATE_ATTRIBUTES ( "UPDATE xmlelements SET attributes = ? WHERE element = ?" );

static const QLatin1String CREATE_TABLE_ATTRIBUTES   ( "CREATE TABLE xmlattributes( elementAttr QString primary key, attributeValues QString )" );
static const QLatin1String PREPARE_INSERT_ATTRIBUTES ( "INSERT INTO xmlattributes( elementAttr, attributeValues ) VALUES( ?, ? )" );
static const QLatin1String PREPARE_DELETE_ATTRIBUTES ( "DELETE FROM xmlattributes WHERE elementAttr = ?" );
static const QLatin1String PREPARE_SELECT_ATTRIBUTES ( "SELECT * FROM xmlattributes WHERE elementAttr = ?" );
static const QLatin1String SELECT_ALL_ATTRIBUTES     ( "SELECT * FROM xmlattributes" );

static const QLatin1String PREPARE_UPDATE_ATTRVALUES ( "UPDATE xmlattributes SET attributeValues = ? WHERE elementAttr = ?" );

/*--------------------------------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );

/* Separator for individual QStringList items. */
static const QString SEPARATOR( "~!@" );


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

  if( !query.exec( CREATE_TABLE_ELEMENTS ) )
  {
    errMsg = QString( "Failed to create elements table for \"%1\": [%2]." ).arg( dbConName )
                                                                           .arg( query.lastError().text() );
    return false;
  }

  if( !query.exec( CREATE_TABLE_ATTRIBUTES ) )
  {
    errMsg = QString( "Failed to create attributes table for \"%1\": [%2]" ).arg( dbConName )
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
  QStringList  helperElementNames = helper.getElementNames();

  QStringList  existingElements = knownElements();
  QVariantList elementsToUpdate;
  QVariantList elementsToAdd;

  /* Separate new from existing elements. */
  for( int i = 0; i < helperElementNames.size(); ++i )
  {
    if( existingElements.contains( helperElementNames.at( i ) ) )
    {
      elementsToUpdate << helperElementNames.at( i );
    }
    else
    {
      elementsToAdd << helperElementNames.at( i );
    }
  }

  /* I haven't figured out a way to batch process UPDATES...the problem is that we will have to
    extract what is already there in order to add what is new and not overwrite what exists.
    The only way forward may be to go the GCElementRecord route... */
  QVariantList commentsToAdd;               // goes into xmlelements table
  QVariantList attributesToAdd;             // goes into xmlelements table
  QVariantList elementAttributesToAdd;      // goes into xmlattributes table
  QVariantList elementAttributeValuesToAdd; // goes into xmlattributes table

  foreach( QVariant elementVariant, elementsToAdd )
  {
    QString element = elementVariant.toString();
    QString comments = helper.getElementComments( element ).join( SEPARATOR );

    if( !comments.isEmpty() )
    {
      commentsToAdd << comments;
    }
    else
    {
      commentsToAdd << QVariant( QVariant::String );
    }

    QString attributes = helper.getAttributeNames( element ).join( SEPARATOR );

    if( !attributes.isEmpty() )
    {
      attributesToAdd << attributes;

      foreach( QString attribute, attributes )
      {
        elementAttributesToAdd << element + attribute;

        QString elementAttributeValues = helper.getAttributeValues( element, attribute ).join( SEPARATOR );

        if( !elementAttributeValues.isEmpty() )
        {
          elementAttributeValuesToAdd << elementAttributeValues;
        }
        else
        {
          elementAttributeValuesToAdd << QVariant( QVariant::String );
        }
      }
    }
    else
    {
      attributesToAdd << QVariant( QVariant::String );
    }
  }

  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName )
                                                                                     .arg( db.lastError().text() );
    return false;
  }

  /* See if we already have this element in the DB. */
  QSqlQuery query( db );

  if( !query.prepare( PREPARE_INSERT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare INSERT batch elements failed - [%2]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( elementsToAdd );
  query.addBindValue( commentsToAdd );
  query.addBindValue( attributesToAdd );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "INSERT batch elements failed - [%2]" ).arg( query.lastError().text() );
    return false;
  }

  if( !query.prepare( PREPARE_INSERT_ATTRIBUTES ) )
  {
    m_lastErrorMsg = QString( "Prepare INSERT batch attributes failed - [%2]" ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( elementAttributesToAdd );
  query.addBindValue( elementAttributeValuesToAdd );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "INSERT batch attributes failed - [%2]" ).arg( query.lastError().text() );
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

  if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "\"%1\" FAILED for element [%2], error: [%3]" ).arg( PREPARE_SELECT_ELEMENT )
                                                                             .arg( element )
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

  if( !query.prepare( PREPARE_SELECT_ATTRIBUTES ) )
  {
    m_lastErrorMsg = QString( "\"%1\" FAILED for element [%2], error: [%3]" ).arg( PREPARE_SELECT_ATTRIBUTES )
                                                                             .arg( element )
                                                                             .arg( query.lastError().text() );
    success = false;
  }

  /* The DB Key for this table is elementAttr (concatenated). */
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
    if( !query.prepare( PREPARE_INSERT_ATTRIBUTES ) )
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

    if( !query.prepare( PREPARE_UPDATE_ATTRVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element )
                                                                                             .arg( query.lastError().text() );
      return false;
    }

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

  if( !query.exec( SELECT_ALL_ATTRIBUTES ) )
  {
    m_lastErrorMsg = QString( "SELECT all attributes failed - [%1]" ).arg( query.lastError().text() );
    return QStringList();
  }

  QStringList attributeNames;

  while( query.next() )
  {
    attributeNames.append( query.record().field( "elementAttr" ).value().toString() );
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

  return QStringList( query.record().value( "attributes" ).toString().split( SEPARATOR ) );
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

  return QStringList( query.record().value( "attributeValues" ).toString().split( SEPARATOR ) );
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
