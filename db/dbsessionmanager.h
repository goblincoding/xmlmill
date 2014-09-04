/* Copyright (c) 2012 - 2013 by William Hallatt.
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

#ifndef DBSESSIONMANAGER_H
#define DBSESSIONMANAGER_H

#include <QDialog>

namespace Ui
{
  class DBSessionManager;
}

/// Responsible for managing database connections and active database sessions.

/**
   This class is responsible for managing database connections and active database sessions
   and will prompt the user to confirm actions or changes that may result in the current DOM
   doc being reset.
*/
class DBSessionManager : public QDialog
{
Q_OBJECT

public:
  /*! Constructor. */
  explicit DBSessionManager( QWidget* parent = 0 );

  /*! Destructor. */
  ~DBSessionManager();

  /*! Select a known database from the dropdown, or add a new or existing database from file.
      @param currentRoot - used to determine whether or not the change will affect the active
                           document (if not provided, the current document is assumed empty). */
  void selectActiveDatabase( const QString& currentRoot = QString() );

  /*! Display the list of known databases that can be removed.
      @param currentRoot - used to determine whether or not the change will affect the active
                           document (if not provided, the current document is assumed empty). */
  void removeDatabase( const QString& currentRoot = QString() );

  public slots:
  /*! Add an existing database from file.
      @param currentRoot - used to determine whether or not the change will affect the active
                           document (if not provided, the current document is assumed empty). */
  void addExistingDatabase( const QString& currentRoot = QString() );

  /*! Create and add a new database.
      @param currentRoot - used to determine whether or not the change will affect the active
                           document (if not provided, the current document is assumed empty). */
  void addNewDatabase( const QString& currentRoot = QString() );

signals:
  /*! Emitted whenever the active database session is changed. */
  void activeDatabaseChanged( QString );

  /*! Emitted when the database change affects the current active document and
      informs the listener that the document must be reset. */
  void reset();

  private slots:
  /*! Triggered when the user selection is completed. Removes the selected database
      via DatabaseInterface. */
  void removeDatabaseConnection();

  /*! Triggered when the user selection is completed. Sets the selected database
      via DatabaseInterface.*/
  void setActiveDatabase();

  /*! Displays help specific to this form. */
  void showHelp();

private:
  /*! Sets the active database session to "dbName" via DatabaseInterface. */
  void setActiveDatabase( const QString& dbName );

  /*! Adds "dbName" to the list of known databases via DatabaseInterface. */
  void addDatabaseConnection( const QString& dbName );

  /*! Sets the list of known database names on the combo box. */
  void setDatabaseList();

  Ui::DBSessionManager* ui;
  QString m_currentRoot;
};

#endif // DBSESSIONMANAGER_H