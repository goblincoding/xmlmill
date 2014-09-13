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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "forms/additemsform.h"
#include "forms/removeitemsform.h"
#include "forms/helpdialog.h"
#include "forms/searchform.h"
#include "forms/addsnippetsform.h"
#include "forms/restorefilesform.h"
#include "utils/treewidgetitem.h"
#include "utils/combobox.h"
#include "utils/messagespace.h"
#include "utils/globalsettings.h"

#include <QDesktopServices>
#include <QSignalMapper>
#include <QTextBlock>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include <QUrl>
#include <QCloseEvent>
#include <QFont>
#include <QScrollBar>
#include <QMovie>
#include <QSettings>
#include <QXmlInputSource>

/*----------------------------------------------------------------------------*/

const QString EMPTY("---");
const QString LEFTRIGHTBRACKETS("\\[|\\]");

const qint64 DOMWARNING(262144); // 0.25MB or ~7 500 lines

/*----------------------------------------------------------------------------*/

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_saveTimer(nullptr),
      m_spinner(nullptr), m_progressLabel(nullptr), m_currentXMLFileName(""),
      m_db(), m_fileContentsChanged(false) {
  ui->setupUi(this);

  /* XML File related. */
  connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
  connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
  connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFile()));
  connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveFileAs()));
  connect(ui->actionCloseFile, SIGNAL(triggered()), this, SLOT(closeFile()));

  /* Various other actions. */
  connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
  connect(ui->actionForgetPreferences, SIGNAL(triggered()), this,
          SLOT(forgetMessagePreferences()));
  connect(ui->actionHelpContents, SIGNAL(triggered()), this,
          SLOT(showMainHelp()));
  connect(ui->actionVisitOfficialSite, SIGNAL(triggered()), this,
          SLOT(goToSite()));
  connect(ui->actionUseDarkTheme, SIGNAL(triggered(bool)), this,
          SLOT(useDarkTheme(bool)));

  /* Help related. */
  connect(ui->actionShowHelpButtons, SIGNAL(triggered(bool)), this,
          SLOT(setShowHelpButtons(bool)));

  /* Database related. */
  connect(ui->actionImportXMLToDatabase, SIGNAL(triggered()), this,
          SLOT(importXMLFromFile()));

  // TODO - connect DB action signal to error handling slot!!!

  readSettings();

  /* Wait for the event loop to be initialised before calling this function. */
  QTimer::singleShot(0, this, SLOT(queryRestoreFiles()));
}

/*----------------------------------------------------------------------------*/

MainWindow::~MainWindow() { delete ui; }

/*----------------------------------------------------------------------------*/

void MainWindow::closeEvent(QCloseEvent *event) {
  if (m_fileContentsChanged) {
    QMessageBox::StandardButtons accept = QMessageBox::question(
        this, "Save File?", "Save changes before closing?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (accept == QMessageBox::Yes) {
      saveFile();
    } else if (accept == QMessageBox::Cancel) {
      event->ignore();
      return;
    }
  }

  deleteTempFile();
  saveSettings();
  QMainWindow::closeEvent(event);
}

/*----------------------------------------------------------------------------*/

QString MainWindow::getOpenFileName() {
  /* Start off where the user finished last. */
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open File", GlobalSettings::lastUserSelectedDirectory(),
      "XML Files (*.*)");

  /* Save whatever directory the user ended up in (provided the user didn't
   * cancel the operation). */
  if (!fileName.isEmpty()) {
    QFileInfo fileInfo(fileName);
    QString finalDirectory = fileInfo.dir().path();
    GlobalSettings::setLastUserSelectedDirectory(finalDirectory);
  }

  return fileName;
}

/*----------------------------------------------------------------------------*/

void MainWindow::openFile() {
  if (!saveAndContinue("Save document before continuing?")) {
    return;
  }

  m_currentXMLFileName = getOpenFileName();
  m_fileContentsChanged = false; // at first load, nothing has changed
}

/*----------------------------------------------------------------------------*/

void MainWindow::newFile() {
  if (saveAndContinue("Save document before continuing?")) {
    resetDOM();
    m_currentXMLFileName = "";
    ui->actionCloseFile->setEnabled(true);
    ui->actionSaveAs->setEnabled(true);
    ui->actionSave->setEnabled(true);
  }
}

/*----------------------------------------------------------------------------*/

bool MainWindow::saveFile() {
  if (m_currentXMLFileName.isEmpty()) {
    return saveFileAs();
  } else {
    QFile file(m_currentXMLFileName);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate |
                   QIODevice::Text)) {
      QString errMsg = QString("Failed to save file \"%1\": [%2].")
                           .arg(m_currentXMLFileName)
                           .arg(file.errorString());
      MessageSpace::showErrorMessageBox(this, errMsg);
      return false;
    } else {
      QTextStream outStream(&file);
      // outStream << ui->treeWidget->toString();
      file.close();

      m_fileContentsChanged = false;
      deleteTempFile();
      startSaveTimer();
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

bool MainWindow::saveFileAs() {
  QString file = QFileDialog::getSaveFileName(
      this, "Save As", GlobalSettings::lastUserSelectedDirectory(),
      "XML Files (*.*)");

  /* If the user clicked "OK". */
  if (!file.isEmpty()) {
    m_currentXMLFileName = file;

    /* Save the last visited directory. */
    QFileInfo fileInfo(m_currentXMLFileName);
    QString finalDirectory = fileInfo.dir().path();
    GlobalSettings::setLastUserSelectedDirectory(finalDirectory);

    return saveFile();
  }

  /* Return false when the file save operation is cancelled so that
   * "queryResetDom" does not inadvertently cause a file to be reset by
   * accident when the user changes his/her mind. */
  return false;
}

/*----------------------------------------------------------------------------*/

void MainWindow::closeFile() {
  if (saveAndContinue("Save document before continuing?")) {
    resetDOM();
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::importXMLFromFile() {
  QString fileName = getOpenFileName();

  if (!fileName.isEmpty()) {
    QFile file(fileName);

    /* This application isn't optimised for dealing with very large XML files
     * (the
     * entire point is that this suite should provide the functionality
     * necessary
     * for the manual manipulation of, e.g. XML config files normally set up by
     * hand via copy and paste exercises), if this file is too large to be
     * handled
     * comfortably, we need to let the user know and also make sure that we
     * don't
     * try to set the DOM content as text in the QTextEdit (QTextEdit is
     * optimised
     * for paragraphs). */
    qint64 fileSize = file.size();

    if (fileSize > DOMWARNING) {
      QMessageBox::warning(this, "Large file!", "The file you just opened is "
                                                "pretty large. Response times "
                                                "may be slow.");
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream inStream(&file);
      QString fileContent(inStream.readAll());
      file.close();
      importXMLToDatabase(fileContent);

      QMessageBox::StandardButtons accepted = QMessageBox::question(
          this, "Edit file", "Also open file for editing?",
          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

      if (accepted == QMessageBox::Yes) {
        loadFile();
      }
    } else {
      QString errorMsg = QString("Failed to open file \"%1\": [%2]")
                             .arg(fileName)
                             .arg(file.errorString());
      MessageSpace::showErrorMessageBox(this, errorMsg);
    }
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::importXMLToDatabase(const QString &xml) {
  QXmlInputSource source;
  source.setData(xml);
  QXmlSimpleReader reader;

  QString xmlErr("");
  int line(-1);
  int col(-1);

  if (m_domDoc.setContent(&source, &reader, &xmlErr, &line, &col)) {
    createSpinner();
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    m_db.processDomDocument(m_domDoc);
    deleteSpinner();
  } else {
    // ERROR HANDLING TO ADD
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::searchDocument() {
  /* Delete on close flag set (no clean-up needed). */
  //  SearchForm *form = new SearchForm(ui->treeWidget->allTreeWidgetItems(),
  //                                    ui->dockWidgetTextEdit, this);
  //  connect(form, SIGNAL(foundItem(TreeWidgetItem *)), this,
  //          SLOT(itemFound(TreeWidgetItem *)));
  //  form->exec();
}

/*----------------------------------------------------------------------------*/

void MainWindow::forgetMessagePreferences() {
  MessageSpace::forgetAllPreferences();
}

/*----------------------------------------------------------------------------*/

void MainWindow::createSpinner() {
  /* Clean-up should be handled in the calling function, but just in case. */
  if (m_spinner || m_progressLabel) {
    deleteSpinner();
  }

  m_progressLabel = new QLabel(this, Qt::Popup);
  m_progressLabel->move(window()->frameGeometry().topLeft() +
                        window()->rect().center() -
                        m_progressLabel->rect().center());

  m_spinner = new QMovie(":/resources/spinner.gif");
  m_progressLabel->setMovie(m_spinner);

  /* Delay the display as it could happen that the spinner gets created and
   * subsequently destroyed very shortly after each other (the process we are
   * monitoring might very well be executing faster than expected). */
  QTimer timer;
  timer.singleShot(1000, this, SLOT(showSpinner()));
}

/*----------------------------------------------------------------------------*/

void MainWindow::deleteSpinner() {
  delete m_spinner;
  m_spinner = nullptr;

  delete m_progressLabel;
  m_progressLabel = nullptr;
}

/*----------------------------------------------------------------------------*/

void MainWindow::showSpinner() {
  if (m_spinner && m_progressLabel) {
    m_spinner->start();
    m_progressLabel->show();
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::resetDOM() {
  m_domDoc.clear();
  m_fileContentsChanged = false;

  /* The timer will be reactivated as soon as work starts again on a legitimate
   * document and the user saves it for the first time. */
  if (m_saveTimer) {
    m_saveTimer->stop();
  }
}

/*----------------------------------------------------------------------------*/

bool MainWindow::saveAndContinue(const QString &resetReason) {
  /* There are a number of places and opportunities for "resetDOM" to be called,
   * if there is an active document, check if it's content has changed since the
   * last time it had changed and make sure we don't accidentally delete
   * anything. */
  if (m_fileContentsChanged) {
    QMessageBox::StandardButtons accept = QMessageBox::question(
        this, "Save file?", resetReason,
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (accept == QMessageBox::Yes) {
      return saveFile();
    } else if (accept == QMessageBox::No) {
      m_fileContentsChanged = false;
    } else {
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

void MainWindow::showEmptyProfileHelp() {
  QMessageBox::information(
      this, "Empty Profile",
      "The active profile is empty.  You can either import XML from file "
      "via \"Edit -> Import XML to Profile\" or you can populate the "
      "profile from scratch via \"Edit -> Edit Profile -> Add Items\".");
}

/*----------------------------------------------------------------------------*/

void MainWindow::showElementHelp() {
  QMessageBox::information(this, "Adding Elements and Snippets",
                           "If the document is still empty, your first element "
                           "will be the root element.\n\n"
                           "New elements are added as children of the element "
                           "selected in the element tree.\n\n"
                           "\"Empty\" duplicate siblings are added by "
                           "selecting the element name in the drop down "
                           "that matches that of the element selected in the "
                           "element tree (these names will be "
                           "bracketed by \"[]\").\n\n"
                           "\"Snippets\" are fully formed XML segments "
                           "consisting of the entire element hierarchy "
                           "with the element selected in the drop down combo "
                           "box as base. Selecting this option will "
                           "provide you with the opportunity to provide "
                           "default values for the snippet's attributes.");
}

/*----------------------------------------------------------------------------*/

void MainWindow::showMainHelp() {
  QFile file(":/resources/help/Help.txt");

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    MessageSpace::showErrorMessageBox(
        this,
        QString("Failed to open \"Help\" file: [%1]").arg(file.errorString()));
  } else {
    QTextStream stream(&file);
    QString fileContent = stream.readAll();
    file.close();

    /* Qt::WA_DeleteOnClose flag set...no cleanup required. */
    HelpDialog *dialog = new HelpDialog(fileContent, this);
    dialog->show();
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::goToSite() {
  QDesktopServices::openUrl(QUrl("http://goblincoding.com"));
}

/*----------------------------------------------------------------------------*/

void MainWindow::useDarkTheme(bool dark) {
  if (dark) {
    QFile file(":resources/dark.txt");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);

    qApp->setStyleSheet(stream.readAll());
    file.close();
  } else {
    qApp->setStyleSheet(QString());
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::setShowHelpButtons(bool show) {
  GlobalSettings::setShowHelpButtons(show);
}

/*----------------------------------------------------------------------------*/

void MainWindow::setShowTreeItemsVerbose(bool verbose) {
  GlobalSettings::setShowTreeItemsVerbose(verbose);
}

/*----------------------------------------------------------------------------*/

void MainWindow::loadFile() {
  /* Enable file save options. */
  ui->actionCloseFile->setEnabled(true);
  ui->actionSave->setEnabled(true);
  ui->actionSaveAs->setEnabled(true);

  /* Generally-speaking, we want the file contents changed flag to be set
   * whenever the text edit content is set (this is done, not surprisingly, in
   * "setTextEditContent" above).  However, whenever a DOM document is processed
   * for the first time, nothing is changed in it, so to avoid the annoying
   * "Save File" queries when nothing has been done yet, we unset the flag here.
   */
  m_fileContentsChanged = false;
}

/*----------------------------------------------------------------------------*/

void MainWindow::setStatusBarMessage(const QString &message) {
  Q_UNUSED(message);
  // TODO.
}

/*----------------------------------------------------------------------------*/

void MainWindow::startSaveTimer() {
  /* Automatically save the file at five minute intervals. */
  if (!m_saveTimer) {
    m_saveTimer = new QTimer(this);
    connect(m_saveTimer, SIGNAL(timeout()), this, SLOT(saveTempFile()));
    m_saveTimer->start(300000);
  } else {
    /* If the timer was stopped due to a DOM reset, start it again. */
    m_saveTimer->start(300000);
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::readSettings() {
  restoreGeometry(GlobalSettings::windowGeometry());
  restoreState(GlobalSettings::windowState());

  setShowHelpButtons(GlobalSettings::showHelpButtons());
  setShowTreeItemsVerbose(GlobalSettings::showTreeItemsVerbose());

  ui->actionRememberWindowGeometry->setChecked(
      GlobalSettings::useWindowSettings());
  ui->actionUseDarkTheme->setChecked(GlobalSettings::useDarkTheme());
  ui->actionShowHelpButtons->setChecked(GlobalSettings::showHelpButtons());
}

/*----------------------------------------------------------------------------*/

void MainWindow::saveSettings() {
  if (ui->actionRememberWindowGeometry->isChecked()) {
    GlobalSettings::setWindowGeometry(saveGeometry());
    GlobalSettings::setWindowState(saveState());
  } else {
    GlobalSettings::removeWindowInfo();
  }

  GlobalSettings::setUseDarkTheme(ui->actionUseDarkTheme->isChecked());
  GlobalSettings::setUseWindowSettings(
      ui->actionRememberWindowGeometry->isChecked());
}

/*----------------------------------------------------------------------------*/

void MainWindow::queryRestoreFiles() {
  QString dbName = GlobalSettings::DB_NAME;
  QStringList tempFiles = QDir::current().entryList(QDir::Files).filter(
      QString("%1_temp").arg(dbName.remove(".db")));

  if (!tempFiles.empty()) {
    QMessageBox::StandardButton accept = QMessageBox::information(
        this, "Found recovery files", "Found auto-recover files related to "
                                      "this profile. Would you like to view "
                                      "and save?",
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

    if (accept == QMessageBox::Ok) {
      /* "Delete on close" flag set. */
      RestoreFilesForm *restore = new RestoreFilesForm(tempFiles, this);
      restore->exec();
    }
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::saveTempFile() {
  QString dbName = GlobalSettings::DB_NAME;
  QFile file(QDir::currentPath() +
             QString("/%1_%2_temp")
                 .arg(m_currentXMLFileName.split("/").last())
                 .arg(dbName.remove(".db")));

  /* Since this is an attempt at auto-saving, we aim for a "best case" scenario
   * and don't display error messages if encountered. */
  if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream outStream(&file);
    // outStream << ui->treeWidget->toString();
    file.close();
    startSaveTimer();
  }
}

/*----------------------------------------------------------------------------*/

void MainWindow::deleteTempFile() {
  QString dbName = GlobalSettings::DB_NAME;
  QFile file(QDir::currentPath() +
             QString("/%1_%2_temp")
                 .arg(m_currentXMLFileName.split("/").last())
                 .arg(dbName.remove(".db")));

  file.remove();
}

/*----------------------------------------------------------------------------*/
