# Zulip legacy desktop app

The legacy Zulip desktop app is a C++ application written with the Qt toolkit. It is a lightweight wrapper around a Webkit web view: it loads the zulip webapp as a single page full-screen webpage. The desktop app provides some native integrations: tray icon and Dock support, notifications, and more.

It is deprecated in favor of the [new electron implementation](https://github.com/zulip/zulip-electron).

# Dependencies

* cmake >= 2.10
* Qt4 OR Qt5
* qjson

## Linux

* Qt4 with Phonon

## OS X

* Sparkle.framework
* Growl.framework (both should be installed to `/Library/Frameworks`)

## Windows

* NSIS

# Installation

```
mkdir build
cd build
cmake ..
make
```

To build with Qt5, pass `-DBUILD_WITH_QT5=On` to `cmake`.

# Use With Custom Servers

The desktop client accepts the following switches:

```
--debug                          | Log debug info
--site https://yourdomain.com    | Connect to a custom domain
--allow-insecure                 | Don't verify SSL certificates
```

To use with your own domain, start the desktop client using the `--site` option.


## Architecture

Though the desktop app is written in C++ using the Qt toolkit, the Mac version of the desktop app does not use the QtWebkit included with Qt: it instead uses a native Cocoa WebView that is embedded inside the main Qt app with a QMacCocoaViewContainer bridge.

The `HWebView` class provides an abstraction around the platforms-specific pieces that comprise this web view: on Mac, `HWebView_mac.mm` is the Objective-C++ file that integrate with the native Cocoa webview. On Windows and Linux, `HWebView.cpp` uses QtWebkit to render the display.

# Building releases

## OS X

On OS X we can generate a packaged .dmg with a bundled `Zulip Desktop.app`.

Run `./admin/mac/make_package.sh CODESIGN_NAME ZULIP_VERSION` where:

* CODESIGN_NAME is the name of the developer ID that should be used to codesign the resulting .app
* ZULIP_VERSION is the version
string you want for this build of Zulip.

## Sparkle

Our OS X app uses Sparkle as an auto-updating mechanism. Sparkle is configured as an RSS feed on our server that the desktop app checks daily---if there is a new release in the RSS feed, Sparkle will prompt the user to upgrade, and if the user confirms it will automatically download, install, and restart the Zulip app.

The sparkle RSS feeds and Changelogs are configured in the `sparkle` puppet files in the main `zulip` server repository. The `make-package.sh` build script above will also output the Sparkle codesign signature that should be included in the Sparkle RSS file.

## Windows

This requires a MSVC build toolchain and the Zulip dependencies built with the **same** version of MSVC. The windows build requires Qt5.

## Linux

See your favorite distribution package management system for how to distribute a binary Zulip package on linux.

# Debugging

## Linux / Windows

It is possible to attach a web inspector to the QtWebkit page that is used in the Linux/Windows desktop app. Here are the steps:

1. Run zulip with zulip --debug
2. Download and extract http://files.lfranchi.com/QtTestBrowser.tar.bz2 (the version of webkit used to connect to the remote developer tools must be the same as the version of webkit in zulip)
3. In the extracted folder, run `qmake && make`
4. Run the webkit browser with bin/QtTestBrowser
5. Navigate to `localhost:8888` in your test browser

## Mac

The Cocoa WebView in the Mac app natively supports opening the Safari web inspector.

1. Run `Zulip.app/Contents/MacOS/Zulip --debug`
2. Right-click on an element on the page and select "Inspect Element"
