#ifndef ENVIRONNEMENT_H
#define ENVIRONNEMENT_H

#include <QtCore>


//// TO CHANGE DEV TO PROD
const static QString current_env = "prod";
//const static QString current_env = "dev";
const static QString current_version = "3.05";
const static QString date_released = "16/03/2019";  //(dd/mm/yyyy)



const static QString dev = "http://localhost/index.php/";
const static QString prod = "https://maximumtrainer.com/";
const static QString indexPage = "index.php/";



/// GoogleMap
const static QString urlGoogleMapEn = "google-map";


const static QString urlStravaAuthorize("https://www.strava.com/oauth/authorize?"
                                        "client_id=7252"
                                        "&response_type=code"
                                        "&scope=write"
                                        "&state=mystate"
                                        "&approval_prompt=force");



///TrainingPeaks - These are the API URLs that you will use for development:
//https://oauth.trainingpeaks.com/oauth/token
//https://oauth.trainingpeaks.com/oauth/authorize
//https://api.trainingpeaks.com/v1/file

const static QString urlTrainingPeaksAuthorize("https://oauth.trainingpeaks.com/oauth/authorize?"
                                               "client_id=maximumtrainer"
                                               "&response_type=code"
                                               "&scope=file:write");

const static QString CLIENT_ID_TP = "maximumtrainer";
const static QString CLIENT_SECRET_TP = "tXd2eDxHe73taHkVB4oHkMkSPw6aZ3nO5n2R0bxc";
const static QString URL_TOKEN_TP = "https://oauth.trainingpeaks.com/oauth/token/";
const static QString URL_POST_FILE_TP = "https://api.trainingpeaks.com/v1/file/";



/// Login
const static QString urlLoginEn = "login/insideMT";
const static QString urlLoginFr = "connexion/insideMT";


/// Profile
const static QString urlProfilEn = "myprofile/insideMT";
const static QString urlProfilFr = "monprofil/insideMT";
const static QString urlProfilFrOutsideMt = "monprofil";
const static QString urlProfilEnOutsideMt = "myprofile";


/// Choose-Subscription
const static QString urlChooseSubEn = "choose-subscription";
const static QString urlChooseSubFr = "choisir-abonnement";



/// News
const static QString urlNewsEn = "news";
const static QString urlNewsFr = "nouvelles";


/// Zones
const static QString urlZonesEn = "training-zones";
const static QString urlZonesFr = "zones-entrainement";


/// Studio
const static QString urlStudioEn = "studio";
const static QString urlStudioFr = "studio-fr";


/// Achievement
const static QString urlAchievEn = "achievement/insideMT";
const static QString urlAchievFr = "accomplissement/insideMT";


/// Settings
const static QString urlSettingsEn = "settings";
const static QString urlSettingsFr = "configuration";


/// Workout
const static QString urlWorkoutEn = "workouts";
const static QString urlWorkoutFr = "entrainements";

/// Workout-creator
const static QString urlWorkoutCreatorEn = "workout-creator";
const static QString urlWorkoutCreatorFr = "workout-createur";

/// Course-creator
const static QString urlCourseCreatorEn = "course-creator";
const static QString urlCourseCreatorFr = "parcours-createur";


/// Training-Plans
const static QString urlPlanEn = "training-plans/insideMT";
const static QString urlPlanFr = "plans-entrainement/insideMT";





/// Help #AntStick
const static QString urlSupportEn = "support/insideMT";  //support#ant-stick
const static QString urlSupportFr = "aide/insideMT";
const static QString urlSupportAntEn = "support/insideMT/#ant-stick-not-found";  //support#ant-stick
const static QString urlSupportAntFr = "aide/insideMT/#cle-ant-non-trouvee";

/// Download
const static QString urlDownloadEn = "download-mt";
const static QString urlDownloadFr = "telecharger-mt";





class Environnement
{
public:
    Environnement();



    /// Public method -----------------------
    static QString getURLEnvironnement();
    static QString getURLEnvironnementWS();
    static QString getVersion();
    static QString getDateBuilded();


    static QString getURLGoogleMap();
    static QString getURLStravaAuthorize();
    static QString getURLTrainingPeaksAuthorize();


    static QString getUrlLogin();
    static QString getUrlProfileOutsideMt();
    static QString getUrlChooseSub();
    static QString getUrlNews();

    static QString getUrlZones();
    static QString getUrlStudio();
    static QString getUrlAchievement();
    static QString getUrlSettings();

    static QString getUrlWorkout();
    static QString getUrlWorkoutCreator();
    static QString getUrlCourseCreator();
    static QString getUrlDownload();
    static QString getUrlPlans();

    static QString getUrlSupport();
    static QString getUrlSupportAntStick();

};

#endif // ENVIRONNEMENT_H
