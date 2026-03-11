#include "mainwindow.h"

#include <QDebug>
#include <QMessageBox>
#ifdef Q_OS_WIN
#include <QOperatingSystemVersion>
#endif

#include "z_stylesheet.h"
#include "dialoglogin.h"
#include "globalvars.h"
#include "logger.h"

#ifdef GC_HAVE_VLCQT
#include "myvlcplayer.h"
#endif




int main(int argc, char *argv[]) {

    // Install the Logger as the Qt message handler as early as possible so
    // that qWarning() / qCritical() calls from Qt internals and third-party
    // libraries are captured.  Default log level is Info; loadConfig() below
    // will override it from QSettings once the application identity is set.
    Logger::install();

    QApplication app(argc, argv);

#ifdef Q_OS_WIN
    // Windows 10 version 1703 (Creators Update, build 15063) is the minimum
    // required for the WinRT Bluetooth LE APIs used by Qt Bluetooth.
    if (QOperatingSystemVersion::current() <
            QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 15063)) {
        QMessageBox::critical(nullptr,
            QObject::tr("Unsupported Windows Version"),
            QObject::tr("MaximumTrainer requires Windows 10 version 1703 (Creators Update) or later.\n"
                        "Windows 7, 8, and 8.1 are not supported (missing WinRT Bluetooth LE APIs).\n\n"
                        "Please upgrade your operating system."));
        return 1;
    }
#endif


    //initialize global object (Account, Settings, SoundPlayer and QNetworkAccessManager)
    GlobalVars myVars;

    // Now that the application identity has been established by GlobalVars
    // (org "Max++ inc.", app "MaximumTrainer"), load logging preferences from
    // QSettings so the user's level / file-path choices take effect for the
    // rest of the session.
    Logger::instance().loadConfig();
    LOG_INFO("main", QStringLiteral("MaximumTrainer starting"));

//    MyVlcPlayer player;
//    player.setMinimumSize(QSize(500,300));
//    player.show();

//    WebBrowserV2 player;
//    player.setMinimumSize(QSize(500,300));
//    player.show();


    /// App Stylesheet (hack so I can type stylesheet in designer instead of source code)
    Z_StyleSheet styleSheetDummy;
    app.setStyleSheet(styleSheetDummy.styleSheet());

#ifndef Q_OS_WASM
    DialogLogin login;
    if (login.exec() != QDialog::Accepted) {
        return 0; // Login refused
    }
    if (login.getGotUpdate()) {
        return 0; // Executed DialogLogin and redirected to download new version
    }
#endif // Q_OS_WASM

    MainWindow w;
    w.show();


    return app.exec();
}




