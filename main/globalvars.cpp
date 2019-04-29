#include "globalvars.h"

#include <QNetworkAccessManager>

#include "account.h"
#include "settings.h"
#include "soundplayer.h"
#include "sensor.h"
#include "fec_controller.h"
#include "oxygen_controller.h"
#include "power_controller.h"
#include "trackpoint.h"
#include "userstudio.h"

#include <QWebEngineSettings>
#include <VLCQtCore/Common.h>








GlobalVars::GlobalVars(QObject *parent) :
    QObject(parent)
{

    qDebug() << "SSL version" << QSslSocket::sslLibraryBuildVersionString();

    QCoreApplication::setOrganizationName("Max++ inc.");
    QCoreApplication::setOrganizationDomain("maximumtrainer.com");
    QCoreApplication::setApplicationName("MaximumTrainer");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);


    // Used in Signal/Slots connection
    qRegisterMetaType<PowerCurve>("PowerCurve");
    qRegisterMetaType<Sensor>("Sensor");
    qRegisterMetaType<QList<Sensor> >( "QList<Sensor>" );
    qRegisterMetaType<QList<int> >( "QList<int>" );
    qRegisterMetaType<CalibrationType>("CalibrationType");
    qRegisterMetaType<QVector<UserStudio> >( "QVector<UserStudio>" );
    qRegisterMetaType<Oxygen_Controller::COMMAND>("Oxygen_Controller::COMMAND");
    qRegisterMetaType<FEC_Controller::CALIBRATION_TYPE>("FEC_Controller::CALIBRATION_TYPE");
    qRegisterMetaType<FEC_Controller::TEMPERATURE_CONDITION>("FEC_Controller::TEMPERATURE_CONDITION");
    qRegisterMetaType<FEC_Controller::SPEED_CONDITION>("FEC_Controller::SPEED_CONDITION");
    qRegisterMetaType<QList<Trackpoint> >( "QList<Trackpoint>" );




    //plugin path using by libvlc
    VlcCommon::setPluginPath(qApp->applicationDirPath() + "/plugins");


    ///-------- INIT GLOBAL VAR ---------------------------
    Account *account = new Account(this);
    Settings *settings = new Settings(this);
    SoundPlayer *soundPlayer = new SoundPlayer(this);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkAccessManager *managerWS = new QNetworkAccessManager(this);


    QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> ca_list = sslCfg.caCertificates();
    QList<QSslCertificate> ca_new = QSslCertificate::fromData("CaCertificates");
    ca_list += ca_new;
    sslCfg.setCaCertificates(ca_list);
    sslCfg.setProtocol(QSsl::AnyProtocol);
    QSslConfiguration::setDefaultConfiguration(sslCfg);


    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorHandler(QNetworkReply*,QList<QSslError>)));
    connect(managerWS, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorHandler(QNetworkReply*,QList<QSslError>)));


    qApp->setProperty("Account", QVariant::fromValue<Account*>(account));
    qApp->setProperty("User_Settings", QVariant::fromValue<Settings*>(settings));
    qApp->setProperty("SoundPlayer", QVariant::fromValue<SoundPlayer*>(soundPlayer));
    qApp->setProperty("NetworkManager", QVariant::fromValue<QNetworkAccessManager*>(manager));
    qApp->setProperty("NetworkManagerWS", QVariant::fromValue<QNetworkAccessManager*>(managerWS));


}

//----------------------------------------------------------------------------------
void GlobalVars::sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist) {

    Q_UNUSED(errlist);

    // show list of all ssl errors
    // foreach (QSslError err, errlist)
    //   qDebug() << "Max ssl error: " << err;

    qnr->ignoreSslErrors();
}
