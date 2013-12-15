#ifndef ZULIPAPPLICATION_H
#define ZULIPAPPLICATION_H

#include <QApplication>
#include <QSettings>
#include <QWeakPointer>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

#define APP static_cast<ZulipApplication*>(qApp)

class ZulipWindow;
class QButton;
class QLineEdit;

class ZulipApplication : public QApplication
{
    Q_OBJECT
public:
    explicit ZulipApplication(int & argc, char ** argv);

    void setMainWindow(ZulipWindow* mw) { m_mw = mw; }
    ZulipWindow* mainWindow() const { return m_mw; }

    void setDebugMode(bool debugMode) { m_debugMode = debugMode; }
    bool debugMode() const { return m_debugMode; }

    bool explicitDomain() const;
    void setExplicitDomain(const QString& domain);

#ifdef Q_OS_MAC
    bool bounceDockIcon() { return m_bounceDockIcon; };
    void setBounceDockIcon(bool bounce) { m_bounceDockIcon = bounce; }
#endif

    // If we were unable to preflight a request,
    // ask the user to enter their custom server if
    // there is one
    // Will call: success(QString) or retry() as a
    // SLOT on the response object
    void askForCustomServer(QObject *responseObj);
protected:
#ifdef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private slots:
    void customServerOK();
    void customServerCancel();

private:
    ZulipWindow* m_mw;
    bool m_debugMode;
    bool m_explicitDomain;
    bool m_bounceDockIcon;

    // Internal
    QWeakPointer<QDialog> m_customServerDialog;
    QLineEdit * m_customDomain;
    QWeakPointer<QObject> m_customServerResponseObj;
};

#endif // ZULIPAPPLICATION_H
