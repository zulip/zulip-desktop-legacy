#include "HumbugWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Humbug Desktop");
    a.setApplicationVersion("0.1");

    HumbugWindow w;
    if (argc == 3 && QString(argv[1]) == QString("--site")) {
        w.setUrl(QUrl(argv[2]));
    }

    w.show();

    return a.exec();
}
