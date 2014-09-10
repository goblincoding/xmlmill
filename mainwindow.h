/* Copyright (c) 2012 - 2015 by William Hallatt.
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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "db/dbinterface.h"

#include <QMainWindow>
#include <QHash>
#include <QDomElement>

namespace Ui {
class MainWindow;
}

class TreeWidgetItem;
class QSignalMapper;
class QTableWidgetItem;
class QComboBox;
class QTimer;
class QLabel;
class QMovie;

/*! \mainpage Goblin Coding's XML Mill
 *
 * \section intro_sec Introduction  Please note that this is not a user manual
 *or "Help" documentation, but rather source documentation intended for use by
 *developers or parties interested in the code.  If you are a user and want to
 *know more about the application and its uses itself, the <a
 *href="http://goblincoding.com/xmlmill/xmlmilloverview/">official site</a>
 *contains all the relevant information about this application.
 *
 * Please also feel free to <a href="http://goblincoding.com/contact">contact
 *me</a> for any reason whatsoever.
 *
 * \section download Download  If you haven't yet, please see the <a
 *href="http://goblincoding.com/xmlmilldownload">download page</a> for a list of
 *possible download options.  If you find any bugs or errors in the code, or
 *typo's in the documentation, please use the <a
 *href="http://goblincoding.com/contact">contact form</a> to let me know. */

/// The main application window class.

/*! All the code refers to "databases" whereas all the user prompts reference
 * "profiles". This is deliberate.  In reality, everything is persisted to
 * SQLite database files, but a friend suggested that end users may be
 * intimidated by the use of the word "database" (especially if they aren't
 * necessarily technically inclined) and that "profile" may be less scary... I
 * agreed :) */
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  /*! Constructor. */
  explicit MainWindow(QWidget *parent = 0);

  /*! Destructor. */
  ~MainWindow();

protected:
  /*! Re-implemented from QMainWindow.  Queries user to save before closing and
   * saves the user's "Options" preferences to settings. */
  void closeEvent(QCloseEvent *event);

private slots:
  /*! Triggered as soon as the main event loop is entered (via a connection to a
   * single shot timer in the constructor). This function ensures that
   * DataBaseInterface is successfully initialised and prompts the user to
   * select a database for the current session. */
  void initialise();

  /*! Connected to the UI tree widget's "CurrentItemChanged( TreeWidgetItem*,
     int )" signal.
      \sa elementSelected */
  void elementChanged(TreeWidgetItem *item, int column);

  /*! Connected to the UI tree widget's "CurrentItemSelected( TreeWidgetItem*,
     int )" signal. The trigger will populate the table widget with the names of
     the attributes associated with the selected item's element as well as combo
     boxes containing their known values.  This function will also create
     "empty" cells and combo boxes so that the user may add new attribute names.
     The addition of new attributes and values will automatically be persisted
     to the active database.
      \sa elementChanged */
  void elementSelected(TreeWidgetItem *item, int column);

  /*! Connected to the UI table widget's "itemChanged( QTableWidgetItem* )"
     signal. This function is called when the user changes the name of an
     existing attribute via the table widget, or when the attribute's
     include/exclude state changes. The new attribute name will be persisted to
     the database (with the same known values of the "old" attribute) and
     associated with the current highlighted element. The current DOM will be
     updated to reflect the new attribute name instead of the one that was
     replaced.
      \sa attributeSelected
      \sa attributeValueChanged
      \sa setCurrentComboBox */
  void attributeChanged(QTableWidgetItem *tableItem);

  /*! Connected to the UI table widget's "itemClicked( QTableWidgetItem* )"
     signal. This function is called whenever the user selects an attribute in
     the table widget and keeps track of the active attribute and attribute
     name.
      \sa attributeChanged
      \sa attributeValueChanged
      \sa setCurrentComboBox */
  void attributeSelected(QTableWidgetItem *tableItem);

  /*! Connected to ComboBox's "currentIndexChanged( QString )" signal. Triggered
     whenever the current value of a combo box changes or when the user edits
     the content of a combo box.  In the first scenario, the DOM will be updated
     to reflect the new value for the specific element and associated attribute,
     in the latter case, the edited/provided value will be persisted to the
     database as a known value against the current element and associated
     attribute if it was previously unknown.
      \sa attributeSelected
      \sa attributeChanged
      \sa setCurrentComboBox */
  void attributeValueChanged(const QString &value);

  /*! Connected to the signal mapper's "mapped( QWidget* )" signal which is
     emitted every time a ComboBox is activated (whenever the user enters or
     otherwise activates a combo box). The active combo box is used to determine
     the row of the associated attribute (in the table widget), which in turn is
     required to determine which attribute must be updated when an attribute
     value changes.
      \sa attributeSelected
      \sa attributeValueChanged
      \sa attributeChanged */
  void setCurrentComboBox(QWidget *combo);

  /*! Triggered whenever the user decides to open an XML file.
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  bool openXMLFile();

  /*! Triggered whenever the user decides to create a new XML file.
      \sa openXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  void newXMLFile();

  /*! Triggered whenever the user explicitly saves the current document and also
     for scenarios where saving the file is implied/logical (generally preceded
     by a query to the user to confirm the file save operation).
      \sa newXMLFile
      \sa openXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  bool saveXMLFile();

  /*! Triggered whenever the user explicitly wishes to save the current document
     with a specific name and also whenever the file save operation is requested
     without an active document name. Returns "false" when the file save
     operation is unsuccessful OR cancelled.
      \sa newXMLFile
      \sa saveXMLFile
      \sa openXMLFile
      \sa closeXMLFile
      \sa importXMLFromFile */
  bool saveXMLFileAs();

  /*! Triggered whenever the user explicitly wishes to close the current
     document.
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa openXMLFile
      \sa importXMLFromFile */
  void closeXMLFile();

  /*! Saves a temporary file at 5 min intervals (when an active file is being
     edited) for auto-recovery purposes.
      \sa deleteTempFile
      \sa queryRestoreFiles */
  void saveTempFile();

  /*! Triggered by the "Import XML to Profile" UI action.
      \sa openXMLFile
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa importXMLToDatabase */
  void importXMLFromFile();

  /*! Connected to the "Add Element" button's "clicked()" signal. This function
     adds the new element (selected in the combo box) as a child to the current
     element or as a sibling in the case where the element of the same name and
     with the square bracket syntax is selected in the combo.  In other words,
     if the current element is "MyElement", then selecting "[MyElement]"
     from the combo will add another MyElement element as a sibling to the
     currently active element.
      \sa addSnippetToDocument
      \sa insertSnippet */
  void addElementToDocument();

  /*! Connected to the "Add Snippet" button's "clicked()" signal. This function
     creates and displays an instance of AddSnippetsForm to allow the user to
     add one (or more) XML snippets to the active document.
      \sa addElementToDocument
      \sa insertSnippet */
  void addSnippetToDocument();

  /*! Connected to the AddSnippetsForm's "snippetAdded()" signal.  This function
     updates the GUI whenever new snippets are added to the active document.
      \sa addElementToDocument
      \sa addSnippetToDocument */
  void insertSnippet(TreeWidgetItem *treeItem, QDomElement element);

  /*! Triggered by the "Remove Items" UI action. This function creates and
     displays an instance of RemoveItemsForm to allow the user to remove
     elements and/or attributes
     from the active database.
      \sa addItemsToDB */
  void removeItemsFromDB();

  /*! Triggered by the "Add Items" UI action. This function creates and displays
     an instance of AddItemsForm to allow the user to add elements and/or
     attributes to the active database.
      \sa removeItemsFromDB */
  void addItemsToDB();

  /*! Connected to the "Find in Document" UI action. This function creates and
     displays an instance of SearchForm to allow the user to search for specific
     strings in the current document.
      \sa itemFound */
  void searchDocument();

  /*! Connected to SearchForm's "foundItem" signal.  This slot sets the found
     item as active.
      \sa searchDocument */
  void itemFound(TreeWidgetItem *item);

  /*! Connected to PlainTextEdit's "commentOut" signal. Removes the items with
   * indices matching those in the parameter list from the tree as well as from
   * the DOM document and replaces their XML with that of a comment node
   * containing the (well-formed) "comment" string. */
  void commentOut(const QList<int> &indices, const QString &comment);

  /*! Connected to PlainTextEdit's "manualEditAccepted()" signal. Rebuilds the
   * XML hierarchy for the special occasions where a manual user edit is
   * allowed. */
  void rebuild();

  /*! Connected to the comment line edit's "textEdited" signal, this updates the
   * active comment node's value to "comment".  This function will not execute
   * when new comments or elements are added. */
  void updateComment(const QString &comment);

  /*! Connectd to the "Expand All" checkbox's "clicked( bool )" signal.  This
   * slot toggles the expansion and collapse of the UI tree widget. */
  void collapseOrExpandTreeWidget(bool checked);

  /*! Unchecks the "Expand All" checkbox as soon as any of the tree items are *
   * collapsed. */
  void uncheckExpandAll();

  /*! Connected to the "Forget Message Preferences" UI action.  This slot will
   * reset all saved user preferences regarding user input via message dialogs
   * and prompts. */
  void forgetMessagePreferences();

  /*! Creates and displays a "loading" style spinner for use during expensive
     operations. The calling function is responsible for clean-up (preferably
     through calling deleteSpinner() ).
      \sa deleteSpinner */
  void createSpinner();

  /*! Resets the DOM and DOM related flags and cleans and clears all maps
     containing DOM element information.
      \sa queryResetDOM */
  void resetDOM();

  /*! Triggered when the "empty profile help" button is clicked.  This button is
   * only shown when the active profile is empty and provides information that
   * will help the user populate the active profile. */
  void showEmptyProfileHelp();

  /*! Displays a brief message about adding elements to a document. */
  void showElementHelp();

  /*! Connected to the "Help Contents" action.  Displays the main help page. */
  void showMainHelp();

  /*! Decides whether or not to display help buttons throughout the entire
   * application. */
  void setShowHelpButtons(bool show);

  /*! Decides whether or not to display tree items' elements "verbose"
   * throughout the entire application. */
  void setShowTreeItemsVerbose(bool verbose);

  /*! Opens this application's website. */
  void goToSite();

  /*! Sets the "dark theme" style sheet on the application. */
  void useDarkTheme(bool dark);

private:
  /*! Kicks off a recursive DOM tree traversal to populate the tree widget and
   * element maps with the information contained in the active DOM document. */
  void processDOMDoc();

  /*! Displays a message in the status bar. */
  void setStatusBarMessage(const QString &message);

  /*! Displays the DOM document's content in the text edit area.
      \sa highlightTextElement */
  void setTextEditContent(TreeWidgetItem *item = 0);

  /*! Highlights the currently active DOM element in the text edit area.
      \sa setTextEditContent */
  void highlightTextElement(TreeWidgetItem *item);

  /*! Creates an additional empty table row each time the table widget is
   * populated so that the user may add new attributes to the active element. */
  void insertEmptyTableRow();

  /*! Cleans up and clears the table widget. */
  void resetTableWidget();

  /*! Starts the timer responsible for the automatic saving of the current
   * document. */
  void startSaveTimer();

  /*! Activates or deactivates the add element combo box and buttons when the
   * profile is empty or when the active element doesn't have first level
   * children. */
  void toggleAddElementWidgets();

  /*! Reads the saved window state, geometry and theme settings from the
     registry/XML/ini file.
      \sa saveSettings */
  void readSettings();

  /*! Saves the window state, geometry and theme settings to the
     registry/XML/ini file.
      \sa readSettings */
  void saveSettings();

  /*! Called whenever an action may or will reset the DOM document and prompts
     the user to confirm that it's OK to do so (if not, the action won't be
     completed).
      \sa resetDOM */
  bool queryResetDOM(const QString &resetReason);

  /*! Imports the DOM content to the active database.
      \sa importXMLFromFile */
  bool importXMLToDatabase();

  /*! Deletes the "busy loading" spinner.
      \sa createSpinner */
  void deleteSpinner();

  /*! If temporary files exist, it may be that the application (unlikely) or
     Windows (more likely) crashed while the user was working on a file.  In
     this case, ask the user if he/she would like to recover their work.
      \sa saveTempFile
      \sa deleteTempFile */
  void queryRestoreFiles();

  /*! Delete the auto-recover temporary file every time the user changes or
     explicitly saves the active file.
      \sa saveTempFile
      \sa queryRestorefiles */
  void deleteTempFile();

private:
  Ui::MainWindow *ui;
  QSignalMapper *m_signalMapper;
  QTableWidgetItem *m_activeAttribute;
  QWidget *m_currentCombo;
  QTimer *m_saveTimer;
  QLabel *m_activeProfileLabel;
  QLabel *m_progressLabel;
  QMovie *m_spinner;
  QString m_currentXMLFileName;
  QString m_activeAttributeName;

  DB m_db;

  bool m_wasTreeItemActivated;
  bool m_newAttributeAdded;
  bool m_busyImporting;
  bool m_fileContentsChanged;

  QHash<QWidget *, int /* table row*/> m_comboBoxes;
};

#endif // MAINWINDOW_H
