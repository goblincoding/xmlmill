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
  /*! Constructor. */
  explicit GCDBSessionManager( QWidget *parent = 0 );

  /*! Destructor. */
  ~GCDBSessionManager();

  /*! Select a known database from the dropdown, or add a new or existing database from file.
      @param currentRoot - used to determine whether or not the change will affect
                           the active document. */
  void selectActiveDatabase(  const QString &currentRoot = QString() );

  /*! Display the list of known databases that can be removed.
      @param currentRoot - used to determine whether or not the change will affect
                           the active document. */
  void removeDatabase( const QString &currentRoot = QString() );

public slots:
  /*! Add an existing database from file.
      @param currentRoot - used to determine whether or not the change will affect
                           the active document. */
  void addExistingDatabase( const QString &currentRoot = QString() );

  /*! Create and add a new database.
      @param currentRoot - used to determine whether or not the change will affect
                           the active document. */
  void addNewDatabase( const QString &currentRoot = QString() );

signals:
  /*! Emitted whenever the active database session is changed. */
  void activeDatabaseChanged( QString );

  /*! Emitted when the database change affects the current active document and 
      informs the listener that the document was reset. */
  void reset();

private slots:
  /*! Triggered when the user selection is completed. Removes the selected database
      via GCDatabaseInterface. */
  void removeDBConnection();

  /*! Triggered when the user selection is completed. */
  void setActiveDatabase();

private:
  /*! Sets the active database session to "dbName" via GCDatabaseInterface. */
  void setActiveDatabase( const QString &dbName );

  /*! Adds "dbName" to the list of known databases via GCDatabaseInterface. */
  void addDBConnection( const QString &dbName );

  /*! Sets the list of known database names on the combo box. */
  void setDBList();
  
  Ui::GCDBSessionManager *ui;
  QString  m_currentRoot;
};

#endif // GCDBSESSIONMANAGER_H
