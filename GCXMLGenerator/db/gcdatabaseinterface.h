#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>

class GCDataBaseInterface : public QObject
{
  Q_OBJECT
public:
  explicit GCDataBaseInterface( QObject *parent = 0 );
  bool initialise();

  /* Getters. */
  const QStringList &getDBList() const;
  QString getLastError() const;
  
public slots:
  bool addDatabase ( QString dbName );
  bool setSessionDB( QString dbName );

private:
  bool addDBConnection ( QString dbName, QString &errMsg );
  bool openDBConnection( QString dbName, QString &errMsg );
  bool initialiseDB    ( QString dbName, QString &errMsg );

  QString     m_sessionDBName;
  QString     m_lastErrorMsg;
  QStringList m_dbList;
};

#endif // GCDATABASEINTERFACE_H
