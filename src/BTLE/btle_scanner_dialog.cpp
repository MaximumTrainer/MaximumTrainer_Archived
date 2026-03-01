#include "btle_scanner_dialog.h"
#include "ui_btle_scanner_dialog.h"

#include <QDebug>
#include <QMessageBox>

// ─────────────────────────────────────────────────────────────────────────────
BtleScannerDialog::BtleScannerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BtleScannerDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Select Bluetooth Device"));

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({ tr("Device Name"), tr("Address") });
    ui->tableView_devices->setModel(m_model);
    ui->tableView_devices->horizontalHeader()->setStretchLastSection(true);
    ui->tableView_devices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_devices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_devices->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->pushButton_connect->setEnabled(false);

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(SCAN_TIMEOUT_MS);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BtleScannerDialog::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BtleScannerDialog::onScanFinished);
    connect(m_discoveryAgent,
            static_cast<void(QBluetoothDeviceDiscoveryAgent::*)
                (QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error),
            this, &BtleScannerDialog::onScanError);

    connect(ui->pushButton_scan,    &QPushButton::clicked,
            this, &BtleScannerDialog::startScan);
    connect(ui->pushButton_stop,    &QPushButton::clicked,
            this, &BtleScannerDialog::stopScan);
    connect(ui->pushButton_connect, &QPushButton::clicked,
            this, &BtleScannerDialog::onConnectClicked);
    connect(ui->tableView_devices->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this, &BtleScannerDialog::onSelectionChanged);

    startScan();
}

BtleScannerDialog::~BtleScannerDialog()
{
    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
void BtleScannerDialog::startScan()
{
    m_model->removeRows(0, m_model->rowCount());
    m_devices.clear();
    ui->label_status->setText(tr("Scanning..."));
    ui->pushButton_scan->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);

    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BtleScannerDialog::stopScan()
{
    m_discoveryAgent->stop();
}

void BtleScannerDialog::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    // Only show BLE devices
    if (!(device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;
    addOrUpdateDevice(device);
}

void BtleScannerDialog::addOrUpdateDevice(const QBluetoothDeviceInfo &device)
{
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].address() == device.address()) {
            m_devices[i] = device;
            QString name = device.name().isEmpty() ? tr("(unknown)") : device.name();
            m_model->item(i, 0)->setText(name);
            return;
        }
    }

    m_devices.append(device);
    QString name = device.name().isEmpty() ? tr("(unknown)") : device.name();
    QList<QStandardItem*> row;
    row << new QStandardItem(name)
        << new QStandardItem(device.address().toString());
    m_model->appendRow(row);
}

void BtleScannerDialog::onScanFinished()
{
    ui->label_status->setText(tr("Scan complete. %1 device(s) found.").arg(m_devices.size()));
    ui->pushButton_scan->setEnabled(true);
    ui->pushButton_stop->setEnabled(false);
}

void BtleScannerDialog::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error)
    ui->label_status->setText(tr("Scan error: %1").arg(m_discoveryAgent->errorString()));
    ui->pushButton_scan->setEnabled(true);
    ui->pushButton_stop->setEnabled(false);
}

void BtleScannerDialog::onSelectionChanged()
{
    bool hasRow = ui->tableView_devices->selectionModel()->hasSelection();
    ui->pushButton_connect->setEnabled(hasRow);
}

void BtleScannerDialog::onConnectClicked()
{
    QModelIndexList selected = ui->tableView_devices->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;

    int row = selected.first().row();
    if (row < 0 || row >= m_devices.size())
        return;

    m_selectedDevice = m_devices.at(row);

    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();

    accept();
}
