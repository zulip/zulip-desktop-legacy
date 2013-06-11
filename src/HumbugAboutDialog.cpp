#include "HumbugAboutDialog.h"
#include "ui_HumbugAboutDialog.h"

HumbugAboutDialog::HumbugAboutDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::HumbugAboutDialog)
{
    m_ui->setupUi(this);
    m_ui->webView->load(QUrl("qrc:/about.html"));
}


HumbugAboutDialog::~HumbugAboutDialog()
{
}
