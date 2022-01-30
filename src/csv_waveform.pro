#-------------------------------------------------
#
# Project created by QtCreator 2012-03-04T23:24:55
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

TARGET = csv_waveform
TEMPLATE = app

INCLUDEPATH += $${_PRO_FILE_PWD_}/lua\
               $${_PRO_FILE_PWD_}/qcustomplot

LIBS += $${_PRO_FILE_PWD_}/lua/lua/lua53.dll

SOURCES += main.cpp\
        mainwindow.cpp \
        qcustomplot/qcustomplot.cpp

HEADERS  += mainwindow.h \
            qcustomplot/qcustomplot.h

FORMS    += mainwindow.ui


DISTFILES += \
    lua/lua/lua53.dll 

