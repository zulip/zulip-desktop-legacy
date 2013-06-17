#ifndef HUMBUGAPPLICATION_H
#define HUMBUGAPPLICATION_H

#include <QApplication>

#define APP static_cast<HumbugApplication*>(qApp)

class HumbugWindow;

class HumbugApplication : public QApplication
{
    Q_OBJECT
public:
    explicit HumbugApplication(int & argc, char ** argv) : QApplication(argc, argv) {}

    void setMainWindow(HumbugWindow* mw) { m_mw = mw; }

#ifdef Q_OS_MAC
    void desktopNotification(const QString& title, const QString& msg);
#endif

protected:
#ifdef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private:
    HumbugWindow* m_mw;
};

#endif // HUMBUGAPPLICATION_H
