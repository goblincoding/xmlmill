#ifndef GCDATABASEINTERFACE_H
#define GCDATABASEINTERFACE_H

#include <QObject>

class GCDataBaseInterface : public QObject
{
  Q_OBJECT
public:
  explicit GCDataBaseInterface( QObject *parent = 0 );
  bool initDB( QString dbFileName, QString &errMsg );
  
signals:
  
public slots:
  
};

#endif // GCDATABASEINTERFACE_H
