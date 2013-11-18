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
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    const QByteArray utf8 = url.toEncoded();
    NSURL* nsU = [NSURL URLWithString:fromQBA(utf8)];

    [nsU retain];

    [pool drain];
    return nsU;
}

static inline QUrl toQUrl(NSURL* url) {
    if (!url)
        return QUrl();

    return QUrl::fromEncoded(toQString([url absoluteString]).toUtf8());
}

#endif
