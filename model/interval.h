#ifndef INTERVAL_H
#define INTERVAL_H

#include <QString>
#include <QDataStream>
#include <QTime>
#include <memory>
#include <QList>
#include <QUuid>


class Interval
{

public:

    enum StepType
    {
        NONE,
        FLAT,
        PROGRESSIVE,
    };

    ~Interval() {}
    Interval();
    Interval(QTime duration, QString displayMessage,
             StepType powerStepType, double targetFTP_start, double targetFTP_end, int targetFTP_range, double rightPowerTarget,
             StepType cadenceStepType, int targetCadence_start, int targetCadence_end, int cadence_range,
             StepType hrStepType, double targetHR_start, double targetHR_end, int HR_range,
             bool testInterval, double repeatInscreaseFTP, int repeatIncreaseCadence, double repeatIncreaseLTHR);

    //    bool operator==(const Interval &other) const;
    friend QDataStream& operator<<(QDataStream& out, const Interval& interval);
    friend QDataStream& operator>>(QDataStream& out, Interval& interval);


    /// -------------------- setters -----------------------
    void setTime(QTime time) {
        this->duration = time;
    }
    void setDisplayMsg(const QString &displayMsg) {
        displayMessage = displayMsg;
    }
    /// Power
    void setPowerStepType(StepType powerStepType) {
        this->powerStepType = powerStepType;
    }
    void setTargetFTP_start(double targetFTP_start) {
        this->targetFTP_start = targetFTP_start;
    }
    void setTargetFTP_end(double targetFTP_end) {
        this->targetFTP_end = targetFTP_end;
    }
    void setTargetFTP_range(int targetFTP_range) {
        this->targetFTP_range = targetFTP_range;
    }
    void setRightPowerTarget(double rightPowerTarget) {
        this->rightPowerTarget = rightPowerTarget;
    }


    /// Cadence
    void setCadenceStepType(StepType cadenceStepType) {
        this->cadenceStepType = cadenceStepType;
    }
    void setTargetCadence_start(int targetCadence_start) {
        this->targetCadence_start = targetCadence_start;
    }
    void setTargetCadence_end(int targetCadence_end) {
        this->targetCadence_end = targetCadence_end;
    }
    void setTargetCadence_range(int targetCadence_range) {
        this->targetCadence_range = targetCadence_range;
    }
    /// Heartrate
    void setHrStepType(StepType hrStepType) {
        this->hrStepType = hrStepType;
    }
    void setTargetHR_start(double targetHR_start) {
        this->targetHR_start = targetHR_start;
    }
    void setTargetHR_end(double targetHR_end) {
        this->targetHR_end = targetHR_end;
    }
    void setTargetHR_range(int targetHR_range) {
        this->targetHR_range = targetHR_range;
    }


    void setSourceRowLstInterval(int sourceRowLstInterval) {
        this->sourceRowLstInterval = sourceRowLstInterval;
    }
    void setTestInterval(bool val) {
        this->testInterval = val;
    }
    void setRepeatIncreaseFTP(double repeatIncreaseFTP) {
        this->repeatIncreaseFTP = repeatIncreaseFTP;
    }
    void setRepeatIncreaseCadence(double repeatIncreaseCadence) {
        this->repeatIncreaseCadence = repeatIncreaseCadence;
    }
    void setRepeatIncreaseLTHR(double repeatIncreaseLTHR) {
        this->repeatIncreaseLTHR = repeatIncreaseLTHR;
    }



    /////////////////////////////////////////////////////////////////////////////////////////
    QTime getDurationQTime() const;
    static QString getStepTypeToString(StepType type);
    QString getDisplayMessage() const;

    StepType getPowerStepType() const;
    int getFTP_range() const;
    double getFTP_start() const;
    double getFTP_end() const;
    double getRightPowerTarget() const;


    StepType getCadenceStepType() const;
    int getCadence_range() const;
    int getCadence_start() const;
    int getCadence_end() const;

    StepType getHRStepType() const;
    int getHR_range() const;
    double getHR_start() const;
    double getHR_end() const;

    bool isTestInterval() const;
    double getRepeatIncreaseFTP() const;
    int getRepeatIncreaseCadence() const;
    double getRepeatIncreaseLTHR() const;


    int getSourceRowLstInterval() const {
        return this->sourceRowLstInterval;
    }



private:

    int sourceRowLstInterval;
    QTime duration;
    QString displayMessage;

    StepType powerStepType;
    double targetFTP_start;
    double targetFTP_end;
    int targetFTP_range;
    double rightPowerTarget;

    StepType cadenceStepType;
    int targetCadence_start;
    int targetCadence_end;
    int targetCadence_range;

    StepType hrStepType;
    double targetHR_start;
    double targetHR_end;
    int targetHR_range;

    bool testInterval;

    double repeatIncreaseFTP;
    int repeatIncreaseCadence;
    double repeatIncreaseLTHR;




};
Q_DECLARE_METATYPE(Interval)







#endif // INTERVAL_H
