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

#include <QMainWindow>
#include <QDomDocument>
#include <QThread>

namespace Ui {
class MainWindow;
}

class QTimer;
class QLabel;
class QtWaitingSpinner;

/*! \mainpage Goblin Coding's XML Mill
 *
 * \section intro_sec Introduction  Please
 * note that this is not a user manual or "Help" documentation, but rather
 * source documentation intended for use by developers or parties interested in
 * the code.  If you are a user and want to know more about the application and
 * its uses itself, the
 * <ahref="http://goblincoding.com/xmlmill/xmlmilloverview/">official
 * site</a>contains all the relevant information about this application. Please
 * also feel free to <a href="http://goblincoding.com/contact">contact me</a>
 * for any reason whatsoever.
 *
 * \section download Download  If you haven't yet, please
 * see the <ahref="http://goblincoding.com/xmlmilldownload">download page</a>
 * for a list ofpossible download options.  If you find any bugs or errors in
 * the code, or typo's in the documentation, please use the
 * <ahref="http://goblincoding.com/contact">contact form</a> to let me know. */
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  /*! Constructor. */
  explicit MainWindow(QWidget *parent = 0);

  /*! Destructor. */
  ~MainWindow();

public slots:
  /*! Connected to the \sa DB "result" signal. */
  void handleDBResult(DB::Result result, const QString &msg);

signals:
  /*! Emitted whenever an entire DOM document's XML must be processed. @param
   * xml - the DOM Doc's string representation. */
  void processDocumentXml(const QString &xml);

protected:
  /*! Re-implemented from QMainWindow.  Queries user to save before closing and
   * saves the user's "Options" preferences to settings. */
  void closeEvent(QCloseEvent *event);

private slots:
  /*! Triggered whenever the user decides to open an XML file. Once the XML
     import to the database has been successfully completed, the file is opened
     for editing.
      \sa newFile
      \sa saveFile
      \sa saveFileAs
      \sa closeFile */
  void openFile();

  /*! Triggered whenever the user decides to create a new XML file.
      \sa saveFile
      \sa saveFileAs
      \sa closeFile
      \sa openFile */
  void newFile();

  /*! Triggered whenever the user explicitly saves the current document and also
     for scenarios where saving the file is implied/logical (generally preceded
     by a query to the user to confirm the file save operation).
      \sa newFile
      \sa saveFileAs
      \sa closeFile
      \sa openFile */
  bool saveFile();

  /*! Triggered whenever the user explicitly wishes to save the current document
     with a specific name and also whenever the file save operation is requested
     without an active document name. Returns "false" when the file save
     operation is unsuccessful OR cancelled.
      \sa newFile
      \sa saveFile
      \sa closeFile
      \sa openFile */
  bool saveFileAs();

  /*! Triggered whenever the user explicitly wishes to close the current
     document.
      \sa newFile
      \sa saveFile
      \sa saveFileAs
      \sa openFile */
  void closeFile();

  /*! Connected to the "Find in Document" UI action. This function creates and
     displays an instance of SearchForm to allow the user to search for specific
     strings in the current document. */
  void searchDocument();

  /*! Connected to the "Forget Message Preferences" UI action.  This slot will
   * reset all saved user preferences regarding user input via message dialogs
   * and prompts. */
  void forgetMessagePreferences();

  /*! Resets the DOM and DOM related flags and cleans and clears all maps
     containing DOM element information.
      \sa queryResetDOM */
  void resetDOM();

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

  /*! Connected to the "Expand All" check box, expands or collapses the entire
   * tree. */
  void expandCollapse(bool expand);

private:
  /*! Pops open a QFileDialog for an "open file" name. Returns an empty string
     if user cancelled.
     \sa openFile */
  QString getOpenFileName();

  /*! Returns the contents of "fileName" as a QString if file opening was
   * successful, displays an error message and returns an empty string if not.
   * \sa openFile */
  QString readFile(const QString &fileName);

  /*! Sets the DomModel's content to the XML contained in "fileName" after a
     successful database batch process has completed.
      \sa openFile */
  void loadDocument();

  /*! Starts the timer responsible for the automatic saving of the current
   * document. */
  void startSaveTimer();

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

  /*! Reads the saved window state, geometry and theme settings from the
     registry/XML/ini file.
      \sa saveSettings */
  void readSavedSettings();

  /*! Saves the window state, geometry and theme settings to the
     registry/XML/ini file.
      \sa readSettings */
  void saveSettings();

  /*! Starts a separate database thread for batch processing entire DOM
   * documents.
   * \sa stopBatchProcessingThread */
  void startBatchProcessingThread();

  /*! Can be used to explicitly stop the batch processing thread started by \sa
   * startBatchProcessingThread */
  void stopBatchProcessingThread();

  /*! Set "save", "save as" and "close" actions to "enabled" */
  void enableFileActions(bool enabled);

  /*! Creates a pop-up label with a "waiting" message for use during long,
   * GUI-blocking processes. */
  QLabel *almostThere();

private:
  Ui::MainWindow *ui;
  QtWaitingSpinner *m_spinner;
  QTimer *m_saveTimer;

  QDomDocument m_domDoc;
  QDomDocument m_tmpDomDoc;

  QThread m_dbThread;
  DomModel m_model;

  QString m_currentXMLFileName;
  bool m_fileContentsChanged;
};

#endif // MAINWINDOW_H
