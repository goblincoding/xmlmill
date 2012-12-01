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

#ifndef GCNEWDBWIZARD_H
#define GCNEWDBWIZARD_H

#include <QWizard>

namespace Ui
{
  class GCNewDBWizard;
}

/*------------------------------------------------------------------------------------------

  A wizard to guide users through the process of creating and populating new databases.

------------------------------------------------------------------------------------------*/

class GCNewDBWizard : public QWizard
{
  Q_OBJECT
  
public:
  explicit GCNewDBWizard( QWidget *parent = 0 );
  ~GCNewDBWizard();
  QString xmlFileName();
  QString dbFileName();

public slots:
  void accept();
  
private slots:
  void openDBFileDialog();
  void openXMLFileDialog();

private:
  Ui::GCNewDBWizard *ui;
  QString m_dbFileName;
  QString m_xmlFileName;
};

#endif // GCNEWDBWIZARD_H
