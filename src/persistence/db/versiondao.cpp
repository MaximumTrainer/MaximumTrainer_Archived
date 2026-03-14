#include "versiondao.h"

#include "environnement.h"



// Fetches the latest release metadata from the GitHub Releases API.
// The JSON response contains a "tag_name" field (e.g. "v0.0.26") which is
// compared against the running app's APP_VERSION to detect available updates.
QNetworkReply* VersionDAO::getVersion() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    qDebug() << "Get Version (GitHub Releases API):" << urlGitHubReleasesApi;

    QNetworkRequest request;
    request.setUrl(QUrl(urlGitHubReleasesApi));
    request.setRawHeader("User-Agent", "MaximumTrainer");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");

    return managerWS->get(request);
}

