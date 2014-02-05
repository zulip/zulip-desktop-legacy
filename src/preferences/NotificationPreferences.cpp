#include "NotificationPreferences.h"
#include "ui_NotificationPreferences.h"

#include <QButtonGroup>
#include <QDebug>

NotificationPreferences::NotificationPreferences(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotificationPreferences)
{
    ui->setupUi(this);

    connect(ui->webpageSettingsLabel, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));

    // On non-OS X there's no Dock icon, but a tray icon instead
#ifndef Q_OS_MAC
    QString titleText = ui->bounceTitleLabel->text();
    titleText.replace("bounce dock icon", "flash tray icon");
    ui->bounceTitleLabel->setText(titleText);

    ui->verticalLayout->setSpacing(0);
    ui->verticalLayout_2->setSizeConstraint(QLayout::SetMinimumSize);
#endif

    m_bounceGroup = new QButtonGroup(this);
    m_bounceGroup->setExclusive(true);

    m_bounceGroup->addButton(ui->bounceNever, BounceNever);
    m_bounceGroup->addButton(ui->bounceOnPrivate, BounceOnPM);
    m_bounceGroup->addButton(ui->bounceOnAll, BounceOnAll);
}

NotificationPreferences::~NotificationPreferences()
{
    delete ui;
}

NotificationPreferences::BounceSetting NotificationPreferences::bounceSetting() const {
    return (BounceSetting)m_bounceGroup->checkedId();
}

void NotificationPreferences::setBounceSetting(BounceSetting setting) {
    QAbstractButton *button = m_bounceGroup->button(setting);
    button->setChecked(true);
}

void NotificationPreferences::linkActivated(const QString &link) {
    if (!link.startsWith("#")) {
        return;
    }

    // Close settings dialog, go to link in main view
    emit linkClicked(link);
}
