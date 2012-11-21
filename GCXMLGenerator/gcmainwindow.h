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

#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QHash>

/*--------------------------------------------------------------------------------------*/
/*
  All the code refers to "database" whereas all the user prompts reference "profiles". This
  is deliberate.  In reality, everything is persisted to SQLite database files, but a friend
  suggested that end users may be intimidated by the use of the word "database" (especially
  if they aren't necessarily technically inclined) and that "profiles" may be less scary :)
*/

/*--------------------------------------------------------------------------------------*/

namespace Ui {
  class GCMainWindow;
}

class GCDBSessionManager;
class QSignalMapper;
class QTreeWidgetItem;
class QTableWidgetItem;
class QComboBox;
class QDomDocument;
class QSettings;
class QDomElement;
class QTimer;

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow( QWidget *parent = 0 );
  ~GCMainWindow();

private slots:
  void treeWidgetItemChanged     ( QTreeWidgetItem *item, int column );
  void treeWidgetItemActivated   ( QTreeWidgetItem *item, int column );

  void setActiveAttributeName    ( QTableWidgetItem *item );
  void attributeNameChanged      ( QTableWidgetItem *item );
  void attributeValueChanged     ( const QString &value );

  void setCurrentComboBox        ( QWidget *combo );
  void collapseOrExpandTreeWidget( bool checked );
  void switchSuperUserMode       ( bool super );
  void toggleShowDocContent      ( bool show );

  void userCancelledKnownDBForm();
  void dbSessionChanged();

  /* XML file related. */
  void newXMLFile();
  void openXMLFile();
  void saveXMLFile();
  void saveXMLFileAs();

  /* Database related. */
  void switchDBSession();
  void importXMLToDatabase();

  /* DOM and DB. */
  void resetDOM();
  void showNewElementForm();

  void deleteElementFromDOM();
  void addChildElementToDOM();

  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revertDirectEdit();
  void saveDirectEdit();

  /* Receives new element information from "GCElementForm". */
  void addNewElement( const QString &element, const QStringList &attributes );

  /* Receives user preference regarding future displays of a specific message
    from "GCMessageDialog". */
  void rememberPreference( bool remember );
  void forgetAllMessagePreferences();
  void saveSetting( const QString &key, const QVariant &value );
  
private:
  void showErrorMessageBox( const QString &errorMsg );

  void setTextEditXML( const QDomElement &element );
  void showLargeFileWarnings( qint64 fileSize );
  void resetTableWidget();
  void startSaveTimer();
  void toggleAddElementWidgets();

  void processDOMDoc();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );

  Ui::GCMainWindow    *ui;
  GCDBSessionManager  *m_dbSessionManager;
  QSignalMapper       *m_signalMapper;
  QDomDocument        *m_domDoc;
  QSettings           *m_settings;
  QWidget             *m_currentCombo;
  QTimer              *m_saveTimer;
  QString              m_currentXMLFileName;
  QString              m_activeAttributeName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;
  bool                 m_wasTreeItemActivated;
  bool                 m_newElementWasAdded;
  bool                 m_rememberPreference;
  bool                 m_busyImporting;
  bool                 m_DOMTooLarge;
  bool                 m_showDocContent;

  QHash< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QHash< QWidget*, int/* table row*/ >   m_comboBoxes;
  QHash< QString /*setting name*/, QVariant /*message*/ > m_messages;

};

#endif // GCMAINWINDOW_H
