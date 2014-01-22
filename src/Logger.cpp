#include "Logger.h"

#include "Config.h"

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

    // Only show Critical and Fatal debug messages on the console
    if ((type == QtDebugMsg || type == QtWarningMsg)) {
        return;
    }

    cout << QTime::currentTime().toString().toUtf8().data() << "\t" << msg << endl << flush;
}

#ifdef QT5_BUILD
void qt5LogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    logHandler(type, msg.toUtf8());
}
#endif

QDir
loggingDirectory()
{
#if defined(Q_OS_WIN)
    return QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#elif defined(Q_OS_MAC)
    return QDir( QDir::homePath() + "/Library/Logs" );
#else
    return QDir::home().filePath( ".local/share/Zulip" );
#endif
}

void setupLogging() {
    QDir logFileDir = loggingDirectory();
    logFileDir.mkpath(logFileDir.absolutePath());

    const QString filePath = logFileDir.absoluteFilePath("Zulip.log");
    logfile.open( filePath.toLocal8Bit(), ios::app );

    cout.flush();

#if defined(QT4_BUILD)
    qInstallMsgHandler( logHandler );
#elif defined(QT5_BUILD)
    qInstallMessageHandler( qt5LogHandler );
#endif
}

}
