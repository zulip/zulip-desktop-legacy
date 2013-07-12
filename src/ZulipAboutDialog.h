#ifndef ZULIPABOUTDIALOG_H
#define ZULIPABOUTDIALOG_H

#include <QDialog>

namespace Ui
{
class ZulipAboutDialog;
}

class ZulipAboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ZulipAboutDialog(QWidget *parent = 0);
    ~ZulipAboutDialog();

signals:

public slots:

private:
    Ui::ZulipAboutDialog *m_ui;
};

#endif // ZULIPABOUTDIALOG_H
