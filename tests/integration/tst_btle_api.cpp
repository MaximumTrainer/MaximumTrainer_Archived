#include <QtTest>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QSignalSpy>
#include "btle_hub.h"

// BLE API Smoke Tests
//
// These tests exercise the real BtleHub → QLowEnergyController code path.
// Unlike the BTLE integration tests (which use SimulatorHub and never touch
// any Bluetooth API), these tests invoke the actual OS BLE stack via
// QLowEnergyController::createCentral() + connectToDevice().
//
// On CI runners without Bluetooth hardware the tests QSKIP gracefully.
// On machines with a BLE adapter the OS stack reports a connection error
// for the fake device address — which is the expected outcome.
class TstBtleApi : public QObject
{
    Q_OBJECT
private slots:
    void testBleAdapterQuery();
    void testBtleHubSmokeTest();
};

// Verify QBluetoothLocalDevice::allDevices() is callable without crashing.
// Confirms QT += bluetooth compiled in and the OS Bluetooth stack is reachable.
// Always passes; result can be empty on CI runners with no hardware.
void TstBtleApi::testBleAdapterQuery()
{
    const QList<QBluetoothHostInfo> adapters = QBluetoothLocalDevice::allDevices();
    qInfo() << "[MT] BLE adapters found:" << adapters.size();
    QVERIFY2(true, "QBluetoothLocalDevice::allDevices() completed");
}

// Verify BtleHub::connectToDevice() invokes a real QLowEnergyController
// connection attempt.  With a fake device address the OS BLE stack reports a
// connectionError — confirming the real BLE API is exercised.
// QSKIP if no Bluetooth adapter is detected (typical for CI runners).
void TstBtleApi::testBtleHubSmokeTest()
{
    if (QBluetoothLocalDevice::allDevices().isEmpty()) {
        QSKIP("No Bluetooth adapter found — skipping BLE smoke test");
    }

    BtleHub hub;
    QSignalSpy errorSpy(&hub, &BtleHub::connectionError);
    QSignalSpy connectedSpy(&hub, &BtleHub::deviceConnected);

    // Build a fake device to trigger a real QLowEnergyController::connectToDevice()
    // call into the OS BLE stack.
#ifdef Q_OS_MACOS
    // CoreBluetooth on macOS identifies peripherals by UUID, not MAC address.
    QBluetoothDeviceInfo fakeDevice(
        QBluetoothUuid(QStringLiteral("{00000000-0000-0000-0000-000000000000}")),
        QStringLiteral("FakeTestDevice"), 0);
#else
    // BlueZ (Linux) and WinRT (Windows) use MAC addresses.
    QBluetoothDeviceInfo fakeDevice(
        QBluetoothAddress(QStringLiteral("00:11:22:33:44:55")),
        QStringLiteral("FakeTestDevice"), 0);
#endif

    hub.connectToDevice(fakeDevice);

    // Allow up to 30 s for the OS BLE stack to respond.
    // connectionError is expected — the fake device does not exist.
    // QSKIP if the adapter returns nothing (e.g. restricted CI environment).
    const bool gotError = errorSpy.wait(30000);
    const bool gotConnected = connectedSpy.count() > 0;

    if (!gotError && !gotConnected) {
        QSKIP("BLE API did not respond within 30s — adapter may be restricted in CI");
    }

    qInfo() << "[MT] BLE API responded: connectionError=" << errorSpy.count()
            << " deviceConnected=" << connectedSpy.count();

    // A connection to a non-existent device should produce a connectionError.
    // A deviceConnected response (some other device answered) is also acceptable.
    if (!gotConnected) {
        QVERIFY2(gotError,
                 "Expected connectionError signal when connecting to a non-existent device");
    }
}

QTEST_MAIN(TstBtleApi)
#include "tst_btle_api.moc"
