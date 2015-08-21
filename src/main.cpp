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

    if (numerOfDomains) {
      QString domain = settings.value("Domain").toString();
      if (!domain.length()) {
        settings.beginReadArray("InstanceDomains");
        settings.setArrayIndex(numerOfDomains-1);
        QString domainUrl = settings.value("url").toString();
        settings.endArray();
        w.setUrl(QUrl(domainUrl));
        a.setExplicitDomain(domainUrl);
        w.show();
      } else {
        w.setUrl(QUrl(domain));
        a.setExplicitDomain(domain);
        w.show();
      }
    } else {
      a.askForDomain(true);
    }

    return a.exec();
}
