#include "gcdatabaseinterface.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

/*-------------------------------------------------------------*/

/* SQL Command Strings. */
static const QLatin1String CREATE_TABLE_ELEMENTS   ( "CREATE TABLE xmlelements( element QString primary key, attributes QString )" );
static const QLatin1String CREATE_TABLE_ATTRIBUTES ( "CREATE TABLE xmlattributes( attribute QString primary key, attrvalues QString )" );

static const QLatin1String PREPARE_INSERT_ELEMENT  ( "INSERT INTO xmlelements( attributes ) VALUES( ? )" );
static const QLatin1String PREPARE_INSERT_ATTRIBUTE( "INSERT INTO xmlattributes( attrvalues ) VALUES( ? )" );

static const QLatin1String PREPARE_DELETE_ELEMENT  ( "DELETE FROM xmlelements WHERE element = ?" );
static const QLatin1String PREPARE_DELETE_ATTRIBUTE( "DELETE FROM xmlattributes WHERE attribute = ?" );

static const QLatin1String PREPARE_UPDATE_ELEMENT  ( "UPDATE xmlelements SET attributes = ? WHERE element = ?" );
static const QLatin1String PREPARE_UPDATE_ATTRIBUTE( "UPDATE xmlattributes SET attrvalues = ? WHERE attribute = ?" );

static const QLatin1String PREPARE_SELECT_ELEMENT  ( "SELECT attributes FROM xmlelements WHERE element = ?" );
static const QLatin1String PREPARE_SELECT_ATTRIBUTE( "SELECT attrvalues FROM xmlattributes WHERE attribute = ?" );

/*-------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );

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
        saveFile();

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
    saveFile();

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
  if( openDBConnection( dbConName, m_lastErrorMsg ) )
  {
    m_lastErrorMsg = "";
    m_sessionDBName = dbConName;
    m_hasActiveSession = true;
    return true;
  }
  else if( m_lastErrorMsg == "ADD_NEW_DB" )
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
  if( !QSqlDatabase::contains( dbConName ) )
  {
    errMsg = "ADD_NEW_DB";
    return false;
  }

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

bool GCDataBaseInterface::addElements( const GCElementsMap &elements )
{
  /* Get the current session connection and ensure that it's open. */
  QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

  if( !db.isValid() )
  {
    m_lastErrorMsg = QString( "Failed to open session connection \"%1\", error: %2" ).arg( m_sessionDBName ).arg( db.lastError().text() );
    return false;
  }

  /* Retrieve the records corresponding to the elements we just received (if
    they already exist) and update the DB with the associated attributes contained
    in the map.  If no record for this element exists, we'll obviously add a new one. */
  QList< QString > keys = elements.keys();

  for( int i = 0; i < keys.size(); ++i )
  {
    QSqlQuery query( db );

    if( !query.prepare( PREPARE_SELECT_ELEMENT ) )
    {
      m_lastErrorMsg = QString( "Prepare SELECT element failed - [%1]" ).arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( keys.at( i ) );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "SELECT element failed - [%1]" ).arg( query.lastError().text() );
      return false;
    }

    /* If we don't have an existing record, add it. */
    if( query.size() < 1 )
    {
      if( !query.prepare( PREPARE_INSERT_ELEMENT ) )
      {
        m_lastErrorMsg = QString( "Prepare INSERT element failed - [%1]" ).arg( query.lastError().text() );
        return false;
      }

      /* Create a comma-separated list of all the associated attributes. */
      query.addBindValue( elements.value( keys.at( i ) ).join( "," ) );

      if( !query.exec() )
      {
        m_lastErrorMsg = QString( "INSERT element failed - [%1]" ).arg( query.lastError().text() );
        return false;
      }
    }
    else
    {
      /* The value saved in the "attributes" column of the "xmlelements" table is a comma
        separated list of associated attributes. */
      QStringList attributes = query.value( query.record().indexOf( "attributes" ) ).toString().split( "," );
      attributes.append( elements.value( keys.at( i ) ) );
      attributes.removeDuplicates();

      if( !query.prepare( PREPARE_UPDATE_ELEMENT ) )
      {
        m_lastErrorMsg = QString( "Prepare UPDATE element failed - [%1]" ).arg( query.lastError().text() );
        return false;
      }

      /* Revert the QStringList back to a single comma-separated QString for storing. */
      query.addBindValue( attributes.join( "," ) );

      if( !query.exec() )
      {
        m_lastErrorMsg = QString( "UPDATE element failed - [%1]" ).arg( query.lastError().text() );
        return false;
      }
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::addAttributes( const GCAttributesMap &attributes )
{
  return true;
}

/*-------------------------------------------------------------*/

void GCDataBaseInterface::saveFile()
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


/* Example code from a previous pet project:
QSqlQuery q;

//Prepares and executes query
if(!q.prepare(QLatin1String("INSERT INTO testpackets(service, packet) VALUES(?,?)"))) {
  QMessageBox::critical(this,"Error","PREPARE Failed, Error: " + q.lastError().text());
  return;
}

q.addBindValue(serviceLineEdit->text());
q.addBindValue(requestTextEdit->toPlainText());

if(!q.exec()) {
  QMessageBox::critical(this,"Error","INSERT Failed, Error: " + q.lastError().text());
  return;
} else {
QMessageBox::information(this,"Success","Packet Added");
}



QSqlQuery q;

//Prepares and executes query
if(!q.prepare(QLatin1String("DELETE FROM testpackets WHERE id = ?"))) {
  QMessageBox::critical(this,"Error","PREPARE Failed, Error: " + q.lastError().text());
  return;
}

q.addBindValue(rec.field(0).value());

if(!q.exec()) {
  QMessageBox::critical(this,"Error","DELETE Failed, Error: " + q.lastError().text());
  return;
} else  {
requestTextEdit->clear();
QMessageBox::information(this,"Success","Packet Deleted");
rec.clear();        //clears record for next transaction
}



QSqlQuery q;

//Prepares and executes query
if(!q.prepare(QLatin1String("UPDATE testpackets SET packet = ? WHERE id = ?"))) {
  QMessageBox::critical(this,"Error","PREPARE Failed, Error: " + q.lastError().text());
  return;
}

q.addBindValue(requestTextEdit->toPlainText());
q.addBindValue(rec.field(0).value());

if(!q.exec()) {
  QMessageBox::critical(this,"Error","UPDATE Failed, Error: " + q.lastError().text());
  return;
} else {
QMessageBox::information(this,"Success","Packet Updated");
}

QSqlQuery q;

model.clear();

if(!q.prepare("SELECT * FROM testpackets WHERE service = ?")) {
  QMessageBox::critical(this,"Error","PREPARE Failed, Error: " + q.lastError().text());
  return;
}

q.addBindValue(serviceLineEdit->text());

if(!q.exec()) {
  QMessageBox::critical(this,"Error","SELECT Failed, Error: " + q.lastError().text());
  return;
}
*/
