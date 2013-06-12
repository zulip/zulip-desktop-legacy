#include "WheelFilter.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QWidget>
#include <QWebView>

WheelFilter::WheelFilter(QWebView* filtered, QObject *parent) :
    QObject(parent)
  , m_filtered(filtered)
  , m_timer(new QTimer(this))
  , m_xdelta(0)
  , m_ydelta(0)
{
    m_timer->setSingleShot(true);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(aggregateScroll()));
}

bool WheelFilter::eventFilter(QObject *obj, QEvent *e) {
    if (e->type() == QEvent::Wheel) {
        QWheelEvent* we = static_cast<QWheelEvent*>(e);

        if (we->orientation() == Qt::Horizontal)
            m_xdelta += we->delta();
        else
            m_ydelta += we->delta();

        if (!m_timer->isActive()) {
            m_startGlobalPos = we->globalPos();
            m_startPos = we->pos();

            m_timer->start(35);
        }

        return true;
    }

    return false;
}

void WheelFilter::aggregateScroll() {
    if (!m_filtered)
        return;

    if (m_ydelta != 0) {
        QWheelEvent ev(m_startPos, m_ydelta, Qt::NoButton, Qt::NoModifier, Qt::Vertical);
        m_filtered->event(&ev);
    }

    if (m_xdelta != 0) {
        QWheelEvent ev(m_startPos, m_xdelta, Qt::NoButton, Qt::NoModifier, Qt::Horizontal);
        m_filtered->event(&ev);
    }

    m_xdelta = m_ydelta = 0;
    m_startGlobalPos = m_startPos = QPoint();
}
