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

    void askForDomain(bool isInitialDomain);

protected:
#ifdef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private slots:
    void askForDomainOK();
    void askForDomainCancel();

private:
    ZulipWindow* m_mw;
    bool m_debugMode;
    bool m_explicitDomain;

    // Internal
    QWeakPointer<QDialog> m_customServerDialog;
    QWeakPointer<QDialog> m_initialDomainDialog;
    QLineEdit * m_customDomain;
    QWeakPointer<QObject> m_customServerResponseObj;
};

#endif // ZULIPAPPLICATION_H
