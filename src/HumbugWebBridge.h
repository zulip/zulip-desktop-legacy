#ifndef HUMBUGWEBBRIDGE_H
#define HUMBUGWEBBRIDGE_H

#include <QObject>
#include <QWebView>
#include <QVariant>
#include "HumbugTrayIcon.h"

class HumbugWebBridge : public QObject
{
    Q_OBJECT
public:
    explicit HumbugWebBridge(QObject *parent = 0, QWebView *wv = 0, HumbugTrayIcon *ti = 0);
    
private:
    QWebView *webView;
    HumbugTrayIcon *trayIcon;
    int unreadCount;

signals:
    
public slots:
    QVariantMap systemInfo();
    void notify(const QVariant &msg);
    void updateCount(int count);
    int getCount();
    
};

#endif // HUMBUGWEBBRIDGE_H
