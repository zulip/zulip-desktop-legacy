#ifndef UTILS_MAC_H
#define UTILS_MAC_H

#include "Converters.h"

#include <Foundation/NSString.h>
#include <Foundation/NSAutoreleasePool.h>

#include <QUrl>

// This file can only be #include-d from .mm files

static inline  NSString* fromQBA(const QByteArray& ba) {
    const char* cData = ba.constData();
    return [[NSString alloc] initWithUTF8String:cData];
}

static inline NSURL* fromQUrl(const QUrl& url) {
    const QByteArray utf8 = url.toEncoded();
    return [NSURL URLWithString:fromQBA(utf8)];
}

static inline QUrl toQUrl(NSURL* url) {
    if (!url)
        return QUrl();

    return QUrl::fromEncoded(toQString([url absoluteString]).toUtf8());
}


static inline QByteArray fromNSData(NSData *data) {
    if (!data) {
        return QByteArray();
    }

    QByteArray qData;
    qData.resize([data length]);
    [data getBytes:qData.data() length: qData.size()];
    return qData;
}

#endif
