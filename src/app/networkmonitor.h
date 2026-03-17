#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QObject>
#include <QNetworkReply>

class QTimer;
class QNetworkAccessManager;

///
/// Application-wide network connectivity monitor.
///
/// Periodically probes https://intervals.icu with a lightweight HEAD request
/// and emits onlineChanged(bool) whenever the reachable / unreachable state
/// flips.  Components that depend on a live internet connection (TabIntervalsIcu,
/// the Cloud Sync preferences panel, post-workout auto-upload) connect to this
/// signal to hide or disable themselves while offline.
///
/// Usage:
///   NetworkMonitor::instance()->isOnline()          // synchronous query
///   connect(NetworkMonitor::instance(),              // reactive update
///           &NetworkMonitor::onlineChanged,
///           this, &MyClass::onOnlineChanged);
///
/// In WASM builds the monitor is always offline (no outbound HTTP).
///
class NetworkMonitor : public QObject
{
    Q_OBJECT

public:
    /// Returns the application-wide singleton.  Created lazily on first call.
    static NetworkMonitor *instance();

    /// true if the most-recent connectivity probe succeeded.
    /// Starts as false on all platforms until the first probe completes.
    bool isOnline() const { return m_isOnline; }

    /// Trigger an immediate (asynchronous) probe outside the normal interval.
    void checkNow();

signals:
    /// Emitted whenever the online / offline state changes.
    void onlineChanged(bool isOnline);

private slots:
#ifndef GC_WASM_BUILD
    void onCheckReplyFinished();
#endif

private:
    explicit NetworkMonitor(QObject *parent = nullptr);

    void setOnline(bool online);

    static NetworkMonitor *s_instance;

    bool m_isOnline;

#ifndef GC_WASM_BUILD
    QTimer               *m_timer      = nullptr;
    QNetworkAccessManager *m_manager   = nullptr;
    QNetworkReply        *m_checkReply = nullptr;
#endif
};

#endif // NETWORKMONITOR_H
