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
#include "compareversions.h"

#include <QXmlStreamReader>
#include <QtDebug>

namespace qtsparkle {

struct AppCast::Private {
  struct Item {
    QString version_;
    QString download_url_;
    QString release_notes_url_;
    QString description_;

    bool operator <(const Item& other) const {
      return CompareVersions(version_, other.version_);
    }
  };
  typedef QList<Item> ItemList;

  static Item LoadItem(QXmlStreamReader* reader) {
    Item ret;

    while (!reader->atEnd()) {
      reader->readNext();

      if (reader->tokenType() == QXmlStreamReader::EndElement
          && reader->name() == "item")
        break;

      if (reader->tokenType() == QXmlStreamReader::StartElement) {
        if (reader->name() == "releaseNotesLink" &&
            reader->namespaceUri() == Private::kNamespace) {
          ret.release_notes_url_ = reader->readElementText().trimmed();
        } else if (reader->name() == "enclosure") {
          ret.download_url_ = reader->attributes().value("url").toString();
          ret.version_ = reader->attributes().value(Private::kNamespace, "version").toString();
        } else if (reader->name() == "description") {
          reader->readNext();
          ret.description_ = reader->text().toString();
        } else {
          reader->skipCurrentElement();
        }
      }
    }
    return ret;
  }

  Item latest_;
  QString error_reason_;

  static const char* kNamespace;
};

const char* AppCast::Private::kNamespace = "http://www.andymatuschak.org/xml-namespaces/sparkle";


AppCast::AppCast()
  : d(new Private)
{
}

AppCast::~AppCast() {
}

QString AppCast::version() const { return d->latest_.version_; }
QString AppCast::download_url() const { return d->latest_.download_url_; }
QString AppCast::release_notes_url() const { return d->latest_.release_notes_url_; }
QString AppCast::description() const { return d->latest_.description_; }
QString AppCast::error_reason() const { return d->error_reason_; }

bool AppCast::Load(QIODevice* dev) {
  QXmlStreamReader reader(dev);

  d->latest_ = Private::Item();
  while (!reader.atEnd()) {
    reader.readNext();

    if (reader.tokenType() == QXmlStreamReader::StartElement &&
        reader.name() == "item") {
      Private::Item item = d->LoadItem(&reader);
      if (d->latest_ < item)
        d->latest_ = item;
    }
  }

  d->error_reason_ = reader.errorString();
  return !reader.hasError();
}

} // namespace qtsparkle
