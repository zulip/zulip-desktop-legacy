#ifndef HUMBUGTRAYICON_H
#define HUMBUGTRAYICON_H

#include <QSystemTrayIcon>

class HumbugTrayIcon : public QSystemTrayIcon
{
  Q_OBJECT
public:
  explicit HumbugTrayIcon(QObject *parent = 0);

signals:

public slots:

};

#endif // HUMBUGTRAYICON_H
