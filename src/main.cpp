#include "ZulipWindow.h"
#include "ZulipApplication.h"
#include "Config.h"
#include "Logger.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    ZulipApplication a(argc, argv);
    a.setApplicationName("Zulip Desktop");
    a.setOrganizationName("Zulip");
    a.setOrganizationDomain("zulip.org");
    a.setApplicationVersion(ZULIP_VERSION_STRING);

    Logging::setupLogging();

    if (argc == 2 && QString(argv[1]) == QString("--debug")) {
        a.setDebugMode(true);
    }

    ZulipWindow w;

    a.setMainWindow(&w);

    QSettings settings;
    int numerOfDomains = settings.beginReadArray("InstanceDomains");
    settings.endArray();

    QString settingsDomain = settings.value("Domain").toString();

    // Priority order:
    // 1. --site command-line flag
    // 2. Domain= explicit setting
    // 3. Last domain in the InstanceDomains list
    QString domain;
    if (argc == 3 && QString(argv[1]) == QString("--site")) {
        domain = argv[2];
    } else if (!settingsDomain.isEmpty()) {
        domain = settingsDomain;
    } else if (numerOfDomains > 0) {
        settings.beginReadArray("InstanceDomains");
        settings.setArrayIndex(numerOfDomains-1);
        domain = settings.value("url").toString();
        settings.endArray();
    }

    // If we still don't have a domain to use, prompt for one
    if (!domain.isEmpty()) {
        w.setUrl(QUrl(domain));
        a.setExplicitDomain(domain);

        w.show();
    } else {
        a.askForDomain(true);
    }

    return a.exec();
}
