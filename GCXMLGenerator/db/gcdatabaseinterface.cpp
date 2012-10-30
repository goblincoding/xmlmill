#include "gcdatabaseinterface.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

/*-------------------------------------------------------------*/

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
static const QLatin1String PREPARE_INSERT_ATTRVAL    ( "INSERT INTO xmlattributes( elementAttr, attributeValues ) VALUES( ?, ? )" );
static const QLatin1String PREPARE_DELETE_ATTRVAL    ( "DELETE FROM xmlattributes WHERE elementAttr = ?" );
static const QLatin1String PREPARE_SELECT_ATTRVAL    ( "SELECT * FROM xmlattributes WHERE elementAttr = ?" );

static const QLatin1String PREPARE_UPDATE_ATTRVALUES ( "UPDATE xmlattributes SET attributeValues = ? WHERE elementAttr = ?" );


//select * from sqlite_master where type = 'table' and name ='myTable';


/*-------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );

/* Separator for individual QStringList items. */
static const QString SEPARATOR( "~!@" );

/*-------------------------------------------------------------*/

GCDataBaseInterface::GCDataBaseInterface( QObject *parent ) :
  QObject           ( parent ),
  m_sessionDBName   ( "" ),
  m_lastErrorMsg    ( "" ),
  m_hasActiveSession( false ),
  m_dbMap           ()
{
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::initialise()
{
  QFile flatFile( DB_FILE );

  /* ReadWrite mode is really only required for the first time
    the file is opened so that QFile may create it in the process. */
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

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::openDBConnection( QString dbConName, QString &errMsg )
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

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::initialiseDB( QString dbConName, QString &errMsg )
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

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::addElement( const QString &element, const QStringList &comments, const QStringList &attributes )
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return false;
  }

  /* See if we already have this element in the DB. */
  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  /* If we don't have an existing record, add it. */
  if( !query.first() )
  {
    if( !query.prepare( PREPARE_INSERT_ELEMENT ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and comments. */
    query.addBindValue( element );
    query.addBindValue( comments.join( SEPARATOR ) );
    query.addBindValue( attributes.join( SEPARATOR ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::updateElementComments( const QString &element, const QStringList &comments )
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return false;
  }

  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  /* If we don't have an existing record, fail, otherwise update the existing one. */
  if( !query.first() )
  {
    m_lastErrorMsg = QString( "No element \"%1\" exists." ).arg( element );
    return false;
  }
  else
  {
    QStringList existingComments( query.record().field( "comments" ).value().toString().split( SEPARATOR ) );
    existingComments.append( comments.join( SEPARATOR ) );
    existingComments.removeDuplicates();
    existingComments.removeAll( "" );

    if( !query.prepare( PREPARE_UPDATE_COMMENTS ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( existingComments.join( SEPARATOR ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE comment failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::updateElementAttributes( const QString &element, const QStringList &attributes )
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return false;
  }

  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  /* If we don't have an existing record, fail, otherwise update the existing one. */
  if( !query.first() )
  {
    m_lastErrorMsg = QString( "No element \"%1\" exists." ).arg( element );
    return false;
  }
  else
  {
    QStringList existingAttributes( query.record().field( "attributes" ).value().toString().split( SEPARATOR ) );
    existingAttributes.append( attributes.join( SEPARATOR ) );
    existingAttributes.removeDuplicates();
    existingAttributes.removeAll( "" );

    if( !query.prepare( PREPARE_UPDATE_ATTRIBUTES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( existingAttributes.join( SEPARATOR ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::updateAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues )
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return false;
  }

  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ATTRVAL ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  /* The DB Key for this table is elementAttr (concatenated). */
  query.addBindValue( element + attribute );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return false;
  }

  /* If we don't have an existing record, add it, otherwise update the existing one. */
  if( !query.first() )
  {
    if( !query.prepare( PREPARE_INSERT_ATTRVAL ) )
    {
      m_lastErrorMsg = QString( "Prepare INSERT attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }

    /* Create a comma-separated list of all the associated attributes and comments. */
    query.addBindValue( element + attribute );
    query.addBindValue( attributeValues.join( SEPARATOR ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "INSERT attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    QStringList existingValues( query.record().field( "attributeValues" ).value().toString().split( SEPARATOR ) );
    existingValues.append( attributeValues.join( SEPARATOR ) );
    existingValues.removeDuplicates();
    existingValues.removeAll( "" );

    if( !query.prepare( PREPARE_UPDATE_ATTRVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( existingValues.join( SEPARATOR ) );
    query.addBindValue( element + attribute );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "UPDATE attribute failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::removeElement( const QString &element )
{
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::removeElementComment( const QString &element, const QString &comment )
{
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::removeElementAttribute( const QString &element, const QString &attribute )
{
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::removeAttributeValue( const QString &element, const QString &attribute, const QString &attributeValue )
{
  return true;
}

/*-------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownElements() const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
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

/*-------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributes( const QString &element ) const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return QStringList();
  }

  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return QStringList();
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return QStringList();
  }

  /* There should be only one record corresponding to this element. */
  query.first();

  QStringList attributes( query.record().value( "attributes" ).toString().split( SEPARATOR ) );
  attributes.removeDuplicates();
  attributes.removeAll( "" );

  return attributes;
}

/*-------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributeValues( const QString &element, const QString &attribute ) const
{
  /* Get the current session connection and ensure that it's valid. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return QStringList();
  }

  QSqlQuery query( db );

  if( !query.prepare( PREPARE_SELECT_ATTRVAL ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return QStringList();
  }

  query.addBindValue( element + attribute );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\" - [%2]" ).arg( element ).arg( query.lastError().text() );
    return QStringList();
  }

  /* There should be only one record corresponding to this element. */
  query.first();

  QStringList attributeValues( query.record().value( "attributeValues" ).toString().split( SEPARATOR ) );
  attributeValues.removeDuplicates();
  attributeValues.removeAll( "" );

  return attributeValues;
}

/*-------------------------------------------------------------*/

void GCDataBaseInterface::saveDBFile()
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

/*-------------------------------------------------------------*/

QStringList GCDataBaseInterface::getDBList() const
{
  return m_dbMap.keys();
}

/*-------------------------------------------------------------*/

QString GCDataBaseInterface::getLastError() const
{
  return m_lastErrorMsg;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::hasActiveSession() const
{
  return m_hasActiveSession;
}

/*-------------------------------------------------------------*/
