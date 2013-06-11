QT       += core gui webkit phonon network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webkitwidgets

TARGET = HumbugApp
TEMPLATE = app


SOURCES += src/main.cpp\
           src/HumbugWindow.cpp \
           src/HumbugTrayIcon.cpp \
    src/HumbugWebBridge.cpp \
    src/cookiejar.cpp \
    src/HumbugAboutDialog.cpp

HEADERS  += src/HumbugWindow.h \
            src/HumbugTrayIcon.h \
    src/HumbugWebBridge.h \
    src/cookiejar.h \
    src/HumbugAboutDialog.h

FORMS    += src/HumbugWindow.ui \
    src/HumbugAboutDialog.ui

RESOURCES += \
    src/resources.qrc


CONFIG += debug
