#include "Logger.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QTime>

#include <fstream>
#include <iostream>

using namespace std;
ofstream logfile;

namespace Logging {

void logHandler(QtMsgType type, const char *msg) {

    logfile << QTime::currentTime().toString().toUtf8().data();
    switch (type) {
        case QtDebugMsg:
            logfile << " Debug: " << msg << endl;
            break;
        case QtCriticalMsg:
            logfile << " Critical: " << msg << endl;
            break;
        case QtWarningMsg:
            logfile << " Warning: " << msg << endl;
            break;
        case QtFatalMsg:
            logfile << " Fatal: " << msg << endl;
            abort();
    }
    logfile.flush();

    cout << QTime::currentTime().toString().toUtf8().data() << "\t" << msg << endl << flush;
}


QDir
loggingDirectory()
{
#ifdef Q_OS_LINUX
    return QDir::home().filePath( ".local/share/Zulip" );
#elif defined(Q_OS_WIN)
    return QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#elif defined(Q_OS_MAC)
    return QDir( QDir::homePath() + "/Library/Logs" );
#endif
}

void setupLogging() {
    QDir logFileDir = loggingDirectory();
    logFileDir.mkpath(logFileDir.absolutePath());

    const QString filePath = logFileDir.absoluteFilePath("Zulip.log");
    logfile.open( filePath.toLocal8Bit(), ios::app );

    cout.flush();
    qInstallMsgHandler( logHandler );
}

}
