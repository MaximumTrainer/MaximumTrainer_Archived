#ifndef BTLE_SCANNER_DIALOG_H
#define BTLE_SCANNER_DIALOG_H

#include <QDialog>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QStandardItemModel>

namespace Ui {
class BtleScannerDialog;
}

/*
 * BtleScannerDialog
 *
 * Presents a list of nearby BLE devices.  The user selects one and presses
 * "Connect".  The selected QBluetoothDeviceInfo is then available via
 * selectedDevice().
 */
class BtleScannerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BtleScannerDialog(QWidget *parent = nullptr);
    ~BtleScannerDialog();

    QBluetoothDeviceInfo selectedDevice() const { return m_selectedDevice; }
    bool hasSelection() const { return m_selectedDevice.isValid(); }

private slots:
    void startScan();
    void stopScan();
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onSelectionChanged();
    void onConnectClicked();

private:
    void addOrUpdateDevice(const QBluetoothDeviceInfo &device);

    Ui::BtleScannerDialog *ui;
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QStandardItemModel *m_model = nullptr;
    QList<QBluetoothDeviceInfo> m_devices;
    QBluetoothDeviceInfo m_selectedDevice;

    static constexpr int SCAN_TIMEOUT_MS = 10000; // 10 s BLE scan
};

#endif // BTLE_SCANNER_DIALOG_H
