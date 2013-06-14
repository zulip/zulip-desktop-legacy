#include "HumbugWindow.h"
#include "HumbugApplication.h"

int main(int argc, char *argv[])
{
    HumbugApplication a(argc, argv);
    a.setApplicationName("Humbug Desktop");
    a.setApplicationVersion("0.1");

    QCoreApplication::setOrganizationName("Humbug");
    QCoreApplication::setOrganizationDomain("humbug.com");
    QCoreApplication::setApplicationName("Humbug Desktop");

    HumbugWindow w;
    if (argc == 3 && QString(argv[1]) == QString("--site")) {
        w.setUrl(QUrl(argv[2]));
    }

    a.setMainWindow(&w);

    w.show();

    return a.exec();
}
