#-------------------------------------------------
#
# Project created by QtCreator 2014-08-02T13:09:59
#
#-------------------------------------------------

QT       += core

TARGET = encoding
CONFIG   += console
CONFIG   -= app_bundle
#CONFIG   += c++11

QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS

TEMPLATE = app

INCLUDEPATH += $$PWD/libav-10.1/include

win32: LIBS += -L$$PWD/libav-10.1/lib -lavcodec -lavformat -lavutil -lswscale


SOURCES += main.cpp \
    encoder.cpp

HEADERS += \
    encoder.h
