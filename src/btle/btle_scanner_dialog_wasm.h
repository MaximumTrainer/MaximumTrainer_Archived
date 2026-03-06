#ifndef BTLE_SCANNER_DIALOG_WASM_H
#define BTLE_SCANNER_DIALOG_WASM_H

#include <QDialog>

/*
 * BtleScannerDialog (WebAssembly version)
 *
 * On WASM, Bluetooth device selection is handled entirely by the browser's
 * native navigator.bluetooth.requestDevice() dialog via WebBluetoothBridge.
 * This class presents a simple prompt; the actual device picker is owned by
 * the browser, so no QBluetooth types are needed.
 *
 * The interface is compatible with the native BtleScannerDialog:
 *   - hasSelection() always returns true when the dialog is accepted
 *   - selectedDevice() returns 0 (int); BtleHubWasm::connectToDevice(const T&)
 *     is a template that ignores this argument and calls scanForDevice() instead
 */
class BtleScannerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BtleScannerDialog(QWidget *parent = nullptr);
    ~BtleScannerDialog() = default;

    bool hasSelection() const { return result() == QDialog::Accepted; }

    // Returns a dummy value; BtleHubWasm::connectToDevice ignores it.
    int selectedDevice() const { return 0; }
};

#endif // BTLE_SCANNER_DIALOG_WASM_H
