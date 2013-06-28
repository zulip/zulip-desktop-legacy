/* This file is part of qtsparkle.
   Copyright (c) 2010 David Sansome <me@davidsansome.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#ifndef QTSPARKLE_UPDATER_H
#define QTSPARKLE_UPDATER_H

#include <QObject>
#include <QScopedPointer>

class QIcon;
class QNetworkAccessManager;
class QUrl;

namespace qtsparkle {

// Loads qtsparkle's translations from the .ts files compiled into the library,
// and installs them using QCoreApplication::installTranslator.  Use this
// function if you want to use a non-default language for qtsparkle.  If you
// do not call this function, it will be called with the default language
// (QLocale::system().name()) the first time qtsparkle::Updater is created.
void LoadTranslations(const QString& language);


// The Updater is the main class in qtsparkle that you should use in your
// application.  Updater loads its settings from QSettings in its constructor,
// so it's important you set an organizationName, organizationDomain and
// applicationName in QCoreApplication before calling it.
// On the second run of this application it will create a dialog asking the
// user for permission to check for updates.  After that, if the user gave
// permission, it will check for updates automatically on startup.
// Checking for updates and displaying dialogs is done after the application
// returns to the event loop, not in the constructor.
class Updater : public QObject {
  Q_OBJECT

public:
  // appcast_url is the URL that this class should use when checking for
  // updates.  If parent is not NULL then any dialogs created by this class
  // are parented to that widget.
  Updater(const QUrl& appcast_url, QWidget* parent);
  ~Updater();

  // Sets a network access manager to use when making network requests.  If
  // network is NULL, or if this function is not called, this class will create
  // a new QNetworkManager.
  // Must be called before the application returns to the event loop after this
  // object is created.
  // The Updater will NOT take ownership of the network manager, and you must
  // ensure it is not deleted while the Updater is still in scope.
  void SetNetworkAccessManager(QNetworkAccessManager* network);

  // Sets an icon to use in any dialogs that are created.  If no icon is set,
  // the windowIcon() of the parent widget passed to the constructor is used
  // instead.  The icon should be 64x64 pixels or greater.
  void SetIcon(const QIcon& icon);

  // Sets the current version.  If this is not called then the default is to
  // use QCoreApplication::applicationVersion()
  void SetVersion(const QString& version);

  // Sets the update check interval in msec. Default value is one day (86400000).
  // Minimum value is one hour (3600000)
  void SetUpdateInterval(int msec);

public slots:
  // Checks for updates now.  You probably want to call this from a menu item
  // in your application's main window.
  void CheckNow();

private slots:
  void AutoCheck();

protected:
  bool event(QEvent* e);

private:
  struct Private;
  QScopedPointer<Private> d;
};

} // namespace qtsparkle


#endif // QTSPARKLE_UPDATER_H
