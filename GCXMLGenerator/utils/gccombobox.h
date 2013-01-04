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

#ifndef GCCOMBOBOX_H
#define GCCOMBOBOX_H

#include <QComboBox>

/// A custom combo box providing additional user selection information.

/** The only reason this class exists is so that we may know when a combo box is activated.
    Initially I understood that the "activated" signal is emitted when a user clicks on
    a QComboBox (e.g. when the dropdown is expanded), but it turns out that this is not the
    case.
*/
class GCComboBox : public QComboBox
{
  Q_OBJECT

public:
  /*! Constructor. */
  explicit GCComboBox( QWidget *parent = 0 );
  
protected:
  /*! Re-eimplemented from QComboBox to emit the activated(int) signal. */
  void mousePressEvent( QMouseEvent *e );

  /*! Re-eimplemented from QComboBox to emit the activated(int) signal. */
  void focusInEvent ( QFocusEvent *e );

  /*! Re-eimplemented from QComboBox to emit the currentIndexChanged(QString) signal. */
  void focusOutEvent( QFocusEvent *e );
};

#endif // GCCOMBOBOX_H
