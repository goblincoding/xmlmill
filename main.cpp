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

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QSettings>

#include "mainwindow.h"
#include "utils/globalsettings.h"

/*----------------------------------------------------------------------------*/

QString styleSheet() {
  QFile file(":resources/dark.txt");
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream stream(&file);
  QString style = stream.readAll();
  file.close();

  return style;
}

/*----------------------------------------------------------------------------*/

int main(int ar, char *argv[]) {
  QApplication a(ar, argv);

  if (GlobalSettings::useDarkTheme()) {
    a.setStyleSheet(styleSheet());
  }

  MainWindow w;
  w.show();

  return a.exec();
}

/*----------------------------------------------------------------------------*/
