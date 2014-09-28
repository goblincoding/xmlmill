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
#include "model/dommodel.h"

#include <QDomDocument>
#include <QMainWindow>
#include <QThread>

namespace Ui {
class MainWindow;
}

class QTimer;
class DomModel;
class QtWaitingSpinner;

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

public slots:
  void handleDBResult(DB::Result result, const QString &msg);

signals:
  void processDocumentXml(const QString &domDoc);

protected:
  /*! Re-implemented from QMainWindow.  Queries user to save before closing and
   * saves the user's "Options" preferences to settings. */
  void closeEvent(QCloseEvent *event);

private slots:
  /*! Triggered whenever the user decides to open an XML file.
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  void openFile(const QString &fileName);

  /*! Triggered whenever the user decides to create a new XML file.
      \sa openXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  void newFile();

  /*! Triggered whenever the user explicitly saves the current document and also
     for scenarios where saving the file is implied/logical (generally preceded
     by a query to the user to confirm the file save operation).
      \sa newXMLFile
      \sa openXMLFile
      \sa saveXMLFileAs
      \sa closeXMLFile
      \sa importXMLFromFile */
  bool saveFile();

  /*! Triggered whenever the user explicitly wishes to save the current document
     with a specific name and also whenever the file save operation is requested
     without an active document name. Returns "false" when the file save
     operation is unsuccessful OR cancelled.
      \sa newXMLFile
      \sa saveXMLFile
      \sa openXMLFile
      \sa closeXMLFile
      \sa importXMLFromFile */
  bool saveFileAs();

  /*! Triggered whenever the user explicitly wishes to close the current
     document.
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa openXMLFile
      \sa importXMLFromFile */
  void closeFile();

  /*! Triggered by the "Import XML to Profile" UI action.
      \sa openXMLFile
      \sa newXMLFile
      \sa saveXMLFile
      \sa saveXMLFileAs
      \sa importXMLToDatabase */
  void importXMLFromFile();

  /*! Connected to the "Find in Document" UI action. This function creates and
     displays an instance of SearchForm to allow the user to search for specific
     strings in the current document.
      \sa itemFound */
  void searchDocument();

  /*! Connected to the "Forget Message Preferences" UI action.  This slot will
   * reset all saved user preferences regarding user input via message dialogs
   * and prompts. */
  void forgetMessagePreferences();

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

  /*! Saves a temporary file at 5 min intervals (when an active file is being
     edited) for auto-recovery purposes.
      \sa deleteTempFile
      \sa queryRestoreFiles */
  void saveTempFile();

  /*! If temporary files exist, it may be that the application (unlikely) or
     Windows (more likely) crashed while the user was working on a file.  In
     this case, ask the user if he/she would like to recover their work.
      \sa saveTempFile
      \sa deleteTempFile */
  void queryRestoreFiles();

  void expandCollapse(bool expand);

private:
  /*! Kicks off a recursive DOM tree traversal to populate the tree widget and
   * element maps with the information contained in the active DOM document. */
  QString readFile(const QString &fileName);

  /*! Displays a message in the status bar. */
  void setStatusBarMessage(const QString &message);

  /*! Starts the timer responsible for the automatic saving of the current
   * document. */
  void startSaveTimer();

  /*! Reads the saved window state, geometry and theme settings from the
     registry/XML/ini file.
      \sa saveSettings */
  void readSavedSettings();

  /*! Saves the window state, geometry and theme settings to the
     registry/XML/ini file.
      \sa readSettings */
  void saveSettings();

  /*! Called whenever an action may or will reset the DOM document and prompts
     the user to confirm that it's OK to do so (if not, the action won't be
     completed).
      \sa resetDOM */
  bool saveAndContinue(const QString &resetReason);

  /*! Delete the auto-recover temporary file every time the user changes or
     explicitly saves the active file.
      \sa saveTempFile
      \sa queryRestorefiles */
  void deleteTempFile();

  QString getOpenFileName();

  void setUpDBThread();

private:
  Ui::MainWindow *ui;
  QtWaitingSpinner *m_spinner;
  QDomDocument m_domDoc;
  QDomDocument m_tmpDomDoc;
  QTimer *m_saveTimer;
  QString m_currentXMLFileName;
  QString m_importedXmlFileName;
  QThread m_dbThread;

  DB m_db;
  DomModel m_model;

  bool m_fileContentsChanged;
};

#endif // MAINWINDOW_H
