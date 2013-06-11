#ifndef HUMBUGABOUTDIALOG_H
#define HUMBUGABOUTDIALOG_H

#include <QDialog>

namespace Ui
{
class HumbugAboutDialog;
}

class HumbugAboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HumbugAboutDialog(QWidget *parent = 0);
    ~HumbugAboutDialog();

signals:

public slots:

private:
    Ui::HumbugAboutDialog *m_ui;
};

#endif // HUMBUGABOUTDIALOG_H
