#include "btle_scanner_dialog_wasm.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

/*
 * On WebAssembly, Bluetooth device selection is driven by
 * navigator.bluetooth.requestDevice(), which shows a native browser picker.
 * This dialog simply informs the user and provides a "Select Device" button
 * that accepts the dialog.  BtleHubWasm::connectToDevice() then triggers the
 * actual Web Bluetooth scan via WebBluetoothBridge::scanForDevices().
 */
BtleScannerDialog::BtleScannerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect Bluetooth Device"));
    setMinimumWidth(380);

    auto *layout = new QVBoxLayout(this);

    auto *infoLabel = new QLabel(
        tr("<b>Bluetooth Device Selection</b><br><br>"
           "Your browser will display a device picker when you click "
           "<i>Select Device</i>.<br><br>"
           "Make sure Bluetooth is enabled and your device is powered on "
           "before continuing."),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(infoLabel);
    layout->addSpacing(12);

    auto *buttons = new QHBoxLayout;
    auto *cancelBtn  = new QPushButton(tr("Cancel"), this);
    auto *connectBtn = new QPushButton(tr("Select Device"), this);
    connectBtn->setDefault(true);
    buttons->addStretch();
    buttons->addWidget(cancelBtn);
    buttons->addWidget(connectBtn);
    layout->addLayout(buttons);

    connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
    connect(connectBtn, &QPushButton::clicked, this, &QDialog::accept);
}
