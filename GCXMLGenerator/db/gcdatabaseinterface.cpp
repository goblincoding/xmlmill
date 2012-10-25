#include "gcdatabaseinterface.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

/*-------------------------------------------------------------*/

/* SQL Command Strings. */
static const QLatin1String CREATE_TABLE_ELEMENTS    ( "CREATE TABLE xmlelements( element QString primary key,"
                                                      "compulsory_attributes QString, optional_attributes )" );

static const QLatin1String CREATE_TABLE_ATTRIBUTES  ( "CREATE TABLE xmlattributes( attribute QString primary key,"
                                                      "possible_values QString )" );

static const QLatin1String PREPARE_INSERT_ELEMENTS  ( "INSERT INTO xmlelements( compulsory_attributes, optional_attributes) VALUES( ?,? )" );
static const QLatin1String PREPARE_INSERT_ATTRIBUTES( "INSERT INTO xmlattributes( possible_values ) VALUES( ? )" );

static const QLatin1String PREPARE_DELETE_ELEMENTS  ( "DELETE FROM xmlelements WHERE element = ?" );
static const QLatin1String PREPARE_DELETE_ATTRIBUTES( "DELETE FROM xmlattributes WHERE attribute = ?" );

static const QLatin1String PREPARE_UPDATE_ELEMENTS  ( "UPDATE xmlelements SET compulsory_attributes = ?, optional_attributes = ? WHERE element = ?" );
static const QLatin1String PREPARE_UPDATE_ATTRIBUTES( "UPDATE xmlattributes SET possible_values = ? WHERE attribute = ?" );

/*-------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );

/*-------------------------------------------------------------*/

GCDataBaseInterface::GCDataBaseInterface( QObject *parent ) :
  QObject         ( parent ),
  m_sessionDBName ( "" ),
  m_lastErrorMsg  ( "" ),
  m_dbMap         ()
{
}

/*-------------------------------------------------------------*/

GCDataBaseInterface::~GCDataBaseInterface()
{
  /* Upon exit, replace the database list on record with what we
    currently have in the map (to cater for updates, additions,
    removals and so forth). */

}


/*-------------------------------------------------------------*/

bool GCDataBaseInterface::initialise()
{
  QFile flatFile( DB_FILE );

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
      m_dbMap.insert( str.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last(), str );
    }

    return true;
  }

  m_lastErrorMsg = QString( "Failed to access list of databases, file open error: [%1]." ).arg( flatFile.errorString() );
  return false;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::addDatabase( QString dbName )
{
  if( !dbName.isEmpty() )
  {
    m_lastErrorMsg = QString( "" );

    /* Pass the entire path/to/file string through, this string will be
      stripped and manipulated in addDBConnection (to ensure we only add
      it to the map once a connection has been successfully created). */
    return addDBConnection( dbName, m_lastErrorMsg );
  }

  m_lastErrorMsg = QString( "Database name is empty." );
  return false;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::addDBConnection( QString dbName, QString &errMsg )
{
  /* The DB name passed in will most probably consist of a path/to/file string. */
  QString dbFileName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

  QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", dbName );

  if( !db.isValid() )
  {
    errMsg = QString( "Failed to add database \"%1\": [%2]." ).arg( dbFileName ).arg( db.lastError().text() );
    return false;
  }

  db.setDatabaseName( dbFileName );
  m_dbMap.insert( dbFileName, dbName );
  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::setSessionDB( QString dbName )
{
  m_lastErrorMsg = QString( "" );

  /* We get the database name as parameter, but wish to work with
    the connection name from here on. */
  if( openDBConnection( m_dbMap.value( dbName ), m_lastErrorMsg ) )
  {
    m_sessionDBName = dbName;
    return true;
  }

  return false;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::openDBConnection( QString dbName, QString &errMsg )
{
  if( QSqlDatabase::contains( dbName ) )
  {
    /* If we have a previous connection open, close it. */
    QSqlDatabase db = QSqlDatabase::database( m_sessionDBName );

    if( db.isValid() )
    {
      db.close();
    }

    /* Open the new connection. */
    db = QSqlDatabase::database( dbName );

    if ( !db.open() )
    {
      errMsg = QString( "Failed to open database \"%1\": [%2]." ).arg( dbName ).arg( db.lastError().text() );
      return false;
    }

    /* If the DB has not yet been initialised. */
    QStringList tables = db.tables();

    if ( !tables.contains( "xmlelements", Qt::CaseInsensitive ) )
    {
      return initialiseDB( dbName, errMsg );
    }
  }
  else
  {
    return addDBConnection( dbName, errMsg );
  }

  return true;
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::initialiseDB( QString dbName, QString &errMsg )
{
  QSqlDatabase db = QSqlDatabase::database( dbName );

  if( db.isValid() )
  {
    QSqlQuery query( db );

    if( !query.exec( CREATE_TABLE_ELEMENTS ) )
    {
      errMsg = QString( "Failed to create elements table for \"%1\": [%2]." ).arg( dbName ).arg( query.lastError().text() );
      return false;
    }

    if( !query.exec( CREATE_TABLE_ATTRIBUTES ) )
    {
      errMsg = QString( "Failed to create attributes table for \"%1\": [%2]" ).arg( dbName ).arg( query.lastError().text() );
      return false;
    }
  }
  else
  {
    errMsg = QString( "Couldn't establish a valid connection to \"%1\".").arg( dbName );
    return false;
  }

  return true;
}

/*-------------------------------------------------------------*/

QStringList GCDataBaseInterface::getDBList() const
{
  return m_dbMap.values();
}

/*-------------------------------------------------------------*/

QString GCDataBaseInterface::getLastError() const
{
  return m_lastErrorMsg;
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
