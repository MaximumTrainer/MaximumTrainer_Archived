#include "versiondao.h"

#include "environnement.h"
#include "logger.h"

// Fetches the latest release metadata from the GitHub Releases API.
// The JSON response contains a "tag_name" field (e.g. "v0.0.26") which is
// compared against the running app's APP_VERSION to detect available updates.
QNetworkReply* VersionDAO::getVersion() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!managerWS) {
        LOG_WARN("VersionDAO", QStringLiteral("getVersion: NetworkManagerWS not available"));
        return nullptr;
    }

    LOG_INFO("VersionDAO", QStringLiteral("GET ") + urlGitHubReleasesApi);

    QNetworkRequest request;
    request.setUrl(QUrl(urlGitHubReleasesApi));
    request.setRawHeader("User-Agent", "MaximumTrainer");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    // Abort if no data is received within 10 seconds so the app never hangs.
    request.setTransferTimeout(10000);
#endif

    return managerWS->get(request);
}

