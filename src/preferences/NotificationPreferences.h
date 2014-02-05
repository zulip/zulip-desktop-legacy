#ifndef NOTIFICATIONPREFERENCES_H
#define NOTIFICATIONPREFERENCES_H

#include <QWidget>

class QButtonGroup;

namespace Ui {
class NotificationPreferences;
}

class NotificationPreferences : public QWidget
{
    Q_OBJECT
public:
    enum BounceSetting {
        BounceOnAll = 0,
        BounceOnPM,
        BounceNever
    };

    explicit NotificationPreferences(QWidget *parent = 0);
    ~NotificationPreferences();

    BounceSetting bounceSetting() const;
    void setBounceSetting(BounceSetting setting);

signals:
    // User clicked a special in-app link
    void linkClicked(const QString &link);

private slots:
    void linkActivated(const QString& link);

private:
    Ui::NotificationPreferences *ui;
    QButtonGroup *m_bounceGroup;
};

#endif // NOTIFICATIONPREFERENCES_H
