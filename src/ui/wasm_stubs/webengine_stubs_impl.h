// WebAssembly stub implementations for Qt WebEngine classes.
// These are included via the wasm_stubs INCLUDEPATH when building for Wasm,
// replacing the unavailable Qt WebEngine module headers.
#pragma once

#include <QObject>
#include <QWidget>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QLabel>
#include <functional>

// ─────────────────────────────── Forward declarations ────────────────────────
class QWebEnginePage;
class QWebEngineProfile;
class QWebEngineScriptCollection;

// ─────────────────────────────── QWebEngineSettings ──────────────────────────
class QWebEngineSettings {
public:
    enum WebAttribute { PluginsEnabled = 0, JavascriptEnabled = 1 };
    void setAttribute(WebAttribute, bool) {}
    static QWebEngineSettings *defaultSettings() { static QWebEngineSettings s; return &s; }
};

// ─────────────────────────────── QWebEngineScript ────────────────────────────
class QWebEngineScript {
public:
    enum ScriptWorldId { MainWorld = 0 };
    enum InjectionPoint { DocumentCreation = 0, DocumentReady = 1, Deferred = 2 };
    enum SavePageFormat { UnknownSaveFormat = -1, SingleHtmlSaveFormat = 0, CompleteHtmlSaveFormat = 1, MimeHtmlSaveFormat = 2 };
    void setName(const QString &) {}
    void setSourceCode(const QString &) {}
    void setWorldId(ScriptWorldId) {}
    void setInjectionPoint(InjectionPoint) {}
    void setRunsOnSubFrames(bool) {}
};

// ─────────────────────── QWebEngineScriptCollection ──────────────────────────
class QWebEngineScriptCollection {
public:
    void insert(const QWebEngineScript &) {}
    void clear() {}
};

// ─────────────────────────────── QWebEngineProfile ───────────────────────────
class QWebEngineProfile : public QObject {
public:
    enum PersistentCookiesPolicy { NoPersistentCookies = 0 };
    explicit QWebEngineProfile(QObject *parent = nullptr) : QObject(parent) {}
    static QWebEngineProfile *defaultProfile() { static QWebEngineProfile p; return &p; }
    QWebEngineSettings *settings() { return QWebEngineSettings::defaultSettings(); }
    void setPersistentCookiesPolicy(PersistentCookiesPolicy) {}
};

// ─────────────────────────────── QWebEnginePage ──────────────────────────────
class QWebEnginePage : public QObject {
public:
    enum NavigationType { NavigationTypeLinkClicked = 0 };
    explicit QWebEnginePage(QObject *parent = nullptr)
        : QObject(parent), m_profile(new QWebEngineProfile(this)) {}
    explicit QWebEnginePage(QWebEngineProfile *profile, QObject *parent = nullptr)
        : QObject(parent), m_profile(profile ? profile : new QWebEngineProfile(this)) {}

    virtual bool acceptNavigationRequest(const QUrl &, NavigationType, bool) { return true; }

    void runJavaScript(const QString &) {}
    void setUrl(const QUrl &) {}
    QUrl url() const { return {}; }
    QWebEngineProfile *profile() const { return m_profile; }
    QWebEngineScriptCollection &scripts() { return m_scripts; }
    void setWebChannel(QObject *) {}
    void toPlainText(std::function<void(const QString &)>) {}

private:
    QWebEngineProfile *m_profile;
    QWebEngineScriptCollection m_scripts;
};

// ─────────────────────────────── QWebEngineView ──────────────────────────────
class QWebEngineView : public QWidget {
public:
    explicit QWebEngineView(QWidget *parent = nullptr)
        : QWidget(parent), m_page(new QWebEnginePage(this))
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *label = new QLabel(
            "<center><b>Web content</b><br>Not available in the browser version.<br>"
            "Open the desktop app to access this feature.</center>",
            this);
        label->setWordWrap(true);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label, 0, Qt::AlignCenter);
    }

    void load(const QUrl &) {}
    void setUrl(const QUrl &) {}
    QUrl url() const { return {}; }
    void setPage(QWebEnginePage *page) { if (page) m_page = page; }
    QWebEnginePage *page() const { return m_page; }
    void back() {}
    void forward() {}
    void reload() {}
    void stop() {}

private:
    QWebEnginePage *m_page;
};

// ─────────────────── QWebEngineDownloadRequest (Qt 6) ────────────────────────
class QWebEngineDownloadRequest : public QObject {
public:
    enum SavePageFormat { UnknownSaveFormat = -1, SingleHtmlSaveFormat = 0, CompleteHtmlSaveFormat = 1, MimeHtmlSaveFormat = 2 };
    explicit QWebEngineDownloadRequest(QObject *parent = nullptr) : QObject(parent) {}
    QUrl url() const { return {}; }
    void accept() {}
    void cancel() {}
    QString downloadFileName() const { return {}; }
    void setDownloadFileName(const QString &) {}
    QString downloadDirectory() const { return {}; }
    void setDownloadDirectory(const QString &) {}
    SavePageFormat savePageFormat() const { return UnknownSaveFormat; }
};

// ─────────────────────── QWebEngineDownloadItem (Qt 5) ───────────────────────
class QWebEngineDownloadItem : public QObject {
public:
    explicit QWebEngineDownloadItem(QObject *parent = nullptr) : QObject(parent) {}
    QUrl url() const { return {}; }
    void accept() {}
    void cancel() {}
    QString path() const { return {}; }
    void setPath(const QString &) {}
};

// ─────────────────────────────── QWebChannel ─────────────────────────────────
class QWebChannel : public QObject {
public:
    explicit QWebChannel(QObject *parent = nullptr) : QObject(parent) {}
    void registerObject(const QString &, QObject *) {}
    void deregisterObject(QObject *) {}
};
