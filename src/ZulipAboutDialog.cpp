#include "ZulipAboutDialog.h"

#include "ui_ZulipAboutDialog.h"
#include "Config.h"
#include "ZulipApplication.h"
#include "ZulipWindow.h"

#include <QFile>
#include <QDebug>

ZulipAboutDialog::ZulipAboutDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::ZulipAboutDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->webView, SIGNAL(linkClicked(QUrl)), APP->mainWindow(), SLOT(linkClicked(QUrl)));

    QFile html(":/about.html");
    if (!html.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read from qrc:/about.html";
    } else {
        QString contents = QString::fromUtf8(html.readAll());
        // Add version string

        QString license;
#ifdef SSO_BUILD
        license = "the Zulip Enterprise License Agreement";
#else
        license = "the license described in the <a href=\"https://zulip.com/terms\" target='_blank'>Zulip Terms of Service</a>";
#endif
        contents = contents.replace("{{ZULIP_VERSION_STRING}}", ZULIP_VERSION_STRING);
        contents = contents.replace("{{ZULIP_LICENSE_AGREEMENT}}", license);
        m_ui->webView->loadHTML(contents);
    }
}


ZulipAboutDialog::~ZulipAboutDialog()
{
}
