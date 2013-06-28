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

#include "appcast.h"
#include "common.h"
#include "compareversions.h"
#include "followredirects.h"
#include "updatechecker.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QtDebug>

namespace qtsparkle {

struct UpdateChecker::Private {
  Private()
    : network_(NULL),
      busy_(false),
      override_user_skip_(false)
  {
  }

  QNetworkRequest MakeRequest(const QUrl& url);

  QNetworkAccessManager* network_;
  QString version_;

  bool busy_;
  bool override_user_skip_;
};

UpdateChecker::UpdateChecker(QObject* parent)
  : QObject(parent),
    d(new Private)
{
}

UpdateChecker::~UpdateChecker() {
}

void UpdateChecker::SetNetworkAccessManager(QNetworkAccessManager* network) {
  d->network_ = network;
}

void UpdateChecker::SetVersion(const QString& version) {
  d->version_ = version;
}

QNetworkRequest UpdateChecker::Private::MakeRequest(const QUrl& url) {
  QNetworkRequest req(url);
  req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                   QNetworkRequest::AlwaysNetwork);

  return req;
}

void UpdateChecker::Check(const QUrl& appcast_url, bool override_user_skip) {
  if (d->busy_)
    return;

  emit CheckStarted();

  if (!d->network_)
    d->network_ = new QNetworkAccessManager(this);

  d->busy_ = true;
  d->override_user_skip_ = override_user_skip;

  FollowRedirects* reply = new FollowRedirects(
        d->network_->get(d->MakeRequest(appcast_url)));
  connect(reply, SIGNAL(Finished()), SLOT(Finished()));
  connect(reply, SIGNAL(RedirectLimitReached()), SLOT(RedirectLimitReached()));
}

void UpdateChecker::Finished() {
  FollowRedirects* reply = qobject_cast<FollowRedirects*>(sender());
  if (!reply)
    return;
  reply->deleteLater();

  d->busy_ = false;

  AppCastPtr appcast(new AppCast);
  bool success = appcast->Load(reply->reply());
  if (!success) {
    emit CheckFailed(appcast->error_reason());
    return;
  }

  // Is the application version greater than or equal to the latest version?
  if (!CompareVersions(d->version_, appcast->version())) {
    emit UpToDate();
    return;
  }

  // Has the user asked to skip this version?
  if (!d->override_user_skip_) {
    QSettings s;
    s.beginGroup(kSettingsGroup);
    if (s.value("skipped_version").toString() == appcast->version()) {
      emit UpToDate();
      return;
    }
  }

  emit UpdateAvailable(appcast);
}

void UpdateChecker::RedirectLimitReached() {
  emit CheckFailed("Redirect limit reached");
}

} // namespace qtsparkle
