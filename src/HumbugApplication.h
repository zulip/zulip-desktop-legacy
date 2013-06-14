#ifndef HUMBUGAPPLICATION_H
#define HUMBUGAPPLICATION_H

#include <QApplication>

class HumbugWindow;

class HumbugApplication : public QApplication
{
    Q_OBJECT
public:
    explicit HumbugApplication(int & argc, char ** argv);

    void setMainWindow(HumbugWindow* mw) { m_mw = mw; }

protected:
#ifndef Q_OS_MAC
    bool macEventFilter(EventHandlerCallRef, EventRef) { return false; }
#else
    bool macEventFilter(EventHandlerCallRef, EventRef);
#endif

private:
    HumbugWindow* m_mw;
};

#endif // HUMBUGAPPLICATION_H
