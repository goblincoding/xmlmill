#include "gcdatabaseinterface.h"
#include <QStringList>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

/*-------------------------------------------------------------*/

GCDataBaseInterface::GCDataBaseInterface( QObject *parent ) :
  QObject( parent )
{
}

/*-------------------------------------------------------------*/

bool GCDataBaseInterface::initDB( QString dbFileName, QString &errMsg )
{
  /* Opens connection to embedded DB. */
  QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
  db.setDatabaseName( dbFileName );

  if ( !db.open() )
  {
    errMsg = QString( "Failed to open database: [%1]." ).arg( db.lastError().text() );
    return false;
  }

  /* If DB not yet initialised. */
  QStringList tables = db.tables();

  if ( !tables.contains( "xmlnodes", Qt::CaseInsensitive ) )
  {
    QSqlQuery query;

    if( !query.exec( QLatin1String( "CREATE TABLE xmlnodes( element QString primary key,"
                                    "compulsory_attributes QString, optional_attributes )" ) ) )
    {
      errMsg = QString( "Failed to create table for XML nodes [%1]." ).arg( query.lastError().text() );
      return false;
    }

    if( !query.exec( QLatin1String( "CREATE TABLE attributes( attribute QString primary key,"
                                    "possible_values QString )" ) ) )
    {
      errMsg = QString( "Failed to create table for XML attributes [%1]." ).arg( query.lastError().text() );
      return false;
    }

  }

  return true;
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
