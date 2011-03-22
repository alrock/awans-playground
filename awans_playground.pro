#-------------------------------------------------
#
# Project created by QtCreator 2011-03-19T17:57:45
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = awans_playground
CONFIG   += console
CONFIG   -= app_bundle
QMAKE_CXXFLAGS += -std=c++0x

TEMPLATE = app


SOURCES += main.cpp

HEADERS += \
    mpmc_bounded_queue.h \
    thread.h \
    spmc_bounded_queue.h \
    supervisor.h
