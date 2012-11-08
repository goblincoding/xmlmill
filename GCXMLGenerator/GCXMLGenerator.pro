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
    gcmainwindow.cpp \
    db/gcbatchprocessorhelper.cpp \
    db/gcknowndbform.cpp \
    utils/gcmessagebox.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h \
    db/gcbatchprocessorhelper.h \
    utils/gcglobals.h \
    db/gcknowndbform.h \
    utils/gcmessagebox.h

FORMS    += \
    gcmainwindow.ui \
    db/gcsessiondbform.ui

RESOURCES += \
    resources/gcresources.qrc
