#-------------------------------------------------
#
# Project created by QtCreator 2012-10-24T17:22:18
#
#-------------------------------------------------

QT       += core gui xml sql

TARGET = GCXMLGenerator
TEMPLATE = app


SOURCES += main.cpp\
    db/gcdatabaseinterface.cpp \
    gcmainwindow.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h

FORMS    += \
    gcmainwindow.ui

RESOURCES += \
    resources/gcresources.qrc
