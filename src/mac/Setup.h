#ifndef SETUP_H
#define SETUP_H

// This has to be a separate header file as this is included in
// main.cpp, which is a C++ file. We can't use any objective-c
// imports here.

void macMain();

void checkForSparkleUpdate();

#endif
