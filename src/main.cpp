#include "ZulipWindow.h"
#include "ZulipApplication.h"
#include "Config.h"
#include "Logger.h"

#include <QDebug>
#include <QSslConfiguration>

int main(int argc, char *argv[])
{
    ZulipApplication a(argc, argv);
    a.setApplicationName("Zulip Desktop");
    a.setOrganizationName("Zulip");
    a.setOrganizationDomain("zulip.org");
    a.setApplicationVersion(ZULIP_VERSION_STRING);

    Logging::setupLogging();

	QString cmdlinedomain = "";
	bool allow_insecure = false;
	if (argc > 1) {
		int i = 1;
		while (i < argc) {
			if (QString(argv[i]).toLower() == QString("--debug").toLower()) {
				a.setDebugMode(true);
			}
			else if (QString(argv[i]).toLower() == QString("--site").toLower() && (i+1 < argc) ) {
				cmdlinedomain = argv[i + 1];
				++i;
			}
			else if (QString(argv[i]).toLower() == QString("--allow-insecure").toLower()) {
				allow_insecure = true;
			}
			++i;
		}
	}

	if (allow_insecure) {
		QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
		sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
		QSslConfiguration::setDefaultConfiguration(sslConf);
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
    if (cmdlinedomain != QString("")) {
        domain = cmdlinedomain;
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
        // Hide tray icon until the domain has been selected
        w.trayIcon()->hide();
        a.askForDomain(true);
    }

    return a.exec();
}
