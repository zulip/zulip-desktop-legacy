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
#include "followredirects.h"
#include "updatedialog.h"
#include "ui_updatedialog.h"

#include <QDesktopServices>
#include <QSettings>
#include <QtDebug>

namespace qtsparkle {

struct UpdateDialog::Private {
  Private()
    : network_(NULL)
  {
  }

  QScopedPointer<Ui_UpdateDialog> ui_;

  QNetworkAccessManager* network_;
  QString version_;
  AppCastPtr appcast_;

  static const int kIconSize;
};

const int UpdateDialog::Private::kIconSize = 64;


UpdateDialog::UpdateDialog(QWidget *parent)
  : QDialog(parent),
    d(new Private)
{
  d->ui_.reset(new Ui_UpdateDialog);
  d->ui_->setupUi(this);

  d->ui_->release_notes_label->setText(
      "<b>" + d->ui_->release_notes_label->text() + "</b>");
  d->ui_->icon->hide();

  connect(d->ui_->install, SIGNAL(clicked()), SLOT(Install()));
  connect(d->ui_->skip, SIGNAL(clicked()), SLOT(Skip()));
  connect(d->ui_->later, SIGNAL(clicked()), SLOT(close()));
}

UpdateDialog::~UpdateDialog() {
}

void UpdateDialog::SetNetworkAccessManager(QNetworkAccessManager* network) {
  d->network_ = network;
}

void UpdateDialog::SetIcon(const QIcon& icon) {
  if (icon.isNull()) {
    d->ui_->icon->hide();
  } else {
    d->ui_->icon->setPixmap(icon.pixmap(Private::kIconSize));
    d->ui_->icon->show();
  }
}

void UpdateDialog::SetVersion(const QString& version) {
  d->version_ = version;
}

void UpdateDialog::ShowUpdate(AppCastPtr appcast) {
  d->appcast_ = appcast;

  d->ui_->title->setText("<h3>" +
      tr("A new version of %1 is available").arg(qApp->applicationName()) + "</h3>");
  d->ui_->summary->setText(
      tr("%1 %2 is now available - you have %3.  Would you like to download it now?")
      .arg(qApp->applicationName(), appcast->version(), d->version_));

  show();

  if (!appcast->description().isEmpty()) {
    d->ui_->release_notes->setHtml(appcast->description());
  } else {
    if (!d->network_)
      d->network_ = new QNetworkAccessManager(this);

    FollowRedirects* reply = new FollowRedirects(
        d->network_->get(QNetworkRequest(appcast->release_notes_url())));
    connect(reply, SIGNAL(Finished()), SLOT(ReleaseNotesReady()));
  }
}

void UpdateDialog::ReleaseNotesReady() {
  FollowRedirects* reply = qobject_cast<FollowRedirects*>(sender());
  if (!reply)
    return;
  reply->deleteLater();

  if (reply->reply()->header(QNetworkRequest::ContentTypeHeader).toString()
      .contains("text/html")) {
    d->ui_->release_notes->setHtml(reply->reply()->readAll());
  } else {
    d->ui_->release_notes->setPlainText(reply->reply()->readAll());
  }
}

void UpdateDialog::Install() {
  if (!d->appcast_)
    return;

  QDesktopServices::openUrl(d->appcast_->download_url());
  close();
}

void UpdateDialog::Skip() {
  if (!d->appcast_)
    return;

  QSettings s;
  s.beginGroup(kSettingsGroup);
  s.setValue("skipped_version", d->appcast_->version());
  close();
}

} // namespace qtsparkle
