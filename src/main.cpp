#include "ZulipWindow.h"
#include "ZulipApplication.h"
#include "Config.h"

int main(int argc, char *argv[])
{
    ZulipApplication a(argc, argv);
    a.setApplicationName("Zulip");
    a.setApplicationVersion(ZULIP_VERSION_STRING);

    if (argc == 2 && QString(argv[1]) == QString("--debug")) {
        a.setDebugMode(true);
    }

    QCoreApplication::setOrganizationName("Zulip");
    QCoreApplication::setOrganizationDomain("zulip.com");
    QCoreApplication::setApplicationName("Zulip Desktop");

    ZulipWindow w;
    if (argc == 3 && QString(argv[1]) == QString("--site")) {
        w.setUrl(QUrl(argv[2]));
    }

    a.setMainWindow(&w);

    w.show();

    return a.exec();
}
