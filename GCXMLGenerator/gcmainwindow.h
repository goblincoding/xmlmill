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

#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QHash>

namespace Ui
{
  class GCMainWindow;
}

class GCDBSessionManager;
class GCDomElementInfo;
class QSignalMapper;
class QTreeWidgetItem;
class QTableWidgetItem;
class QComboBox;
class QDomDocument;
class QDomElement;
class QTimer;
class QLabel;
class QMovie;

/// The main application window class.

/*!
  All the code refers to "databases" whereas all the user prompts reference "profiles". This
  is deliberate.  In reality, everything is persisted to SQLite database files, but a friend
  suggested that end users may be intimidated by the use of the word "database" (especially
  if they aren't necessarily technically inclined) and that "profile" may be less scary :)
*/
class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  /*! Constructor. */
  explicit GCMainWindow( QWidget *parent = 0 );

  /*! Destructor. */
  ~GCMainWindow();

protected:
  /*! Re-implemented from QMainWindow.  Queries user to save before closing and
      saves the window's geometry and position. */
  void closeEvent( QCloseEvent * event );

private slots:
  /*! Triggered as soon as the main event loop is entered (via a connection to single shot timer in the constructor).
      This function ensures that GCDataBaseInterface is successfully initialised and prompts the user to select
      a database for the current session. */
  void initialise();

  /*! Connected to the UI tree widget's "itemChanged( QTreeWidgetItem*, int )" signal.
      Called only when a user edits the name of an existing tree widget item
      (i.e. element). An element with the new name will be added to the DB
      (if it doesn't yet exist) with the same associated attributes and attribute
      values as the element name it is replacing (the "old" element will not be
      removed from the DB). All occurrences of the old name throughout the current
      DOM will be replaced with the new name and the tree widget will be updated
      accordingly. */
  void elementChanged( QTreeWidgetItem *item, int column );

  /*! Connected to the UI tree widget's "itemClicked( QTreeWidgetItem*, int )" signal.
      Triggered by clicking on a tree widget item, the trigger will populate
      the table widget with the names of the attributes associated with the
      highlighted item's element as well as combo boxes containing their known
      values.  This function will also create "empty" cells and combo boxes so
      that the user may add new attribute names to the selected element.  The
      addition of new attributes and values will automatically be persisted
      to the database. */
  void elementSelected( QTreeWidgetItem *item, int column );

  /*! Connected to the UI table widget's "itemChanged( QTableWidgetItem* )" signal.
      This function is called when the user changes the name of an existing attribute
      via the table widget, or when the attribute's include/exclude state changes. The new
      attribute name will be persisted to the database (with the same known values of
      the "old" attribute) and associated with the current highlighted element.  The current
      DOM will be updated to reflect the new attribute name instead of the one that was replaced. */
  void attributeChanged( QTableWidgetItem *item );

  /*! Connected to the UI table widget's "itemClicked( QTableWidgetItem* )" signal.
      This function is called whenever the user selects an attribute in the table widget and
      keeps track of the active attribute and attribute name. */
  void attributeSelected( QTableWidgetItem *item );

  /*! Connected to GCComboBox's "currentIndexChanged( QString )" signal.
      Triggered whenever the current value of a combo box changes or when the user edits
      the content of a combo box.  In the first scenario, the DOM will be updated to reflect
      the new value for the specific element and associated attribute, in the latter case,
      the edited/provided value will be persisted to the database as a known value against
      the current element and associated attribute if it was previously unknown. */
  void attributeValueChanged( const QString &value );

  /*! Connected to the signal mapper's "mapped( QWidget* )" signal which is emitted every
      time a GCComboBox is activated (whenever the user enters or otherwise activates a combo box).
      The active combo box is used to determine the row of the associated attribute (in the table
      widget), which in turn is required to determine which attribute must be updated when an attribute
      value changes. */
  void setCurrentComboBox( QWidget *combo );

  /*! Triggered whenever the user decides to open an XML file. */
  bool openXMLFile();

  /*! Triggered whenever the user decides to create a new XML file. */
  void newXMLFile();

  /*! Triggered whenever the user explicitly saves the current document and also for scenarios
      where saving the file is implied/logical (generally preceded by a query to the user to
      confirm the file save operation). */
  void saveXMLFile();

  /*! Triggered whenever the user explicitly wishes to save the current document with a specific
      name and also whenever the file save operation is requested without an active document name. */
  void saveXMLFileAs();

  /*! Triggered by the "Add New Profile" UI action. */
  void addNewDatabase();

  /*! Triggered by the "Add Existing Profile" UI action. */
  void addExistingDatabase();

  /*! Triggered by the "Remove Profile" UI action. */
  void removeDatabase();

  /*! Triggered by the "Switch Profile" UI action. */
  void switchActiveDatabase();

  /*! Connected to GCDBSessionManager's "activeDatabaseChanged( QString )" signal. */
  void activeDatabaseChanged( QString dbName );

  /*! Triggered by the "Import XML to Profile" UI action. */
  void importXMLFromFile();

  /*! Connected to the "Remove Highlighted Element" button's "clicked()" signal. This function
      deletes the highlighted element (and its child element tree) from the current document without
      affecting the active database in any way. */
  void deleteElementFromDocument();

  /*! Connected to the "Add Child Element" button's "clicked()" signal. This function adds the new
      element (selected in the combo box) as a child to the current element or as a sibling in the
      case where the element of the same name and with the angular bracket syntax is selected in the
      combo.  In other words, if the current element is "MyElement, then selecting "<MyElement>" from
      the combo will add another MyElement element as a sibling to the currently active element. */
  void addElementToDocument();

  /*! Connected to the "Add Snippet" button's "clicked()" signal. This function creates and displays
      an instance of GCSnippetsForm to allow the user to add one (or more) XML snippets to the active
      document. */
  void addSnippetToDocument();

  /*! Connected to the GCSnippetsForm's "snippetAdded()" signal.  This function updates the GUI whenever
      new snippets are added to the active document. */
  void insertSnippet();

  /*! Triggered by the "Remove Items" UI action. This function creates and displays an instance of
      GCRemoveItemsForm to allow the user to remove elements and/or attributes from the active database. */
  void removeItemsFromDB();

  /*! Triggered by the "Add Items" UI action. This function creates and displays an instance of
      GCAddItemsForm to allow the user to add elements and/or attributes to the active database. */
  void addItemsToDB();

  /*! Connected to the "Find in Document" UI action. This function creates and displays an instance of
      GCSearchForm to allow the user to search for specific strings in the current document. */
  void searchDocument();

  /*! Connected to GCSearchForm's "foundElement( QDomElement)" signal.  This slot sets the "found" element
      corresponding to the user-provided search string as active. */
  void elementFound( const QDomElement &element );

  /*! Connected to the "Revert Manual Changes" button's "clicked()" signal.  This slot will revert all manual
      changes to the active document made BEFORE the changes are saved (i.e. it isn't a classic "undo" button). */
  void revertDirectEdit();

  /*! Connected to the "Save Manual Changes" button's "clicked()" signal.  This slot will save all manual
      changes made to the active document provided that the changes are valid XML.  Triggering this slot also results
      in the current document being imported (or re-imported) to the active database since these changes may
      contain information about elements and attributes that didn't previously exist and therefore aren't known to the
      active database). */
  void saveDirectEdit();

  /*! Connectd to the "Expand All" checkbox's "clicked( bool )" signal.  This slot toggles the expandsion or collapses
      of UI tree widget. */
  void collapseOrExpandTreeWidget( bool checked );

  /*! Connected to the "Forget Message Preferences" UI action.  This slot will reset all saved user preferences
      regarding user input via message dialogs and prompts. */
  void forgetMessagePreferences();

  /*! Creates and displays a "loading" style spinner for use during expensive operations. The calling function
      is responsible for clean-up (preferably through calling deleteSpinner() ). */
  void createSpinner();

  /*! Resets the DOM and DOM related flags and cleans and clears all maps containing DOM element information. */
  void resetDOM();

  void showEmptyProfileHelp();
  void showDOMEditHelp();
  void showMainHelp();
  void goToSite();
  
private:
  void processDOMDoc();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );

  void setStatusBarMessage( const QString &message );
  void showErrorMessageBox( const QString &errorMsg );
  void setTextEditContent( const QDomElement &element );
  void highlightTextElement( const QDomElement &element );

  /*! Prompts the user to select a DB for the current session.  This function is called whenever
      it is determined that no active session exists and will keep prompting the user until a
      database (profile) is selected as active. */
  void querySetActiveSession( QString reason );

  bool queryResetDOM( const QString &resetReason );

  bool importXMLToDatabase();
  void insertEmptyTableRow();
  void resetTableWidget();
  void deleteElementInfo();
  void startSaveTimer();
  void toggleAddElementWidgets();
  void deleteSpinner();
  void readSettings();
  void saveSettings();

  GCDBSessionManager *createDBSessionManager();

  Ui::GCMainWindow *ui;
  QSignalMapper    *m_signalMapper;
  QDomDocument     *m_domDoc;
  QTableWidgetItem *m_activeAttribute;
  QWidget          *m_currentCombo;
  QTimer           *m_saveTimer;
  QLabel           *m_activeSessionLabel;
  QLabel           *m_progressLabel;
  QMovie           *m_spinner;
  QString           m_currentXMLFileName;
  QString           m_activeAttributeName;
  bool              m_wasTreeItemActivated;
  bool              m_newAttributeAdded;
  bool              m_busyImporting;
  bool              m_fileContentsChanged;

  QHash< QTreeWidgetItem*, GCDomElementInfo* > m_elementInfo;
  QHash< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QHash< QWidget*, int/* table row*/ >   m_comboBoxes;

};

#endif // GCMAINWINDOW_H
