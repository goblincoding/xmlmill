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

#ifndef GCSEARCHFORM_H
#define GCSEARCHFORM_H

#include <QDialog>

namespace Ui
{
  class GCSearchForm;
}

class QDomElement;

/*------------------------------------------------------------------------------------------

  Search through the current document for a specific element/attribute/value.

------------------------------------------------------------------------------------------*/

class GCSearchForm : public QDialog
{
  Q_OBJECT
  
public:
  explicit GCSearchForm( const QList< QDomElement > &elements, QWidget *parent = 0 );
  ~GCSearchForm();

signals:
  void foundElement( const QDomElement &element );

private slots:
  void search();
  
private:
  Ui::GCSearchForm *ui;
  int m_lastIndex;

  QList< QDomElement > m_elements;
};

#endif // GCSEARCHFORM_H
