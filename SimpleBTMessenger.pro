#-------------------------------------------------
#
# Project created by QtCreator 2014-11-08T12:19:02
#
#-------------------------------------------------

QT       += core gui widgets bluetooth core

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimpleBTMessenger
TEMPLATE = app


SOURCES += src/main.cpp\
        src/mainwindow.cpp

HEADERS  += src/mainwindow.h

FORMS    += src/mainwindow.ui

CONFIG += mobility
MOBILITY = 

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

OTHER_FILES += \
    android/AndroidManifest.xml

