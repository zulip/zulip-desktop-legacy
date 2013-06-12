#ifndef WHEELFILTER_H
#define WHEELFILTER_H

#include <QObject>
#include <QPoint>

class QWebView;

class QTimer;

/**
 * Hack to work around the fact that scrolling on OS X QWebView
 * is super laggy right now. Consolidate scroll events into larger
 * resolution chunks
 */
class WheelFilter : public QObject
{
    Q_OBJECT
public:
    explicit WheelFilter(QWebView* filtered, QObject *parent = 0);
    virtual ~WheelFilter() {}

    bool eventFilter(QObject *obj, QEvent *e);

public slots:
    void aggregateScroll();

private:
    QWebView* m_filtered;
    QTimer* m_timer;

    QPoint m_startPos;
    QPoint m_startGlobalPos;

    int m_xdelta;
    int m_ydelta;

};

#endif // WHEELFILTER_H
