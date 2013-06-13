#ifndef HWEBVIEW_H
#define HWEBVIEW_H

#include <QWidget>
#include <QUrl>

class HWebViewPrivate;

/**
 * HWebView wraps a QWebView on non-mac, and a native Cocoa WebView on OS X.
 *
 * This is due to bugginess on OS X in QtWebKit that makes it unusable. When adding
 * functionality to this class, make sure to implement it both in the HWebView.cpp and
 * HWebView_mac.mm files.
 */
class HWebView : public QWidget
{
    Q_OBJECT
public:
    HWebView(QWidget *parent);
    virtual ~HWebView();

    void load(const QUrl& url);
    void setUrl(const QUrl& url);

    QUrl url() const;

signals:
    void linkClicked(const QUrl& url);

    // Humbug notifications
    void desktopNotification(const QString& ,const QString&);
    void updateCount(int);
    void bell();

private:
    friend class ::HWebViewPrivate;
    HWebViewPrivate* dptr;
};

#endif
