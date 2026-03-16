#include "importerworkoutzwo.h"
#include "interval.h"
#include "util.h"

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDebug>

ImporterWorkoutZwo::ImporterWorkoutZwo(QObject *parent) : QObject(parent) {}
ImporterWorkoutZwo::~ImporterWorkoutZwo() {}

//-------------------------------------------------------------------------------------------
Workout ImporterWorkoutZwo::importFromFile(QString filename)
{
    qDebug() << "ImporterWorkoutZwo: parsing" << filename;

    QString name;
    QString author;
    QString description;
    QList<Interval> lstInterval;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ImporterWorkoutZwo: cannot open" << filename;
        return Workout();
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString elem = xml.name().toString();
            if (elem.compare("name", Qt::CaseInsensitive) == 0) {
                name = xml.readElementText().trimmed();
            } else if (elem.compare("author", Qt::CaseInsensitive) == 0) {
                author = xml.readElementText().trimmed();
            } else if (elem.compare("description", Qt::CaseInsensitive) == 0) {
                description = xml.readElementText().trimmed();
            } else if (elem.compare("workout", Qt::CaseInsensitive) == 0) {
                lstInterval = parseWorkoutElement(xml);
            }
        }
    }
    if (xml.hasError())
        qWarning() << "ImporterWorkoutZwo: XML error:" << xml.errorString();
    file.close();

    if (name.isEmpty())
        name = QFileInfo(filename).completeBaseName();
    if (author.isEmpty())
        author = "-";
    if (description.isEmpty())
        description = "Imported from .zwo";

    Workout workout("", Workout::USER_MADE, lstInterval,
                    name, author, description, "-", Workout::T_ENDURANCE);
    return workout;
}

//-------------------------------------------------------------------------------------------
QList<Interval> ImporterWorkoutZwo::parseWorkoutElement(QXmlStreamReader &xml)
{
    QList<Interval> lst;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::EndElement &&
                xml.name().toString().compare("workout", Qt::CaseInsensitive) == 0)
            break;

        if (token != QXmlStreamReader::StartElement)
            continue;

        QString elem = xml.name().toString();
        QXmlStreamAttributes attrs = xml.attributes();

        auto attrDouble = [&](const QString &key) -> double {
            return attrs.value(key).toString().toDouble();
        };
        auto attrInt = [&](const QString &key) -> int {
            return attrs.value(key).toString().toInt();
        };

        if (elem.compare("SteadyState", Qt::CaseInsensitive) == 0) {
            int durSecs = attrInt("Duration");
            double power = attrDouble("Power");
            if (durSecs > 0) {
                QTime dur = Util::convertMinutesToQTime(durSecs / 60.0);
                // power range=20, rightPowerTarget=-1 (unused), cadence range=5, HR range=15 — same defaults as mrc/erg importer
                Interval iv(dur, "",
                             Interval::FLAT, power, power, 20, -1,
                             Interval::NONE, 0, 0, 5,
                             Interval::NONE, 0, 0, 15,
                             false, 0, 0, 0);
                lst.append(iv);
            }
        } else if (elem.compare("Warmup", Qt::CaseInsensitive) == 0 ||
                   elem.compare("Cooldown", Qt::CaseInsensitive) == 0 ||
                   elem.compare("Ramp", Qt::CaseInsensitive) == 0) {
            int durSecs = attrInt("Duration");
            double powerLow  = attrDouble("PowerLow");
            double powerHigh = attrDouble("PowerHigh");
            if (durSecs > 0) {
                QTime dur = Util::convertMinutesToQTime(durSecs / 60.0);
                Interval iv(dur, "",
                             Interval::PROGRESSIVE, powerLow, powerHigh, 20, -1,
                             Interval::NONE, 0, 0, 5,
                             Interval::NONE, 0, 0, 15,
                             false, 0, 0, 0);
                lst.append(iv);
            }
        } else if (elem.compare("IntervalsT", Qt::CaseInsensitive) == 0) {
            int repeat     = attrInt("Repeat");
            int onDurSecs  = attrInt("OnDuration");
            int offDurSecs = attrInt("OffDuration");
            double onPower  = attrDouble("OnPower");
            double offPower = attrDouble("OffPower");
            if (repeat > 0 && (onDurSecs > 0 || offDurSecs > 0)) {
                for (int i = 0; i < repeat; ++i) {
                    if (onDurSecs > 0) {
                        QTime onDur = Util::convertMinutesToQTime(onDurSecs / 60.0);
                        Interval on(onDur, "",
                                    Interval::FLAT, onPower, onPower, 20, -1,
                                    Interval::NONE, 0, 0, 5,
                                    Interval::NONE, 0, 0, 15,
                                    false, 0, 0, 0);
                        lst.append(on);
                    }
                    if (offDurSecs > 0) {
                        QTime offDur = Util::convertMinutesToQTime(offDurSecs / 60.0);
                        Interval off(offDur, "",
                                     Interval::FLAT, offPower, offPower, 20, -1,
                                     Interval::NONE, 0, 0, 5,
                                     Interval::NONE, 0, 0, 15,
                                     false, 0, 0, 0);
                        lst.append(off);
                    }
                }
            }
        } else if (elem.compare("FreeRide", Qt::CaseInsensitive) == 0) {
            int durSecs = attrInt("Duration");
            if (durSecs > 0) {
                QTime dur = Util::convertMinutesToQTime(durSecs / 60.0);
                // FreeRide has no power target; use 50% FTP as a neutral placeholder
                Interval iv(dur, "",
                             Interval::FLAT, 0.5, 0.5, 20, -1,
                             Interval::NONE, 0, 0, 5,
                             Interval::NONE, 0, 0, 15,
                             false, 0, 0, 0);
                lst.append(iv);
            }
        }
        // Other element types (e.g. MaxEffort) are skipped
    }

    return lst;
}
