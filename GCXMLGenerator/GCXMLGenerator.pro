#-------------------------------------------------
#
# Project created by QtCreator 2012-10-24T17:22:18
#
#-------------------------------------------------

QT       += core gui xml sql

TARGET = XMLStudio
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
    utils/gchelp.cpp

HEADERS  += \
    db/gcdatabaseinterface.h \
    gcmainwindow.h \
    db/gcbatchprocessorhelper.h \
    xml/xmlsyntaxhighlighter.h \
    forms/gcnewelementform.h \
    forms/gcknowndbform.h \
    utils/gccombobox.h \
    forms/gcmessagedialog.h \
    utils/gchelp.h

FORMS    += \
    gcmainwindow.ui \
    forms/gcnewelementform.ui \
    forms/gcknowndbform.ui \
    forms/gcmessagedialog.ui

RESOURCES += \
    resources/gcresources.qrc
