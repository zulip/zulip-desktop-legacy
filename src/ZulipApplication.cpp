#include "ZulipApplication.h"

#include "ZulipWindow.h"

#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QWeakPointer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include <iostream>

ZulipApplication::ZulipApplication(int &argc, char **argv)
    :  QApplication(argc, argv),
      m_mw(0),
      m_debugMode(false),
      m_explicitDomain(false)
{
}


bool ZulipApplication::explicitDomain() const {
    return m_explicitDomain;
}

void ZulipApplication::setExplicitDomain(const QString &domain) {
    QSettings s;

    if (domain.isEmpty()) {
        s.remove("Domain");
        m_explicitDomain = false;
    } else {
        s.setValue("Domain", domain);
        m_explicitDomain = true;
    }
}


void ZulipApplication::askForDomain(bool isInitialDomain)
{
    m_initialDomainDialog = QWeakPointer<QDialog>(new QDialog(APP->mainWindow()));
    //m_initialDomainDialog.data()->setWindowModality(Qt::WindowModal);
    m_initialDomainDialog.data()->setLayout(new QVBoxLayout());

    m_initialDomainDialog.data()->setWindowTitle("Add Zulip Domain");
    QLabel *msgLabel = new QLabel(m_initialDomainDialog.data());
    if (isInitialDomain) {
      msgLabel->setText("<h3>Welcome to Zulip.</h3>"
                  "It looks like you are using Zulip app for the first time.<br>"
                  "Please enter the domain for your Zulip server below.<br>");
    } else {
      msgLabel->setText("<h3>Add New Domain</h3>"
                  "Please enter the domain for your Zulip server below.<br>");
    }

    m_initialDomainDialog.data()->layout()->addWidget(msgLabel);

    QHBoxLayout *domainEntry = new QHBoxLayout();
    domainEntry->setSpacing(2);
    QLabel *domainLabel = new QLabel("https://", m_initialDomainDialog.data());
    m_customDomain = new QLineEdit(m_initialDomainDialog.data());
    m_customDomain->setPlaceholderText("your_zulip_server_domain.com");
    domainEntry->addWidget(domainLabel);
    domainEntry->addWidget(m_customDomain);
    m_initialDomainDialog.data()->layout()->addItem(domainEntry);

    QDialogButtonBox::StandardButtons buttons = isInitialDomain ?
      QDialogButtonBox::Ok :
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(buttons);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(askForDomainOK()));
    if (!isInitialDomain) {
      connect(buttonBox, SIGNAL(rejected()), this, SLOT(askForDomainCancel()));
    } else {
      connect(m_initialDomainDialog.data(), SIGNAL(rejected()), this, SLOT(quit()));
    }

    m_initialDomainDialog.data()->layout()->addWidget(buttonBox);
    m_initialDomainDialog.data()->show();
}

void ZulipApplication::askForDomainOK()
{
    if (m_initialDomainDialog.isNull()) {
        return;
    }
    QString domain = m_customDomain->text().trimmed();
    if (domain.length() == 0) {
      return;
    }

    if (domain.startsWith("localhost")) {
      domain = "http://" + domain;
    } else if (domain.startsWith("http://localhost")) {
      // this is fine
    } else if (domain.startsWith("https://localhost")) {
      domain.replace("https://", "http://");
    } else if (domain.startsWith("http://")) {
        domain.replace("http://", "https://");
    } else if (domain.length() > 0 && !domain.startsWith("https://")) {
        domain = "https://" + domain;
    }

    m_mw->addNewDomainSelection(domain);

    m_mw->setUrl(QUrl(domain));
    setExplicitDomain(domain);

    m_initialDomainDialog.data()->hide();
    m_initialDomainDialog.data()->deleteLater();

    m_mw->show();
}

void ZulipApplication::askForDomainCancel()
{
    if (m_initialDomainDialog.isNull()) {
      return;
    }
    m_initialDomainDialog.data()->hide();
    m_initialDomainDialog.data()->deleteLater();
}
