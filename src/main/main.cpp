#include "mainwindow.h"

#include <QDebug>


#include "z_stylesheet.h"
#include "dialoglogin.h"
#include "globalvars.h"

#include "myvlcplayer.h"
#include "webbrowserv2.h"

#include "googlemapwidget.h"
//#include "webbrowserv2.h"



int main(int argc, char *argv[]) {


    QApplication app(argc, argv);


    //initialize global object (Account, Settings, SoundPlayer and QNetworkAccessManager)
    GlobalVars myVars;

//    MyVlcPlayer player;
//    player.setMinimumSize(QSize(500,300));
//    player.show();

//    WebBrowserV2 player;
//    player.setMinimumSize(QSize(500,300));
//    player.show();


    /// App Stylesheet (hack so I can type stylesheet in designer instead of source code)
    Z_StyleSheet styleSheetDummy;
    app.setStyleSheet(styleSheetDummy.styleSheet());

    DialogLogin login;
    if (login.exec() != QDialog::Accepted) {
        return 0; // Login refused
    }
    if (login.getGotUpdate()) {
        return 0; // Executed DialogLogin and redirected to download new version
    }
    MainWindow w;
    w.show();


    return app.exec();
}




