#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include "utils/gctypes.h"
#include <QObject>
#include <QMap>

class GCDataBaseInterface : public QObject
{
  Q_OBJECT
public:
  explicit GCDataBaseInterface( QObject *parent = 0 );

  bool initialise();
  bool addElement( const QString &element, const QStringList &comments, const QStringList &attributes );

  bool updateElementComments( const QString &element, const QStringList &comments );
  bool updateElementAttributes( const QString &element, const QStringList &attributes );
  bool updateAttributeValue( const QString &element, const QString &attribute, const QStringList &attributeValues );

  bool removeElement( const QString &element );
  bool removeElementComment( const QString &element, const QString &comment );
  bool removeElementAttribute( const QString &element, const QString &attribute );
  bool removeAttributeValue( const QString &element, const QString &attribute, const QString &attributeValue );

  /* Getters. */
  QStringList getDBList() const;
  QString  getLastError() const;
  bool hasActiveSession() const;

  QStringList knownElements() const;
  QStringList attributes( const QString &element ) const;
  QStringList attributeValues( const QString &element, const QString &attribute ) const;

  
public slots:
  bool addDatabase   ( QString dbName );
  bool removeDatabase( QString dbName );
  bool setSessionDB  ( QString dbName );

private:
  void saveDBFile();
  bool openDBConnection( QString dbConName, QString &errMsg );
  bool initialiseDB    ( QString dbConName, QString &errMsg );

  QString                  m_sessionDBName;
  QString                  m_lastErrorMsg;
  bool                     m_hasActiveSession;
  QMap< QString, QString > m_dbMap;   // connection name, file name
};

#endif // GCDATABASEINTERFACE_H
