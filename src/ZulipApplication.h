#ifndef ZULIPAPPLICATION_H
#define ZULIPAPPLICATION_H

#include <QApplication>

#define APP static_cast<ZulipApplication*>(qApp)

class ZulipWindow;

class ZulipApplication : public QApplication
{
    Q_OBJECT
public:
    explicit ZulipApplication(int & argc, char ** argv) : QApplication(argc, argv),
                                                          m_mw(0),
                                                          m_debugMode(false)
    {
    }

    void setMainWindow(ZulipWindow* mw) { m_mw = mw; }
    ZulipWindow* mainWindow() const { return m_mw; }

    void setDebugMode(bool debugMode) { m_debugMode = debugMode; }
    bool debugMode() const { return m_debugMode; }

protected:
#ifdef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private:
    ZulipWindow* m_mw;
    bool m_debugMode;
};

#endif // ZULIPAPPLICATION_H
