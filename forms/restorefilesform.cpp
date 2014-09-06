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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "restorefilesform.h"
#include "ui_restorefilesform.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "db/dbinterface.h"
#include "utils/globalsettings.h"
#include "utils/messagespace.h"

#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QFileDialog>

/*----------------------------------------------------------------------------*/

RestoreFilesForm::RestoreFilesForm(const QStringList &tempFiles,
                                   QWidget *parent)
    : QDialog(parent), ui(new Ui::RestoreFilesForm), m_tempFiles(tempFiles),
      m_fileName("") {
  ui->setupUi(this);
  ui->plainTextEdit->setFont(QFont(GlobalSettings::FONT, GlobalSettings::FONTSIZE));
  setAttribute(Qt::WA_DeleteOnClose);

  connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveFile()));
  connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(next()));
  connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(ui->discardButton, SIGNAL(clicked()), this, SLOT(deleteTempFile()));

  /* Everything happens automagically and the text edit takes ownership. */
  XmlSyntaxHighlighter *highLighter =
      new XmlSyntaxHighlighter(ui->plainTextEdit->document());
  Q_UNUSED(highLighter);

  next();
}

/*----------------------------------------------------------------------------*/

RestoreFilesForm::~RestoreFilesForm() { delete ui; }

/*----------------------------------------------------------------------------*/

void RestoreFilesForm::saveFile() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Save As", GlobalSettings::lastUserSelectedDirectory(),
      "XML Files (*.*)");

  /* If the user clicked "OK". */
  if (!fileName.isEmpty()) {
    QFile file(fileName);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate |
                   QIODevice::Text)) {
      QString errMsg = QString("Failed to save file \"%1\": [%2].")
                           .arg(fileName)
                           .arg(file.errorString());
      MessageSpace::showErrorMessageBox(this, errMsg);
    } else {
      QTextStream outStream(&file);
      outStream << ui->plainTextEdit->toPlainText();
      file.close();

      deleteTempFile();

      /* Save the last visited directory. */
      QFileInfo fileInfo(fileName);
      QString finalDirectory = fileInfo.dir().path();
      GlobalSettings::setLastUserSelectedDirectory(finalDirectory);
    }
  }
}

/*----------------------------------------------------------------------------*/

void RestoreFilesForm::deleteTempFile() const {
  QDir dir;
  dir.remove(m_fileName);
  ui->plainTextEdit->clear();
  ui->lineEdit->clear();
}

/*----------------------------------------------------------------------------*/

void RestoreFilesForm::next() {
  if (!m_tempFiles.isEmpty()) {
    m_fileName = m_tempFiles.takeFirst();
    ui->nextButton->setEnabled(true);
    loadFile(m_fileName);
  } else {
    ui->plainTextEdit->setPlainText("No documents left to recover.");
    ui->saveButton->setVisible(false);
    ui->discardButton->setVisible(false);
    ui->nextButton->setVisible(false);
    ui->cancelButton->setText("Close");
    ui->lineEdit->clear();
  }
}

/*----------------------------------------------------------------------------*/

void RestoreFilesForm::loadFile(const QString &fileName) {
  QFile file(fileName);
  QString dbName = GlobalSettings::DB_NAME;

  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString displayName = fileName;
    displayName = displayName.remove(
        QString("_%1_temp")
            .arg(dbName.remove(".db")));
    ui->lineEdit->setText(displayName);

    QString xmlErr("");
    int line(-1);
    int col(-1);

    QTextStream inStream(&file);
    QString fileContent = inStream.readAll();
    ui->plainTextEdit->setPlainText(fileContent);

    QDomDocument doc;

    if (!doc.setContent(fileContent, &xmlErr, &line, &col)) {
      QString errorMsg =
          QString("XML is broken - Error [%1], line [%2], column [%3].")
              .arg(xmlErr)
              .arg(line)
              .arg(col);

      MessageSpace::showErrorMessageBox(this, errorMsg);

      /* Unfortunately the line number returned by the DOM doc doesn't match up
       * with what's visible in the QTextEdit.  It seems as if it's
       * mostly off by two lines. For now it's a fix, but will have to figure
       * out how to make sure that we highlight the correct lines. Ultimately
       * this finds the broken XML and highlights it in red...what a mission...
       */
      QTextBlock textBlock =
          ui->plainTextEdit->document()->findBlockByLineNumber(line - 2);
      QTextCursor cursor(textBlock);
      cursor.movePosition(QTextCursor::NextWord);
      cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

      QTextEdit::ExtraSelection highlight;
      highlight.cursor = cursor;
      highlight.format.setBackground(QColor(220, 150, 220));
      highlight.format.setProperty(QTextFormat::FullWidthSelection, true);

      QList<QTextEdit::ExtraSelection> extras;
      extras << highlight;
      ui->plainTextEdit->setExtraSelections(extras);
      ui->plainTextEdit->ensureCursorVisible();
    }
  } else {
    MessageSpace::showErrorMessageBox(this,
                                      "Failed to open file. Cannot recover.");
  }
}

/*----------------------------------------------------------------------------*/
