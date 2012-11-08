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
    utils/gcquerydialog.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h \
    db/gcbatchprocessorhelper.h \
    utils/gcglobals.h \
    db/gcknowndbform.h \
    utils/gcquerydialog.h

FORMS    += \
    gcmainwindow.ui \
    db/gcsessiondbform.ui \
    utils/gcquerydialog.ui

RESOURCES += \
    resources/gcresources.qrc
