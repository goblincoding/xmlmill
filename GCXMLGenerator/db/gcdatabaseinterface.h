#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>
#include <QMap>
#include <QtSql/QSqlQuery>

class QDomDocument;

class GCDataBaseInterface : public QObject
{
  Q_OBJECT
public:
  explicit GCDataBaseInterface( QObject *parent = 0 );

  bool initialise();

  bool batchProcessDOMDocument( const QDomDocument &domDoc ) const;
  bool addElement( const QString &element, const QStringList &comments, const QStringList &attributes ) const;

  bool updateElementComments  ( const QString &element, const QStringList &comments ) const;
  bool updateElementAttributes( const QString &element, const QStringList &attributes ) const;
  bool updateAttributeValues  ( const QString &element, const QString &attribute, const QStringList &attributeValues ) const;

  bool removeElement( const QString &element ) const;
  bool removeElementComment( const QString &element, const QString &comment ) const;
  bool removeElementAttribute( const QString &element, const QString &attribute ) const;
  bool removeAttributeValue( const QString &element, const QString &attribute, const QString &attributeValue ) const;

  /* Getters. */
  QStringList getDBList() const;
  QString  getLastError() const;
  bool hasActiveSession() const;

  QStringList knownElements() const;
  QStringList attributes( const QString &element, bool &success ) const;
  QStringList attributeValues( const QString &element, const QString &attribute, bool &success ) const;

  
public slots:
  bool addDatabase   ( QString dbName );
  bool removeDatabase( QString dbName );
  bool setSessionDB  ( QString dbName );

private:
  QSqlQuery selectElement  ( const QString &element, bool &success ) const;
  QSqlQuery selectAttribute( const QString &element, const QString &attribute, bool &success ) const;
  QStringList knownAttributes() const;
  void saveDBFile() const;
  bool openDBConnection( QString dbConName, QString &errMsg ) const;
  bool initialiseDB    ( QString dbConName, QString &errMsg ) const;

  QString                  m_sessionDBName;
  mutable QString          m_lastErrorMsg;
  bool                     m_hasActiveSession;
  QMap< QString, QString > m_dbMap;   // connection name, file name
};

#endif // GCDATABASEINTERFACE_H
