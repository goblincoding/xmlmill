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
  bool addElements  ( const GCElementsMap   &elements );
  bool addAttributes( const GCAttributesMap &attributes );

  /* Getters. */
  QStringList getDBList() const;
  QString  getLastError() const;
  bool hasActiveSession() const;
  
public slots:
  bool addDatabase   ( QString dbName );
  bool removeDatabase( QString dbName );
  bool setSessionDB  ( QString dbName );

private:
  void saveXMLFile();
  bool openDBConnection( QString dbConName, QString &errMsg );
  bool initialiseDB    ( QString dbConName, QString &errMsg );

  QString                  m_sessionDBName;
  QString                  m_lastErrorMsg;
  bool                     m_hasActiveSession;
  QMap< QString, QString > m_dbMap;   // connection name, file name
};

#endif // GCDATABASEINTERFACE_H
