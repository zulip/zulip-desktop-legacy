#include "ZulipApplication.h"

#include "ZulipWindow.h"

#include "../thirdparty/qocoa/qbutton.h"

#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QWeakPointer>
#include <QHBoxLayout>
#include <QVBoxLayout>

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

void ZulipApplication::askForCustomServer(std::function<void (QString)> success)
{
    m_customDomainSuccess = success;

    m_customServerDialog = QWeakPointer<QDialog>(new QDialog(APP->mainWindow()));
    m_customServerDialog.data()->setWindowFlags(Qt::Sheet);
    m_customServerDialog.data()->setWindowModality(Qt::WindowModal);
    m_customServerDialog.data()->setLayout(new QVBoxLayout());

    m_customServerDialog.data()->setWindowTitle("Zulip Domain Selection");
    QLabel *msgLabel = new QLabel(m_customServerDialog.data());
    msgLabel->setText("<h3>We take your security as seriously as you do.</h3>"
                "We were unable to contact <code>zulip.com</code>. It's likely the case<br>"
                "that either you are not connected to the Internet (in which case<br>"
                "\"Retry\" below), or you are trying to access a self-hosted<br>"
                "installation of Zulip behind a strict firewall.<br><br>"
                "In that case, we appreciate your security consciousness! Please<br>"
                "enter the domain for your Zulip installation below.");
    m_customServerDialog.data()->layout()->addWidget(msgLabel);

    QHBoxLayout *domainEntry = new QHBoxLayout();
    domainEntry->setSpacing(2);
    QLabel *domainLabel = new QLabel("https:", m_customServerDialog.data());
    m_customDomain = new QLineEdit(m_customServerDialog.data());
    m_customDomain->setPlaceholderText("https://zulip.com");
    domainEntry->addWidget(domainLabel);
    domainEntry->addWidget(m_customDomain);
    m_customServerDialog.data()->layout()->addItem(domainEntry);

    QHBoxLayout *buttons = new QHBoxLayout();
    QButton *cancelButton = new QButton(m_customServerDialog.data());
    cancelButton->setText("Cancel");
    QButton *okButton = new QButton(m_customServerDialog.data());
    okButton->setText("OK");

    buttons->addSpacing(60);
    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);

    m_customServerDialog.data()->layout()->addItem(buttons);
    m_customServerDialog.data()->show();

    connect(okButton, SIGNAL(clicked()), this, SLOT(customServerOK()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(customServerCancel()));

}

void ZulipApplication::customServerOK()
{
    if (m_customServerDialog.isNull()) {
        return;
    }
    const QString domain = m_customDomain->text().trimmed();
    qDebug() << "Setting explicit domain to:" << domain;
    setExplicitDomain(domain);
    m_customDomainSuccess(domain);

    m_customServerDialog.data()->hide();
    m_customServerDialog.data()->deleteLater();
}

void ZulipApplication::customServerCancel()
{
    if (m_customServerDialog.isNull()) {
        return;
    }
    // User hit cancel---abort the login completely.
    m_customServerDialog.data()->hide();
    m_customServerDialog.data()->deleteLater();
}
