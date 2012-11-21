#ifndef GCDBSESSIONMANAGER_H
#define GCDBSESSIONMANAGER_H

#include <QObject>
#include "forms/gcknowndbform.h"

class QSettings;

class GCDBSessionManager : public QObject
{
  Q_OBJECT
public:
  explicit GCDBSessionManager( QWidget *parent = 0 );
  void showKnownDBForm( GCKnownDBForm::Buttons buttons );
  void switchDBSession( bool docEmpty );

signals:
  void saveSetting( const QString &key, const QVariant &value );
  void rememberPreference( bool remember );
  void dbSessionChanged();
  void userCancelledKnownDBForm();
  void reset();
  
private slots:
  void addNewDB();      // calls addDBConnection
  void addExistingDB(); // calls addDBConnection

  void setSessionDB( const QString &dbName ); // receives signal from DB form

  void removeDB();                                  // shows known DB form
  void removeDBConnection( const QString &dbName ); // receives signal from DB form

private:
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection    ( const QString &dbName );

  QSettings *m_settings;
  QWidget   *m_parentWidget;
  
};

#endif // GCDBSESSIONMANAGER_H
