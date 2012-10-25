#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>
#include <QMap>

class GCDataBaseInterface : public QObject
{
  Q_OBJECT
public:
  explicit GCDataBaseInterface( QObject *parent = 0 );
  ~GCDataBaseInterface();

  bool initialise();

  /* Getters. */
  QStringList getDBList() const;
  QString  getLastError() const;
  
public slots:
  bool addDatabase ( QString dbName );
  bool setSessionDB( QString dbName );

private:
  bool openDBConnection( QString dbConName, QString &errMsg );
  bool initialiseDB    ( QString dbConName, QString &errMsg );

  QString                  m_sessionDBName;
  QString                  m_lastErrorMsg;
  QMap< QString, QString > m_dbMap;
};

#endif // GCDATABASEINTERFACE_H
