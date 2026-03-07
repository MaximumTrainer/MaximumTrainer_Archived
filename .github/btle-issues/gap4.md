## Context

Audit reference: [`btle-audit.md`](../../btle-audit.md) section 5.1 and section 6, Gap 4
Severity: **Low**

## Problem

New Linux users frequently encounter an empty BLE device list with no
actionable error message. The root cause is that their account is not a member
of the `bluetooth` group, or `bluetoothd` is not running.

The `README.md` contains no Linux Bluetooth setup instructions.

## Requirement

Add a **Linux Prerequisites** section to `README.md` (or the main installation
guide) covering:

```
## Linux — Bluetooth Setup

Before running MaximumTrainer on Linux, ensure:

1. BlueZ is installed and the daemon is running:
   sudo systemctl enable --now bluetooth

2. Your user account is in the bluetooth group:
   sudo usermod -aG bluetooth $USER
   # Re-login or run: newgrp bluetooth

3. Your Bluetooth adapter supports BLE (Bluetooth 4.0+).
   Check with: hciconfig -a

4. Required kernel modules are loaded:
   bluetooth, hci_uart (or hci_usb)
```

## Acceptance Criteria

- [ ] `README.md` contains the Linux Bluetooth prerequisite section.
- [ ] Instructions include the `usermod` command and the re-login note.
