#ifndef TAB_INTERVALS_ICU_H
#define TAB_INTERVALS_ICU_H

#include <QWidget>
#include <QDate>
#include <QList>
#include <QNetworkReply>

#include "intervalsicuservice.h"

namespace Ui {
class TabIntervalsIcu;
}

///
/// Native Qt widget that replaces the former TrainerDay WebView tab.
///
/// Shows the user's Intervals.icu calendar for the selected week and allows
/// downloading a planned workout as a .zwo file directly into the local
/// workout folder.
///
/// In WASM builds (GC_WASM_BUILD) network access is disabled and an
/// "offline mode" banner is shown instead.
///
class TabIntervalsIcu : public QWidget
{
    Q_OBJECT

public:
    explicit TabIntervalsIcu(QWidget *parent = nullptr);
    ~TabIntervalsIcu();

    /// Call this once the Account credentials are available (or after they
    /// change in the preferences dialog).
    void refreshCredentials();

signals:
    /// Emitted after a .zwo file has been written to disk.
    /// @param workoutName  base name (without extension) of the saved file.
    void workoutDownloaded(const QString &workoutName);

private slots:
    void onRefreshClicked();
    void onPrevWeekClicked();
    void onNextWeekClicked();
    void onLoadWorkoutClicked();
    void onCalendarFetchFinished();
    void onWorkoutDownloadFinished();

private:
    void updateWeekLabel();
    void populateTable(const QList<IntervalsIcuService::CalendarEvent> &events);
    void setStatus(const QString &msg);
    void setBusy(bool busy);

    Ui::TabIntervalsIcu *ui;

    IntervalsIcuService *m_service;
    QDate m_weekStart;   ///< Monday of the currently displayed week

    QNetworkReply *m_calendarReply  = nullptr;
    QNetworkReply *m_downloadReply  = nullptr;

    // Parallel list of workout IDs for each table row (empty string = no
    // downloadable workout for that event)
    QList<QString> m_rowWorkoutIds;
};

#endif // TAB_INTERVALS_ICU_H
