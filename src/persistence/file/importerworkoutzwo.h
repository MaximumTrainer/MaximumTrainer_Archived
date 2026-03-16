#ifndef IMPORTERWORKOUTZWO_H
#define IMPORTERWORKOUTZWO_H

#include <QByteArray>
#include <QString>

#include "workout.h"

/// Parser for Zwift workout (ZWO) files.
///
/// ZWO is a simple XML format used by Zwift and Intervals.icu.
/// The root element is <workout_file> and contains optional metadata
/// elements (<name>, <author>, <description>) followed by a <workout>
/// element whose children each describe one or more training intervals:
///
///   <Warmup     Duration="s"  PowerLow="ftp"  PowerHigh="ftp" />
///   <SteadyState Duration="s" Power="ftp" />
///   <Ramp       Duration="s"  PowerLow="ftp"  PowerHigh="ftp" />
///   <Cooldown   Duration="s"  PowerLow="ftp"  PowerHigh="ftp" />
///   <IntervalsT Repeat="n"    OnDuration="s"  OffDuration="s"
///               OnPower="ftp" OffPower="ftp" />
///   <FreeRide   Duration="s" />
///
/// Power values are fractions of FTP (1.0 = 100 % FTP).
class ImporterWorkoutZwo
{
public:
    /// Parse a ZWO XML file from a byte array and return a Workout object.
    ///
    /// @param data         The raw bytes of the ZWO file.
    /// @param workoutName  Fallback name used when the ZWO file contains no
    ///                     <name> element. If empty, "Intervals.icu Workout"
    ///                     is used.
    /// @return             A Workout populated with the parsed intervals.
    ///                     Returns an empty Workout if parsing fails.
    static Workout importFromByteArray(const QByteArray &data,
                                       const QString &workoutName = QString());
};

#endif // IMPORTERWORKOUTZWO_H
