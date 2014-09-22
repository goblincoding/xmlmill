# Copyright (c) 2012 - 2015 by William Hallatt.
#
# This file forms part of "XML Mill".
#
# The official website for this project is <http://www.goblincoding.com> and,
# although not compulsory, it would be appreciated if all works of whatever
# nature using this source code (in whole or in part) include a reference to
# this site.
#
# Should you wish to contact me for whatever reason, please do so via:
#
#                 <http://www.goblincoding.com/contact>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program (GNUGPL.txt).  If not, see
#
#                    <http://www.gnu.org/licenses/>


#-------------------------------------------------
#
# Project created by QtCreator 2012-10-24T17:22:18
#
#-------------------------------------------------

QT       += core xml sql widgets

TARGET = XMLMill
TEMPLATE = app

# This is supposed to be sufficient for Qt5, but does not
# seem to work (still getting compiler warnings), hence
# the QMAKE_CXXFLAGS addition
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
    db/dbinterface.cpp \
    mainwindow.cpp \
    db/batchprocesshelper.cpp \
    xml/xmlsyntaxhighlighter.cpp \
    utils/combobox.cpp \
    utils/messagespace.cpp \
    forms/helpdialog.cpp \
    forms/searchform.cpp \
    forms/additemsform.cpp \
    forms/removeitemsform.cpp \
    forms/restorefilesform.cpp \
    utils/domtreewidget.cpp \
    utils/treewidgetitem.cpp \
    forms/addsnippetsform.cpp \
    utils/plaintextedit.cpp \
    utils/globalsettings.cpp \
    model/domitem.cpp \
    delegate/domdelegate.cpp \
    model/dommodel.cpp

HEADERS  += \
    db/dbinterface.h \
    mainwindow.h \
    db/batchprocesshelper.h \
    xml/xmlsyntaxhighlighter.h \
    utils/combobox.h \
    utils/messagespace.h \
    forms/helpdialog.h \
    forms/searchform.h \
    forms/additemsform.h \
    forms/removeitemsform.h \
    forms/restorefilesform.h \
    utils/domtreewidget.h \
    utils/treewidgetitem.h \
    forms/addsnippetsform.h \
    utils/plaintextedit.h \
    utils/globalsettings.h \
    delegate/domdelegate.h \
    model/domitem.h \
    model/dommodel.h

FORMS    += \
    mainwindow.ui \
    forms/messagedialog.ui \
    forms/helpdialog.ui \
    forms/searchform.ui \
    forms/removeitemsform.ui \
    forms/additemsform.ui \
    forms/restorefilesform.ui \
    forms/addsnippetsform.ui

RESOURCES += \
    resources/resources.qrc

win32:RC_FILE = resources/appicon/goblinappico.rc
