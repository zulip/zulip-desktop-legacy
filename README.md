# Zulip Desktop

Zulip Desktop is a Qt/C++ application that serves as a shell around a web view.

# Dependencies

cmake
Qt4 OR Qt5
qjson

Linux Qt4: Phonon

OS X: Sparkle.framework, Growl.framework (both should be installed to `/Library/Frameworks`)

Windows: NSIS

# Installation

`mkdir build`
`cd build`
`cmake ..`
`make`

To build with Qt5, pass "-DBUILD_WITH_QT5=On" to cmake.

# Building releases

## OS X

On OS X we can generate a packaged .dmg with a bundled Zulip Desktop.app.

Then, run `./admin/mac/make_package ZULIP_VERSION` where ZULIP_VERSION is the version
string you want for this build of Zulip.

## Windows

This requires a mingw32 cross-compilation toolchain on OpenSUSE.

If you have that, you can then run `./admin/win/make_package.sh ZULIP_VERSION`

## Linux

TODO CPack