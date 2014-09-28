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
#include "db/dbinterface.h"
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
#include "utils/QtWaitingSpinner.h"
#include "model/dommodel.h"
#include "delegate/domdelegate.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>
#include <QTimer>
#include <QUrl>
#include <QCloseEvent>
#include <QFont>
#include <QSettings>
#include <QLabel>

/*----------------------------------------------------------------------------*/

const QString EMPTY("---");
const QString LEFTRIGHTBRACKETS("\\[|\\]");

/*----------------------------------------------------------------------------*/

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_saveTimer(nullptr),
      m_currentXMLFileName(""), m_importedXmlFileName(""), m_dbThread(),
      m_fileContentsChanged(false) {
  ui->setupUi(this);

  /* XML File related. */
  connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
  connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(importXMLFromFile()));
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
  connect(ui->expandAllCheckBox, SIGNAL(toggled(bool)), this,
          SLOT(expandCollapse(bool)));

  /* Help related. */
  connect(ui->actionShowHelpButtons, SIGNAL(triggered(bool)), this,
          SLOT(setShowHelpButtons(bool)));

  m_spinner = new QtWaitingSpinner(Qt::ApplicationModal, this, true);

  ui->treeView->setModel(&m_model);
  ui->treeView->setItemDelegate(new DomDelegate(this));

  readSavedSettings();

  /* Wait for the event loop to be initialised before calling this function. */
  QTimer::singleShot(0, this, SLOT(queryRestoreFiles()));
}

/*----------------------------------------------------------------------------*/

MainWindow::~MainWindow() {
  delete ui;

  m_dbThread.quit();
  m_dbThread.wait();
}

/*----------------------------------------------------------------------------*/

void MainWindow::handleDBResult(DB::Result result, const QString &msg) {
  switch (result) {
  case DB::Result::ImportSuccess: {
    openFile(m_importedXmlFileName);
    break;
  }
  case DB::Result::Failed: {
    MessageSpace::showErrorMessageBox(this, msg);
    break;
  }
  case DB::Result::Critical: {
    MessageSpace::showErrorMessageBox(this, msg);
    this->close();
  }
  }
}

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

void MainWindow::startBatchProcessingThread() {
  /* Required for the cross-thread signal/slot communication. */
  qRegisterMetaType<DB::Result>("DB::Result");

  DB *db = new DB;
  db->moveToThread(&m_dbThread);
  connect(&m_dbThread, &QThread::finished, db, &QObject::deleteLater);

  /* Set up comms between DB and Main Window. */
  connect(this, &MainWindow::processDocumentXml, db, &DB::processDocumentXml);
  connect(db, &DB::result, this, &MainWindow::handleDBResult);

  /* Automatically start and stop the spinner. */
  connect(&m_dbThread, &QThread::started, m_spinner, &QtWaitingSpinner::start);
  connect(db, &DB::result, m_spinner, &QtWaitingSpinner::stop);

  m_dbThread.start();
}

/*----------------------------------------------------------------------------*/

QLabel *MainWindow::almostThere() {
  QString text(
      "Wow, this is a big file! Hang in there, we're crunching the numbers...");
  QLabel *label = new QLabel(text, this, Qt::Popup);
  label->show();
  label->move(window()->frameGeometry().topLeft() + window()->rect().center() -
              label->rect().center());
  return label;
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

void MainWindow::openFile(const QString &fileName) {
  if (!saveAndContinue("Save document before continuing?")) {
    return;
  }

  if (!fileName.isEmpty()) {
    m_domDoc.clear();
    m_domDoc = m_tmpDomDoc.cloneNode().toDocument();

    std::unique_ptr<QLabel> label{ almostThere() };
    m_model.setDomDocument(m_domDoc);

    expandCollapse(ui->expandAllCheckBox->isChecked());

    /* Enable file save options. */
    ui->actionCloseFile->setEnabled(true);
    ui->actionSave->setEnabled(true);
    ui->actionSaveAs->setEnabled(true);

    m_currentXMLFileName = fileName;
    m_fileContentsChanged = false; // at first load, nothing has changed
  }
}

/*----------------------------------------------------------------------------*/

QString MainWindow::readFile(const QString &fileName) {
  if (!fileName.isEmpty()) {
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream inStream(&file);
      QString fileContent(inStream.readAll());
      file.close();
      return fileContent;
    } else {
      QString errorMsg = QString("Failed to open file \"%1\": [%2]")
                             .arg(fileName)
                             .arg(file.errorString());
      MessageSpace::showErrorMessageBox(this, errorMsg);
    }
  }

  return QString();
}

/*----------------------------------------------------------------------------*/

void MainWindow::importXMLFromFile() {
  QString fileName = getOpenFileName();
  QString xml = readFile(fileName);
  m_importedXmlFileName = "";

  if (!xml.isEmpty()) {
    /* Reading file with Input Source and simple reader results in a lot of
     * empty rows in the model, for now, revert to "standard" DOM string content
     * set method. */
    // QXmlInputSource source();
    // source.setData(xml);
    // QXmlSimpleReader reader;

    QString xmlErr("");
    int line(-1);
    int col(-1);

    // if (m_tmpDomDoc.setContent(&source, &reader, &xmlErr, &line, &col)) {
    if (m_tmpDomDoc.setContent(xml, &xmlErr, &line, &col)) {
      m_importedXmlFileName = fileName;

      /* Fire up the DB thread for the batch processing. */
      startBatchProcessingThread();

      /* Tell the DB thread what XML needs to be processed. If the document was
       * processed successfully, the user will be prompted to open the file as
       * well (see "handleDBResult") */
      emit processDocumentXml(xml);
    } else {
      QString errorMsg = QString("XML is broken: %1: line [%2], column")
                             .arg(xmlErr)
                             .arg(line)
                             .arg(col);
      MessageSpace::showErrorMessageBox(this, errorMsg);
    }
  }
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

void MainWindow::searchDocument() {
  /* Delete on close flag set (no clean-up needed). */
  //  SearchForm *form = new
  // SearchForm(ui->treeWidget->allTreeWidgetItems(),
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

void MainWindow::resetDOM() {
  m_domDoc.clear();
  m_fileContentsChanged = false;

  /* The timer will be reactivated as soon as work starts again on a
   * legitimate
   * document and the user saves it for the first time. */
  if (m_saveTimer) {
    m_saveTimer->stop();
  }
}

/*----------------------------------------------------------------------------*/

bool MainWindow::saveAndContinue(const QString &resetReason) {
  /* There are a number of places and opportunities for "resetDOM" to be
   * called,
   * if there is an active document, check if it's content has changed since
   * the
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

void MainWindow::readSavedSettings() {
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

void MainWindow::expandCollapse(bool expand) {
  expand ? ui->treeView->expandAll() : ui->treeView->collapseAll();
}

/*----------------------------------------------------------------------------*/

void MainWindow::saveTempFile() {
  QString dbName = GlobalSettings::DB_NAME;
  QFile file(QDir::currentPath() +
             QString("/%1_%2_temp")
                 .arg(m_currentXMLFileName.split("/").last())
                 .arg(dbName.remove(".db")));

  /* Since this is an attempt at auto-saving, we aim for a "best case"
   * scenario
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
