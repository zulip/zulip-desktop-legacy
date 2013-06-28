#include "HumbugAboutDialog.h"

#include "ui_HumbugAboutDialog.h"
#include "Config.h"

#include <QFile>
#include <QDebug>

HumbugAboutDialog::HumbugAboutDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::HumbugAboutDialog)
{
    m_ui->setupUi(this);

    QFile html(":/about.html");
    if (!html.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read from qrc:/about.html";
    } else {
        QString contents = QString::fromUtf8(html.readAll());
        // Add version string
        contents = contents.arg(HUMBUG_VERSION_STRING);
        m_ui->webView->loadHTML(contents);
    }
}


HumbugAboutDialog::~HumbugAboutDialog()
{
}
