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

#include "gcdatabaseinterface.h"
#include "gcbatchprocessorhelper.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>

/*--------------------------------------------------------------------------------------*/

/* Have a look at "createTables" to see how the DB is set up. */
static const QLatin1String INSERT_ELEMENT(
    "INSERT INTO xmlelements( element, children, attributes ) VALUES( ?, ?, ? )" );

static const QLatin1String INSERT_ATTRIBUTEVALUES(
    "INSERT INTO xmlattributes( attribute, associatedElement, attributeValues ) VALUES( ?, ?, ? )" );

static const QLatin1String UPDATE_CHILDREN(
    "UPDATE xmlelements SET children   = ? WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTES(
    "UPDATE xmlelements SET attributes = ? WHERE element = ?" );

static const QLatin1String UPDATE_ATTRIBUTEVALUES(
    "UPDATE xmlattributes SET attributeValues = ? WHERE attribute = ? AND associatedElement = ?" );

/*--------------------------------------------------------------------------------------*/

/* Flat file containing list of databases. */
static const QString DB_FILE( "dblist.txt" );

/* Regular expression string to split "\" (Windows) or "/" (Unix) from file path. */
static const QString REGEXP_SLASHES( "(\\\\|\\/)" );

/* The database tables have fields containing strings of strings. For example, the
  "xmlelements" table maps a unique element against a list of all associated attribute
  values.  Since these values have to be entered into a single record, the easiest way
  is to insert a single (possibly massive) string containing all the associated attributes.
  To ensure that we can later extract the individual attributes again, we separate them with
  a sequence that should (theoretically) never be encountered.  This is that sequence. */
static const QString SEPARATOR( "~!@" );


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

GCDataBaseInterface* GCDataBaseInterface::m_instance = NULL;

GCDataBaseInterface* GCDataBaseInterface::instance()
{
  if( !m_instance )
  {
    m_instance = new GCDataBaseInterface;
  }

  return m_instance;
}

/*--------------------------------------------------------------------------------------*/

GCDataBaseInterface::GCDataBaseInterface() :
  m_sessionDB       (),
  m_lastErrorMsg    ( "" ),
  m_hasActiveSession( false ),
  m_initialised     ( false ),
  m_dbMap           ()
{
  QFile flatFile( DB_FILE );

  /* ReadWrite mode is required to create the file if it doesn't exist. */
  if( flatFile.open( QIODevice::ReadWrite | QIODevice::Text ) )
  {
    m_lastErrorMsg = "";
    m_initialised = true;

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
        m_initialised = false;
      }
    }
  }
  else
  {
    m_lastErrorMsg = QString( "Failed to access list of databases, file open error: [%1]." ).arg( flatFile.errorString() );
    m_initialised = false;
  }
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::initialised()
{
  return m_initialised;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::batchProcessDOMDocument( const QDomDocument *domDoc ) const
{
  GCBatchProcessorHelper helper( domDoc,
                                 SEPARATOR,
                                 knownElements(),
                                 knownAttributeKeys() );

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

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

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

  /* Batch update all the existing elements by concatenating the new values to the
    existing values. The first '?' represents our string SEPARATOR. */
  if( !query.prepare( "UPDATE xmlelements "
                      "SET children = ( children || ? || ? ) "
                      "WHERE element = ?" ) )
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

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

  if( !query.prepare( "UPDATE xmlelements "
                      "SET attributes = ( attributes || ? || ? ) "
                      "WHERE element = ?" ) )
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

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

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

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

  /* Batch update all the existing attribute values. */
  if( !query.prepare( "UPDATE xmlattributes "
                      "SET attributeValues = ( attributeValues || ? || ? ) "
                      "WHERE attribute = ? "
                      "AND associatedElement = ?" ) )
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

bool GCDataBaseInterface::addElement( const QString &element, const QStringList &children, const QStringList &attributes ) const
{
  QSqlQuery query = selectElement( element );

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
  QSqlQuery query = selectElement( element );

  /* Update the existing record (if we have one). */
  if( query.first() )
  {
    QStringList allChildren( query.record().field( "children" ).value().toString().split( SEPARATOR ) );
    allChildren.append( children );

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
  QSqlQuery query = selectElement( element );

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

bool GCDataBaseInterface::updateAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const
{
  QSqlQuery query = selectAttribute( attribute, element );

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

    /* The reason for not using concatenating values here is that we don't simply want to add
      all the supposed new values, we want to make sure they are all unique by removing all duplicates
      before sticking it all back into the DB. */
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

bool GCDataBaseInterface::replaceAttributeValues( const QString &element, const QString &attribute, const QStringList &attributeValues ) const
{
  QSqlQuery query = selectAttribute( attribute, element );

  /* Only continue if we have an existing record. */
  if( query.first() )
  {
    /* Create a duplicate list that we can safely manipulate locally. */
    QStringList attributeValuesToClean( attributeValues );

    if( !query.prepare( UPDATE_ATTRIBUTEVALUES ) )
    {
      m_lastErrorMsg = QString( "Prepare UPDATE attribute values failed for element \"%1\" and attribute \"%2\": [%3]" )
          .arg( element )
          .arg( attribute )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( cleanAndJoinListElements( attributeValuesToClean ) );
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
  QSqlQuery query = selectElement( element );

  /* Only continue if we have an existing record. */
  if( query.first() )
  {
    if( !query.prepare( "DELETE FROM xmlelements WHERE element = ?" ) )
    {
      m_lastErrorMsg = QString( "Prepare DELETE element failed for element \"%1\": [%3]" )
                       .arg( element )
                       .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "DELETE element failed for element \"%1\": [%3]" )
                       .arg( element )
                       .arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeChildElement( const QString &element, const QString &child ) const
{
  /* This function is virtually exactly the same as "updateElementChildren" with the only
    exception being that we do not append the child to the known children, but remove it from
    the list and replace the previous DB entry with the updated one...I'll have to rethink
    this approach and see if I can't refactor these functions somehow. */
  QSqlQuery query = selectElement( element );

  /* Update the existing record (if we have one). */
  if( query.first() )
  {
    QStringList allChildren( query.record().field( "children" ).value().toString().split( SEPARATOR ) );
    allChildren.removeAll( child );

    if( !query.prepare( UPDATE_CHILDREN ) )
    {
      m_lastErrorMsg = QString( "Prepare REMOVE element child failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( cleanAndJoinListElements( allChildren ) );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "REMOVE child failed for element \"%1\": [%2]" )
          .arg( element )
          .arg( query.lastError().text() );
      return false;
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeAttribute( const QString &element, const QString &attribute ) const
{    
  QSqlQuery query = selectAttribute( attribute, element );

  /* Only continue if we have an existing record. */
  if( query.first() )
  {
    if( !query.prepare( "DELETE FROM xmlattributes "
                        "WHERE attribute = ? "
                        "AND associatedElement = ?" ) )
    {
      m_lastErrorMsg = QString( "Prepare DELETE attribute failed for element \"%1\" and attribute \"%2\": [%3]" )
                       .arg( element )
                       .arg( attribute )
                       .arg( query.lastError().text() );
      return false;
    }

    query.addBindValue( attribute );
    query.addBindValue( element );

    if( !query.exec() )
    {
      m_lastErrorMsg = QString( "DELETE attribute failed for element \"%1\" and attribute [%2]: [%3]" )
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

bool GCDataBaseInterface::removeRootElement( const QString &element ) const
{
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "DELETE FROM rootelements WHERE root = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare DELETE failed for root \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
    return false;
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "DELETE root element failed for root \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
    return false;
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::hasActiveSession() const
{
  return m_hasActiveSession;
}

/*--------------------------------------------------------------------------------------*/

QString GCDataBaseInterface::activeSessionName() const
{
  if( m_hasActiveSession )
  {
    return m_sessionDB.connectionName();
  }

  return QString();
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::profileEmpty() const
{
  return knownRootElements().isEmpty();
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::isUniqueChildElement( const QString &parentElement, const QString &element ) const
{
  QSqlQuery query = selectAllElements();

  while( query.next() )
  {
    if( query.record().field( "element" ).value().toString() != parentElement &&
        query.record().value( "children" ).toString().split( SEPARATOR ).contains( element ) )
    {
      return false;
    }
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownElements() const
{
  QSqlQuery query = selectAllElements();

  m_lastErrorMsg = "";

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

QStringList GCDataBaseInterface::children( const QString &element ) const
{
  QSqlQuery query = selectElement( element );

  /* There should be only one record corresponding to this element. */
  if( !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of children for element \"%1\"" )
        .arg( element );
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList children = query.record().value( "children" ).toString().split( SEPARATOR );
  cleanList( children );
  children.sort();
  return children;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributes( const QString &element ) const
{
  QSqlQuery query = selectElement( element );

  /* There should be only one record corresponding to this element. */
  if( !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of attributes for element \"%1\"" )
        .arg( element );
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList attributes = query.record().value( "attributes" ).toString().split( SEPARATOR );
  cleanList( attributes );
  return attributes;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::attributeValues( const QString &element, const QString &attribute ) const
{
  QSqlQuery query = selectAttribute( attribute, element );

  /* There should be only one record corresponding to this element. */
  if( !query.first() )
  {
    m_lastErrorMsg = QString( "Failed to obtain the list of attribute values for attribute \"%1\"" )
        .arg( attribute );
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList attributeValues = query.record().value( "attributeValues" ).toString().split( SEPARATOR );
  cleanList( attributeValues );
  attributeValues.sort();
  return attributeValues;
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownRootElements() const
{
  return knownRootElements( m_sessionDB );
}

/*--------------------------------------------------------------------------------------*/

QStringList GCDataBaseInterface::knownRootElements( QSqlDatabase db ) const
{
  QSqlQuery query( db );

  if( !query.exec( "SELECT * FROM rootelements" ) )
  {
    m_lastErrorMsg = QString( "SELECT all root elements failed: [%1]" )
        .arg( query.lastError().text() );
    return QStringList();
  }

  m_lastErrorMsg = "";

  QStringList rootElements;

  while( query.next() )
  {
    rootElements.append( query.record().field( "root" ).value().toString() );
  }

  cleanList( rootElements );
  return rootElements;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::containsKnownRootElement( const QString &dbName, const QString &root ) const
{
  /* In case the db name passed in consists of a path/to/file string. */
  QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

  /* No error messages are logged for this specific query since we aren't necessarily concerned with
    the session we're querying (it may not be the active session). */
  if( QSqlDatabase::contains( dbConName ) )
  {
    QSqlDatabase db = QSqlDatabase::database( dbConName );

    if( db.isValid() && db.open() )
    {
      if( knownRootElements( db ).contains( root ) )
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }

  return false;
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

bool GCDataBaseInterface::addDatabase( const QString &dbName )
{
  if( !dbName.isEmpty() )
  {
    /* In case the db name passed in consists of a path/to/file string. */
    QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

    if( !m_dbMap.contains( dbConName ) )
    {
      QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", dbConName );

      if( db.isValid() )
      {
        db.setDatabaseName( dbName );
        m_dbMap.insert( dbConName, dbName );
        saveDatabaseFile();

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
    /* In case the db name passed in consists of a path/to/file string. */
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
      a warning (to the Qt IDE's "Application Output" window).  This is purely because we have a
      DB member variable and isn't cause for concern as there seems to be no way around it with
      the current QtSQL modules. */
    QSqlDatabase::removeDatabase( dbConName );
    m_dbMap.remove( dbConName );
    saveDatabaseFile();

    m_lastErrorMsg = "";
    return true;
  }

  m_lastErrorMsg = QString( "Database name is empty." );
  return false;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::setActiveDatabase( const QString &dbName )
{
  /* In case the db name passed in consists of a path/to/file string. */
  QString dbConName = dbName.split( QRegExp( REGEXP_SLASHES ), QString::SkipEmptyParts ).last();

  if( QSqlDatabase::contains( dbConName ) )
  {
    if( openConnection( dbConName ) )
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
      if( openConnection( dbConName ) )
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

QStringList GCDataBaseInterface::knownAttributeKeys() const
{
  QSqlQuery query( m_sessionDB );

  m_lastErrorMsg = "";

  QStringList attributeNames;

  while( query.next() )
  {
    /* Concatenate the attribute name and associated element into a single string
      so that it is easier to determine whether a record already exists for that
      particular combination (this is used in GCBatchProcessorHelper). */
    attributeNames.append( query.record().field( "attribute" ).value().toString() +
                           "!" +
                           query.record().field( "associatedElement" ).value().toString());
  }

  return attributeNames;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectElement( const QString &element ) const
{
  /* See if we already have this element in the DB. */
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM xmlelements WHERE element = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT failed for element \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
  }

  query.addBindValue( element );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT element failed for element \"%1\": [%2]" )
        .arg( element )
        .arg( query.lastError().text() );
  }

  return query;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAllElements() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM xmlelements" ) )
  {
    m_lastErrorMsg = QString( "SELECT all root elements failed: [%1]" )
        .arg( query.lastError().text() );
  }

  return query;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAttribute( const QString &attribute, const QString &associatedElement ) const
{
  QSqlQuery query( m_sessionDB );

  if( !query.prepare( "SELECT * FROM xmlattributes "
                      "WHERE attribute = ? "
                      "AND associatedElement = ?" ) )
  {
    m_lastErrorMsg = QString( "Prepare SELECT attribute failed for attribute \"%1\" and element \"%2\": [%3]" )
        .arg( attribute )
        .arg( associatedElement )
        .arg( query.lastError().text() );
  }

  query.addBindValue( attribute );
  query.addBindValue( associatedElement );

  if( !query.exec() )
  {
    m_lastErrorMsg = QString( "SELECT attribute failed for attribute \"%1\" and element \"%2\": [%3]" )
        .arg( attribute )
        .arg( associatedElement )
        .arg( query.lastError().text() );
  }

  return query;
}

/*--------------------------------------------------------------------------------------*/

QSqlQuery GCDataBaseInterface::selectAllAttributes() const
{
  QSqlQuery query( m_sessionDB );

  if( !query.exec( "SELECT * FROM xmlattributes" ) )
  {
    m_lastErrorMsg = QString( "SELECT all attribute values failed: [%1]" )
        .arg( query.lastError().text() );
  }

  return query;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::removeDuplicatesFromFields() const
{
  /* Remove duplicates and update the element records. */
  QSqlQuery query = selectAllElements();

  /* Not checking for query validity since the table may still be empty when
    this funciton gets called (i.e. there is a potentially valid reason for cases
    where no valid records exist). */
  while( query.next() )
  {
    /* Does a record for this element exist? */
    if( query.first() )
    {
      QString element = query.record().field( "element" ).value().toString();
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

      qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
    }
  }

  /* Remove duplicates and update the attribute values records. */
  query = selectAllAttributes();

  /* Not checking for query validity since the table may still be empty when
    this funciton gets called (i.e. there is a potentially valid reason for cases
    where no valid records exist). */
  while( query.next() )
  {
    /* Does a record for this attribute exist? */
    if( query.first() )
    {
      QString associatedElement = query.record().field( "associatedElement" ).value().toString();
      QString attribute = query.record().field( "attribute" ).value().toString();
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

      qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
    }
  }

  m_lastErrorMsg = "";
  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCDataBaseInterface::openConnection( const QString &dbConName )
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
      return createTables();
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

bool GCDataBaseInterface::createTables() const
{
  /* DB connection will be open from openConnection() above so no need to do any checks here. */
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

void GCDataBaseInterface::saveDatabaseFile() const
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
