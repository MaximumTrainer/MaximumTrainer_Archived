#ifndef DIALOG_CONNECTION_METHOD_H
#define DIALOG_CONNECTION_METHOD_H

#include <QDialog>

/*
 * DialogConnectionMethod
 *
 * Presents the user with a choice of connection method before each workout:
 *   - "BTLE Device"   : scan for and connect to a real Bluetooth LE sensor
 *   - "Simulation"    : use SimulatorHub to generate synthetic sensor data
 */
class QPushButton;

class DialogConnectionMethod : public QDialog
{
    Q_OBJECT

public:
    enum ConnectionMethod { BTLE, Simulation };

    explicit DialogConnectionMethod(QWidget *parent = nullptr);

    ConnectionMethod selectedMethod() const { return m_method; }

private:
    ConnectionMethod m_method = BTLE;
};

#endif // DIALOG_CONNECTION_METHOD_H
