#ifndef SIMULATOR_HUB_H
#define SIMULATOR_HUB_H

#include <QObject>
#include <QTimer>

/*
 * SimulatorHub
 *
 * Emits the same sensor-data signals as BtleHub at regular intervals using
 * generated values, allowing the full workout data pipeline to be exercised
 * without any physical Bluetooth hardware.
 *
 * Values drift gradually within realistic ranges:
 *   HR      : ~140 bpm  (±15)
 *   Cadence : ~90 rpm   (±10)
 *   Speed   : ~28 km/h  (±5)
 *   Power   : ~200 W    (±30)
 */
class SimulatorHub : public QObject
{
    Q_OBJECT

public:
    explicit SimulatorHub(QObject *parent = nullptr);
    ~SimulatorHub() override = default;

    void start();
    void stop();

signals:
    // Same signatures as BtleHub / Hub so MainWindow can wire them identically
    void signal_hr(int userID, int hr);
    void signal_cadence(int userID, int cadence);
    void signal_speed(int userID, double speed);   // km/h
    void signal_power(int userID, int power);      // watts

public slots:
    // No-op stubs matching BtleHub/Hub slot signatures
    void setLoad(int antID, double watts);
    void setSlope(int antID, double grade);
    void stopDecodingMsg();

private slots:
    void tick();

private:
    QTimer *m_timer = nullptr;

    // Current simulated values
    double m_hr      = 140.0;
    double m_cadence =  90.0;
    double m_speed   =  28.0;
    double m_power   = 200.0;

    // Drift direction per channel (+1 / -1)
    int m_hrDir      = 1;
    int m_cadenceDir = 1;
    int m_speedDir   = 1;
    int m_powerDir   = 1;
};

#endif // SIMULATOR_HUB_H
