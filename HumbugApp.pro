QT       += core gui webkit

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webkitwidgets

TARGET = HumbugApp
TEMPLATE = app


SOURCES += src/main.cpp\
           src/HumbugWindow.cpp \
           src/HumbugTrayIcon.cpp

HEADERS  += src/HumbugWindow.h \
            src/HumbugTrayIcon.h

FORMS    += src/HumbugWindow.ui

RESOURCES += \
    src/resources.qrc
