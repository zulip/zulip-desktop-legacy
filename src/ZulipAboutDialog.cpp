#include "ZulipAboutDialog.h"

#include "ui_ZulipAboutDialog.h"
#include "Config.h"

#include <QFile>
#include <QDebug>

ZulipAboutDialog::ZulipAboutDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::ZulipAboutDialog)
{
    m_ui->setupUi(this);

    QFile html(":/about.html");
    if (!html.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read from qrc:/about.html";
    } else {
        QString contents = QString::fromUtf8(html.readAll());
        // Add version string
        contents = contents.arg(ZULIP_VERSION_STRING);
        m_ui->webView->loadHTML(contents);
    }
}


ZulipAboutDialog::~ZulipAboutDialog()
{
}
