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

#ifndef GCRESTOREFILESFORM_H
#define GCRESTOREFILESFORM_H

#include <QDialog>

namespace Ui
{
  class GCRestoreFilesForm;
}

/// Displays recovered files so that the user may decide whether or not he/she wants to save them.
class GCRestoreFilesForm : public QDialog
{
Q_OBJECT

public:
  /*! Constructor. */
  explicit GCRestoreFilesForm( const QStringList& tempFiles, QWidget* parent = 0 );

  /*! Destructor. */
  ~GCRestoreFilesForm();

  private slots:
  /*! Connected to the "Save" button's "clicked()" signal.  Opens a file dialog to "Save As". */
  void saveFile();

  /*! Connected to the "Discard" button's "clicked()" signal.  Deletes the current temp file at the
      user's discretion. */
  void deleteTempFile() const;

  /*! Connected to the "Next" button's "clicked()" signal.  Loads the next temp file (if any). */
  void next();

private:
  /*! Loads and displays the current temp file in the text edit. */
  void loadFile( const QString& fileName );

  Ui::GCRestoreFilesForm* ui;
  QStringList m_tempFiles;
  QString m_fileName;
};

#endif // GCRESTOREFILESFORM_H