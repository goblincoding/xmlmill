# Copyright (c) 2012 by William Hallatt.
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

QT       += core gui xml sql

TARGET = XMLMill
TEMPLATE = app


SOURCES += main.cpp\
    db/gcdatabaseinterface.cpp \
    gcmainwindow.cpp \
    db/gcbatchprocessorhelper.cpp \    
    xml/xmlsyntaxhighlighter.cpp \
    forms/gcnewelementform.cpp \
    forms/gcknowndbform.cpp \
    utils/gccombobox.cpp \
    forms/gcmessagedialog.cpp \
    utils/gcdbsessionmanager.cpp \
    utils/gcmessagespace.cpp \
    forms/gchelpdialog.cpp \
    forms/gcdestructiveeditdialog.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h \
    db/gcbatchprocessorhelper.h \
    xml/xmlsyntaxhighlighter.h \
    forms/gcnewelementform.h \
    forms/gcknowndbform.h \
    utils/gccombobox.h \
    forms/gcmessagedialog.h \
    utils/gcdbsessionmanager.h \
    utils/gcmessagespace.h \
    forms/gchelpdialog.h \
    forms/gcdestructiveeditdialog.h

FORMS    += \
    gcmainwindow.ui \
    forms/gcnewelementform.ui \
    forms/gcknowndbform.ui \
    forms/gcmessagedialog.ui \
    forms/gchelpdialog.ui \
    forms/gcdestructiveeditdialog.ui

RESOURCES += \
    resources/gcresources.qrc
