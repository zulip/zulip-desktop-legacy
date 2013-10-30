#ifndef ZULIPAPPLICATION_H
#define ZULIPAPPLICATION_H

#include <QApplication>
#include <QSettings>

#define APP static_cast<ZulipApplication*>(qApp)

class ZulipWindow;

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
protected:
#ifdef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private:
    ZulipWindow* m_mw;
    bool m_debugMode;
    bool m_explicitDomain;
};

#endif // ZULIPAPPLICATION_H
