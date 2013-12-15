/*
Copyright (C) 2011 by Mike McQuaid

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

#include <Foundation/NSString.h>
#include <AppKit/NSImage.h>
#include <QString>
#include <QVBoxLayout>
#include <QMacCocoaViewContainer>

#include "Config.h"

#ifdef QT5_BUILD
#include <QtMac>
#endif

static inline NSString* fromQString(const QString &string)
{
    const QByteArray utf8 = string.toUtf8();
    const char* cString = utf8.constData();
    return [[NSString alloc] initWithUTF8String:cString];
}

static inline QString toQString(NSString *string)
{
    if (!string)
        return QString();
    return QString::fromUtf8([string UTF8String]);
}

static inline NSImage* fromQPixmap(const QPixmap &pixmap)
{
#if defined(QT4_BUILD)
    CGImageRef cgImage = pixmap.toMacCGImageRef();
#elif defined(QT5_BUILD)
    CGImageRef cgImage = QtMac::toCGImageRef(pixmap);
#endif
    return [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
}

static inline void setupLayout(void *cocoaView, QWidget *parent)
{
    parent->setAttribute(Qt::WA_NativeWindow);
    QVBoxLayout *layout = new QVBoxLayout(parent);
    layout->setMargin(0);

#if defined(QT4_BUILD)
    QMacCocoaViewContainer *container = new QMacCocoaViewContainer(cocoaView, parent);
#elif defined(QT5_BUILD)
    NSView *view = (__bridge NSView*)cocoaView;
    QMacCocoaViewContainer *container = new QMacCocoaViewContainer(view, parent);
#endif

    layout->addWidget(container);
}
