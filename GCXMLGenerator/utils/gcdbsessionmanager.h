/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "GoblinCoding's XML Studio".
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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef GCDBSESSIONMANAGER_H
#define GCDBSESSIONMANAGER_H

#include <QObject>
#include "forms/gcknowndbform.h"

class GCDBSessionManager : public QObject
{
  Q_OBJECT
public:
  explicit GCDBSessionManager( QWidget *parent = 0 );
  void showKnownDBForm( GCKnownDBForm::Buttons buttons );

  void switchDBSession( const QString &currentRoot = QString() );
  void removeDB       ( const QString &currentRoot = QString() ); // shows known DB form

public slots:
  void addExistingDB( const QString &currentRoot = QString() ); // calls addDBConnection
  void addNewDB     ( const QString &currentRoot = QString() ); // calls addDBConnection

signals:
  void dbSessionChanged();
  void userCancelledKnownDBForm();
  void reset();
  
private slots:
  void removeDBConnection( const QString &dbName );  // receives signal from DB form
  void setSessionDB      ( const QString &dbName );  // receives signal from DB form

private:
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection    ( const QString &dbName );
  bool acceptSwitchReset();

  QWidget *m_parentWidget;
  QString  m_currentRoot;
  
};

#endif // GCDBSESSIONMANAGER_H
