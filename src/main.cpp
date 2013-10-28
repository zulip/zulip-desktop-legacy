#include "ZulipWindow.h"
#include "ZulipApplication.h"
#include "Config.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
    ZulipApplication a(argc, argv);
    a.setApplicationName("Zulip");
    a.setApplicationVersion(ZULIP_VERSION_STRING);

    Logging::setupLogging();

    if (argc == 2 && QString(argv[1]) == QString("--debug")) {
        a.setDebugMode(true);
    }

    QCoreApplication::setOrganizationName("Zulip");
    QCoreApplication::setOrganizationDomain("zulip.com");
    QCoreApplication::setApplicationName("Zulip Desktop");

    ZulipWindow w;
    if (argc == 3 && QString(argv[1]) == QString("--site")) {
        const QString domain = argv[2];
        w.setUrl(QUrl(domain));
        a.setExplicitDomain(domain);
    }

    a.setMainWindow(&w);

    w.show();

    return a.exec();
}
