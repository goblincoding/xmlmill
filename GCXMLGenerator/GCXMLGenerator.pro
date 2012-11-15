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
    xml/xmlsyntaxhighlighter.cpp \
    forms/gcnewelementform.cpp \
    forms/gcknowndbform.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h \
    db/gcbatchprocessorhelper.h \
    utils/gcglobals.h \
    xml/xmlsyntaxhighlighter.h \
    forms/gcnewelementform.h \
    forms/gcknowndbform.h

FORMS    += \
    gcmainwindow.ui \
    forms/gcnewelementform.ui \
    forms/gcknowndbform.ui

RESOURCES += \
    resources/gcresources.qrc
