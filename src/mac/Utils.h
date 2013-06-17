#ifndef UTILS_H
#define UTILS_H

#include <Foundation/NSString.h>

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

    return QUrl(toQString([url absoluteString]));
}

#endif
