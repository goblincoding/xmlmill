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

#ifndef GCDBSESSIONMANAGER_H
#define GCDBSESSIONMANAGER_H

#include <QObject>
#include "forms/gcknowndbform.h"

/*----------------------------------------------------------------------------------

   The name pretty much says it all.  This guy is responsible for managing database
   connections and active database sessions and will prompt the user to confirm
   actions or changes that may result in the current DOM doc being reset.

----------------------------------------------------------------------------------*/

class GCDBSessionManager : public QObject
{
  Q_OBJECT

public:
  explicit GCDBSessionManager( QWidget *parent = 0 );

  /* Should only be called directly in cases where there isn't an active
    DOM document (for those it's best to use the functions that take the
    known document root name as parameter to ensure that there will be no
    accidental deletion of a user's work). */
  void showKnownDBForm( GCKnownDBForm::Buttons buttons );

  void switchActiveDatabase( const QString &currentRoot = QString() );
  void removeDatabase      ( const QString &currentRoot = QString() ); // shows known DB form

public slots:
  void addExistingDatabase( const QString &currentRoot = QString() ); // calls addDBConnection
  void addNewDatabase     ( const QString &currentRoot = QString() ); // calls addDBConnection

signals:
  void activeDatabaseChanged( QString );
  void userCancelledKnownDBForm();
  void reset();
  
private slots:
  void removeDBConnection( const QString &dbName );  // receives signal from DB form
  void setActiveDatabase ( const QString &dbName );  // receives signal from DB form

private:
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection    ( const QString &dbName );
  bool acceptSwitchReset();

  QWidget *m_parentWidget;
  QString  m_currentRoot;
  
};

#endif // GCDBSESSIONMANAGER_H
