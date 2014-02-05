#ifndef GENERALPREFERENCES_H
#define GENERALPREFERENCES_H

#include <QWidget>

namespace Ui {
class GeneralPreferences;
}

class GeneralPreferences : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralPreferences(QWidget *parent = 0);
    ~GeneralPreferences();

    bool startAtLogin() const;
    void setStartAtLogin(bool start);

    bool showTrayIcon() const;
    void setShowTrayIcon(bool showTrayIcon);
private:
    Ui::GeneralPreferences *ui;
};

#endif // GENERALPREFERENCES_H
