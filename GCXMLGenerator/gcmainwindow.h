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
class QDomElement;
class QTimer;

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow( QWidget *parent = 0 );
  ~GCMainWindow();

private slots:
  /* Called only in "Super User" mode when a user edits the name of an
    existing tree widget item (i.e. element). An element with the new name
    will be added to the DB (if it doesn't yet exist) with the same associated
    attributes and attribute values as the element name it is replacing (the
    "old" element will not be removed from the DB). All occurrences of the old
    name throughout the current DOM will be replaced with the new name and the
    tree widget will be updated accordingly. */
  void treeWidgetItemChanged( QTreeWidgetItem *item, int column );

  /* Triggered by clicking on a tree widget item, the trigger will populate
    the table widget with the names of the attributes associated with the
    highlighted item's element as well as combo boxes containing their known
    values.  In "Super User" mode, this function will also create "empty" cells
    and combo boxes so that the user may add new attribute names to the selected
    element.  The addition of new attributes and values will automatically be
    persisted to the database. */
  void treeWidgetItemActivated( QTreeWidgetItem *item, int column );

  /* Called when a user clicks on/enters an attribute name cell in the table widget
    and sets the name of the current active attribute to that represented by the cell. */
  void setActiveAttributeName( QTableWidgetItem *item );

  /* This function is only called in "Super User" mode when the user changes the name
    of an existing attribute via the table widget.  The new attribute name will be
    persisted to the database (with the same known values of the "old" attribute) and
    associated with the current highlighted element.  The current DOM will be updated to
    reflect the new attribute name instead of the one that's been replaced. */
  void attributeNameChanged( QTableWidgetItem *item );

  /* Triggered whenever the current value of a combo box changes or when the user edits
    the content of a combo box.  In the first scenario, the DOM will be updated to reflect
    the new value for the specific element and associated attribute, in the latter case,
    the edited/provided value will be persisted to the database as a known value against
    the current element and associated attribute if it was previously unknown. */
  void attributeValueChanged( const QString &value );

  /* Called whenever the user enters or otherwise activates a combo box. */
  void setCurrentComboBox( QWidget *combo );

  /* These do exactly what you would expect. */
  void collapseOrExpandTreeWidget( bool checked );
  void switchSuperUserMode       ( bool super );
  void toggleShowDocContent      ( bool show );

  /* These slots are called by the corresponding signals from GCDBSessionManager. */
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

  /* Resets all user preferences to the initial default (to show all prompts). */
  void forgetAllMessagePreferences();
  
private:
  void showErrorMessageBox  ( const QString &errorMsg );
  void showLargeFileWarnings( qint64 fileSize );
  void setTextEditXML( const QDomElement &element );

  void resetTableWidget();
  void startSaveTimer();
  void toggleAddElementWidgets();

  void processDOMDoc();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );

  Ui::GCMainWindow    *ui;
  GCDBSessionManager  *m_dbSessionManager;
  QSignalMapper       *m_signalMapper;
  QDomDocument        *m_domDoc;
  QWidget             *m_currentCombo;
  QTimer              *m_saveTimer;
  QString              m_currentXMLFileName;
  QString              m_activeAttributeName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;
  bool                 m_wasTreeItemActivated;
  bool                 m_newElementWasAdded;
  bool                 m_busyImporting;
  bool                 m_DOMTooLarge;
  bool                 m_showDocContent;

  QHash< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QHash< QWidget*, int/* table row*/ >   m_comboBoxes;
};

#endif // GCMAINWINDOW_H
