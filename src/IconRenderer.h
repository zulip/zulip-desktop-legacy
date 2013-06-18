#ifndef ICONRENDERER_H
#define ICONRENDERER_H

#include <QIcon>
#include <QHash>
#include <QObject>
#include <QSize>
#include <QPixmap>

class QSvgRenderer;

/**
 * Handles rendering & caching the Humbug hat with
 * optional additional configuration options
 */
class IconRenderer : public QObject
{
    Q_OBJECT
public:
    explicit IconRenderer(const QString& svgPath, QObject *parent = 0);

    // Get an icon for the desired unread count
    QIcon icon(int unreadNormal = -1, int unreadPMs = -1);

    // Get the person icon
    QIcon personIcon();

private:
    QString cacheKey(const QSize& size, int unreadNormal, int unreadPMs) const;
    QPixmap pixmap(const QSize& size,  int unreadNormal = -1, int unreadPMs = -1);
    QPixmap render(const QSize& size, int unreadNormal, int unreadPMs);

    QString m_svgPath;
    QSvgRenderer* m_renderer;
    QList<int> m_defaultSizes;
    QIcon m_personIcon;
    QHash<QString, QIcon> m_iconCache;
};

#endif // ICONRENDERER_H
