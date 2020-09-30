#include "settings.h"

#include <QDebug>
#include <QSettings>



Settings::~Settings() {
    qDebug() << "destructor Settings;";
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constructor - load settings
Settings::Settings(QObject *parent) : QObject(parent)  {



    QSettings settings;



    settings.beginGroup("SettingsGeneral");
    rememberMyPassword = settings.value("rememberMyPassword", false).toBool();

    for (int i=0; i<5; i++) {
        QString username = settings.value("username"+i, "").toString();
        if (username != "")
            lstUsername.append(settings.value("username"+i, "").toString());
    }
    lastLoggedUsername = settings.value("lastLoggedUsername", "").toString();
    lastLoggedKey = settings.value("lastLoggedKey", "").toString();
    settings.endGroup();



    settings.beginGroup("SettingsFolders");
    historyFolder = settings.value("historyFolder", "").toString();
    workoutFolder = settings.value("workoutFolder", "").toString();
    courseFolder = settings.value("courseFolder", "").toString();
    settings.endGroup();




    /// Language settings
    settings.beginGroup("language_app");
    language_index = settings.value("lang", 0).toInt();
    settings.endGroup();

    if (language_index == 0)
        language = "en";
    else if (language_index == 1)
        language = "fr";
    else
        language = "en";



}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
void Settings::saveGeneralSettings() {

    QSettings settings;

    qDebug() << "saveGeneralSettings!";



    settings.beginGroup("SettingsGeneral");
    settings.setValue("rememberMyPassword", rememberMyPassword);

    int nbUsername = lstUsername.size();
    for (int i=0; i<nbUsername; i++) {
        settings.setValue("username"+i, lstUsername.at(i));
    }
    for (int i=nbUsername; i<5; i++) {
        settings.setValue("username"+i, "");
    }
    settings.setValue("lastLoggedUsername", lastLoggedUsername);
    settings.setValue("lastLoggedKey", lastLoggedKey);
    settings.endGroup();


    settings.beginGroup("SettingsFolders");
    settings.setValue("historyFolder", historyFolder );
    settings.setValue("workoutFolder", workoutFolder );
    settings.setValue("courseFolder", courseFolder );
    settings.endGroup();




}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
void Settings::saveLanguage() {

    QSettings settings;

    settings.beginGroup("language_app");
    settings.setValue("lang", language_index);
    settings.endGroup();
}



