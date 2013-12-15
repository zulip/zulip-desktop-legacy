#ifndef ZULIPWEBBRIDGE_H
#define ZULIPWEBBRIDGE_H

#include <QObject>
#include <QVariant>

class ZulipWebBridge : public QObject
{
    Q_OBJECT
public:
    explicit ZulipWebBridge(QObject *parent = 0);

public slots:
    QVariantMap systemInfo();

    void desktopNotification(const QString &title, const QString &content);
    void bell();
    void updateCount(int count);
    void updatePMCount(int count);
    void log(const QString& msg);
    QString desktopAppVersion();

signals:
    void doUpdateCount(int newCount);
    void doUpdatePMCount(int newCount);
    void doDesktopNotification(const QString& title, const QString& content);
    void doBell();

};

#endif // ZULIPWEBBRIDGE_H
