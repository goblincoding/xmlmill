/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "GoblinCoding's XML Studio".
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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#ifndef GCNEWELEMENTFORM_H
#define GCNEWELEMENTFORM_H

#include <QWidget>

/*------------------------------------------------------------------------------------------

  This form allows the user to add new elements and their associated attributes to the 
  database.  Although only one element can be added at a time (with or without attributes), 
  all an element's attributes can be provided in one go through simply ensuring that each 
  attribute appears on its own line in the input text edit.  
  
  The user will also be allowed to continue adding elements until "Done" is selected.

------------------------------------------------------------------------------------------*/

namespace Ui
{
  class GCNewElementForm;
}

class GCNewElementForm : public QWidget
{
  Q_OBJECT
  
public:
  explicit GCNewElementForm( QWidget *parent = 0 );
  ~GCNewElementForm();

signals:
  /* Emits the name of the new element and the list of associated attributes
    (or an empty list if the element doesn't have any attributes). */
  void newElementDetails( const QString&, const QStringList& );
  
private slots:
  void addElementAndAttributes();
  void showHelp();

private:
  Ui::GCNewElementForm *ui;
};

#endif // GCNEWELEMENTFORM_H
