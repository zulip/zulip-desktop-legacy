#include "GeneralPreferences.h"
#include "ui_GeneralPreferences.h"

#include <QIcon>
GeneralPreferences::GeneralPreferences(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralPreferences)
{
    ui->setupUi(this);
}

GeneralPreferences::~GeneralPreferences()
{
    delete ui;
}

void GeneralPreferences::setShowTrayIcon(bool showTrayIcon) {
    ui->showTrayIcon->setChecked(showTrayIcon);
}

bool GeneralPreferences::showTrayIcon() const {
    return ui->showTrayIcon->checkState() == Qt::Checked;
}

void GeneralPreferences::setStartAtLogin(bool start) {
    ui->startAtLogin->setChecked(start);
}

bool GeneralPreferences::startAtLogin() const {
    return ui->startAtLogin->checkState() == Qt::Checked;
}
