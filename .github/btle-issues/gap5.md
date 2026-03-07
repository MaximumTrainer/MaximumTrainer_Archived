## Context

Audit reference: [`btle-audit.md`](../../btle-audit.md) section 5.2 and section 6, Gap 5
Severity: **Low**

## Problem

Qt Bluetooth on Windows uses the WinRT `Windows.Devices.Bluetooth` APIs which
are only available from **Windows 10 version 1703 (Creators Update)** onwards.
On Windows 7, 8, and 8.1 the Qt Bluetooth backend silently fails:
`QBluetoothDeviceDiscoveryAgent` returns an empty device list with no error.

The `README.md` does not state the minimum Windows version requirement.

## Requirement

Add a **Windows Prerequisites** section to `README.md`:

```
## Windows — Requirements

- Windows 10 version 1703 (Creators Update) or later is required.
  Windows 7, 8, and 8.1 are not supported (missing WinRT BLE APIs).
- A Bluetooth 4.0+ adapter with a WDM-compatible driver.
- No special permissions or manifest entries are needed.
```

## Acceptance Criteria

- [ ] `README.md` states the Windows 10+ requirement clearly.
- [ ] The requirement is visible in the installation / getting-started section.
