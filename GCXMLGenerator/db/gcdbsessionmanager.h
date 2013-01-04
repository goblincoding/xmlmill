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

#include <QDialog>

namespace Ui
{
  class GCDBSessionManager;
}

/// Responsible for managing database connections and active database sessions.

/**
   This class is responsible for managing database connections and active database sessions
   and will prompt the user to confirm actions or changes that may result in the current DOM
   doc being reset.
*/
class GCDBSessionManager : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCDBSessionManager( QWidget *parent = 0 );
  ~GCDBSessionManager();

  void selectActiveDatabase();
  void switchActiveDatabase( const QString &currentRoot = QString() );
  void removeDatabase( const QString &currentRoot = QString() );

public slots:
  void addExistingDatabase( const QString &currentRoot = QString() );
  void addNewDatabase( const QString &currentRoot = QString() );

signals:
  void activeDatabaseChanged( QString );
  void reset();

private slots:
  void removeDBConnection();
  void setActiveDatabase();

private:
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection( const QString &dbName );
  
  Ui::GCDBSessionManager *ui;
  QString  m_currentRoot;
};

#endif // GCDBSESSIONMANAGER_H
