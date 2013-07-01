#ifndef SETUP_H
#define SETUP_H

#include <QString>

// This has to be a separate header file as this is included in
// main.cpp, which is a C++ file. We can't use any objective-c
// imports here.

void macMain();

void macNotify(const QString& title, const QString& content);

void checkForSparkleUpdate();

#endif
