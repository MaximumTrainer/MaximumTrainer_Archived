# BTLE Subsystem — State of the Union

> **Date:** March 2025  
> **Scope:** Technical audit of the Bluetooth Low Energy (BTLE) subsystem
> following the migration to Qt 6.  
> **Files audited:** `src/btle/` (15 files, ≈ 1 500 lines of C++/JavaScript)

---

## 1. Executive Summary

The BTLE subsystem is in a **good overall state**.  The native (desktop) path
is complete and well-tested.  The WebAssembly (WASM) path is functional but
had three correctness gaps that have been fixed in this audit:

| Item | Severity | Status |
|------|----------|--------|
| `BtleHubWasm::stopDecodingMsg()` was an empty no-op | Medium | ✅ Fixed |
| Moxy muscle-oxygen parsing missing from `BtleHubWasm` | Medium | ✅ Fixed |
| FTMS control-point writes used `writeValueWithoutResponse` | High | ✅ Fixed |
| Moxy service (0xAAB0) absent from WebBluetooth filter list | Medium | ✅ Fixed |
| `BtleHubWasm` has no auto-reconnection logic | Low | 📋 Needs tracking issue |
| `BtleHubWasm` emits `deviceConnected` before GATT is ready | Low | 📋 Needs tracking issue |
| macOS `Info.plist` missing `NSBluetoothAlwaysUsageDescription` | Medium | 📋 Needs tracking issue |
| Linux Bluetooth runtime permissions not documented | Low | 📋 Needs tracking issue |
| Windows BLE requires Windows 10+ / WinRT (undocumented) | Low | 📋 Needs tracking issue |

---

## 2. Codebase Audit

### 2.1 Source File Inventory

| File | Lines | Purpose |
|------|-------|---------|
| `btle_hub.h` / `.cpp` | 130 / 603 | Native Qt Bluetooth hub — desktop |
| `btle_hub_wasm.h` / `.cpp` | 86 / 210 | WebBluetooth hub — WASM |
| `btle_scanner_dialog.h` / `.cpp` | 53 / 144 | Device-picker dialog — desktop |
| `btle_scanner_dialog_wasm.h` / `.cpp` | 34 / 47 | Browser-picker stub — WASM |
| `simulator_hub.h` / `.cpp` | 71 / 80 | Synthetic data generator |
| `webbluetooth_bridge.h` / `.cpp` | 57 / 175 | Emscripten JS bridge |
| `btle_uuids.h` | 20 | UUID constants |
| `btle.pri` | 32 | Build configuration |

### 2.2 No-Op / Stub Methods Found

| Method | File | Line | Finding |
|--------|------|------|---------|
| `BtleHubWasm::stopDecodingMsg()` | `btle_hub_wasm.cpp` | 51 | **Was empty.** Fixed to call `disconnectFromDevice()`. |
| `BtleHubWasm::connectToDevice<T>()` | `btle_hub_wasm.h` | 38 | Intentional stub — ignores device argument and calls `scanForDevice()`. Browser owns device selection. By design. |
| `BtleScannerDialogWasm::selectedDevice()` | `btle_scanner_dialog_wasm.h` | 30 | Returns `int 0` intentionally. `BtleHubWasm::connectToDevice<int>()` discards the argument. By design. |
| `SimulatorHub::setLoad()` / `setSlope()` | `simulator_hub.cpp` | 63–74 | Not stubs — both have real implementations clamping simulated power. |

### 2.3 Platform `#ifdef` Guards

All guards are legitimate Qt5/Qt6 compatibility shims:

| File | Lines | Guard condition | Reason |
|------|-------|----------------|--------|
| `btle_hub.cpp` | 11–19 | `QT_VERSION >= 6.0.0` | `ServiceClassUuid` enum moved into nested scope in Qt6 |
| `btle_hub.cpp` | 24–32 | `QT_VERSION >= 6.0.0` | `CharacteristicType` enum moved |
| `btle_hub.cpp` | 40–46 | `QT_VERSION >= 6.0.0` | `DescriptorType` enum moved |
| `btle_hub.cpp` | 104–112 | `QT_VERSION >= 6.0.0` | `errorOccurred` signal renamed from `error` |
| `btle_hub.cpp` | 280–285 | `QT_VERSION >= 6.0.0` | `RemoteService` state renamed |
| `btle_hub.cpp` | 322–326 | `QT_VERSION >= 6.0.0` | `RemoteServiceDiscovered` state renamed |
| `btle_scanner_dialog.cpp` | 32–40 | `QT_VERSION >= 6.0.0` | Same `error`→`errorOccurred` rename |

No `#ifdef Q_OS_WIN` or similar platform-suppression blocks exist in the BTLE
source.  Platform differences are handled at the build level (`btle.pri` file
selection) not by in-source preprocessor guards.

### 2.4 TODO / FIXME Scan

```
grep -rn "TODO\|FIXME\|HACK" src/btle/
```

**Result: zero hits.**  No outstanding markers in the BTLE subsystem.

---

## 3. Supported BLE Profiles

| Profile | Service UUID | Characteristic UUID | Desktop | WASM |
|---------|-------------|---------------------|---------|------|
| Heart Rate | 0x180D | 0x2A37 | ✅ Full | ✅ Full |
| Cycling Speed & Cadence (CSC) | 0x1816 | 0x2A5B | ✅ Full | ✅ Full |
| Cycling Power | 0x1818 | 0x2A63 | ✅ Full | ✅ Full |
| Fitness Machine (FTMS) | 0x1826 | 0x2AD2 / 0x2AD9 | ✅ Full | ✅ Full |
| Moxy Muscle Oxygen | 0xAAB0 (prop.) | 0xAAB2 | ✅ Full | ✅ Fixed (was missing) |

### Trainer Control (ERG / Slope)

| Command | FTMS Opcode | Desktop | WASM |
|---------|------------|---------|------|
| Set Target Power | 0x05 | ✅ `setLoad()` | ✅ `setLoad()` |
| Set Indoor Bike Simulation (slope) | 0x11 | ✅ `setSlope()` | ✅ `setSlope()` |
| Request Control | 0x00 | ✅ Auto on service discovery | ⚠️ Not implemented — tracked |

---

## 4. Multi-Platform Feature Parity Matrix

| Feature | Linux | Windows | macOS | WASM (browser) |
|---------|-------|---------|-------|----------------|
| **Device Scanning / Discovery** | ✅ | ✅ | ✅ | ✅ (browser picker) |
| **Service / Characteristic Discovery** | ✅ | ✅ | ✅ | ✅ (JS bridge) |
| **Read / Write / Notify** | ✅ | ✅ | ✅ | ✅ |
| **FTMS ERG control** | ✅ | ✅ | ✅ | ✅ |
| **Reconnection (auto, 3×, 5 s)** | ✅ | ✅ | ✅ | ❌ Not implemented |
| **Moxy oxygen sensor** | ✅ | ✅ | ✅ | ✅ (fixed) |
| **CSC rollover-safe math** | ✅ | ✅ | ✅ | ✅ |
| **RSSI / signal strength** | ❌ Not exposed | ❌ Not exposed | ❌ Not exposed | ❌ Not exposed |
| **Battery level** | ❌ Not implemented | ❌ Not implemented | ❌ Not implemented | ❌ Not implemented |

### Error Handling Discrepancies

| Scenario | Linux | Windows | macOS | WASM |
|----------|-------|---------|-------|------|
| Controller error | `connectionError(QString)` signal | `connectionError(QString)` | `connectionError(QString)` | `console.error` only — no C++ signal emitted |
| Device disappears mid-workout | `deviceDisconnected` + auto-reconnect | Same | Same | `gattserverdisconnected` event logged to console only |
| Scan timeout | 10 s `QTimer` in scanner dialog | Same | Same | Browser-enforced, no C++ timeout |

### RSSI / Signal Strength

Neither `BtleHub` nor `BtleHubWasm` exposes signal strength (RSSI) to the
application layer.  `QBluetoothDeviceInfo::rssi()` is available in Qt but is
not wired into any signal or stored.  This is an accepted limitation for the
current feature set.

---

## 5. Platform-Specific Runtime Requirements

### 5.1 Linux

- **Runtime daemon:** `bluetoothd` (BlueZ ≥ 5.50 recommended)
- **User permissions:** The application user must be in the `bluetooth` group, or `bluetoothd` must grant access via D-Bus policy.  
  Command: `sudo usermod -aG bluetooth $USER` (requires re-login)
- **Kernel modules:** `bluetooth`, `bluetooth_6lowpan`, `hci_uart` (or HCI USB equivalent) must be loaded
- **Qt module:** `QT += bluetooth` (included; no extra install needed beyond `libqt6bluetooth6`)

> ⚠️ **Not documented in the project.** See tracked issue.

### 5.2 Windows

- **Minimum OS:** Windows 10 version 1703 (Creators Update) — required for the WinRT Bluetooth LE APIs used by Qt Bluetooth
- **Windows 7 / 8 / 8.1:** Not supported; Qt Bluetooth silently fails or returns an empty device list
- **Permissions:** No extra manifest entries needed; standard desktop app access to Bluetooth
- **Windows SDK:** Qt 6 uses WinRT APIs (`Windows.Devices.Bluetooth`); requires the Windows 10 SDK (≥ 10.0.17134)
- **Bluetooth adapter:** Must be Bluetooth 4.0+ and have a WDM-compatible driver

> ⚠️ **Minimum OS requirement not documented in the project.** See tracked issue.

### 5.3 macOS

- **Minimum OS:** macOS 10.13 (High Sierra) for CoreBluetooth with Qt 6
- **Permissions:** Applications that access Bluetooth must declare the purpose string in `Info.plist`:
  ```xml
  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>MaximumTrainer needs Bluetooth to connect to your bike trainer and sensors.</string>
  ```
  Without this key, macOS 12+ silently blocks BLE access.
- **Entitlements:** For App Store distribution, `com.apple.security.device.bluetooth` entitlement is required

> ⚠️ **`Info.plist` key missing from the project.** See tracked issue.

### 5.4 WebAssembly (Browser)

- **API:** Web Bluetooth (`navigator.bluetooth`) — implemented in `webbluetooth_bridge.cpp`
- **Browser support:** Chrome 56+, Edge 79+.  Firefox and Safari do not support Web Bluetooth
- **Security requirements:**
  - Page must be served over **HTTPS** (or `localhost` for development)
  - Device selection must be triggered by a **user gesture** (button click)
- **Thread model:** Single-threaded Emscripten (`wasm_singlethread`); no `QThread` use
- **Limitations vs native:**
  - No automatic reconnection (browser manages lifecycle)
  - No RSSI access
  - Device list managed by browser (not by app)
  - FTMS Request Control (opcode 0x00) not sent before ERG commands — tracked

---

## 6. Gap Analysis & Backlog

The following gaps were identified and are tracked as GitHub Issues:

### Gap 1 — BtleHubWasm: No Auto-Reconnection Logic

**Affected file:** `src/btle/btle_hub_wasm.cpp`  
**Severity:** Low  
**Description:** `BtleHub` (desktop) automatically retries connection up to 3 times at 5-second intervals after disconnection.  `BtleHubWasm` emits `deviceDisconnected` but makes no reconnection attempt.  In practice, the browser's GATT server connection is managed by the Web Bluetooth API, but a user-visible reconnect prompt should be shown.

### Gap 2 — BtleHubWasm: `deviceConnected` Emitted Before GATT Ready

**Affected file:** `src/btle/btle_hub_wasm.cpp`, line 25–26  
**Severity:** Low  
**Description:** `scanForDevice()` calls `WebBluetoothBridge::scanForDevices()` (async) and immediately emits `deviceConnected()` and `serviceDiscoveryFinished()`.  The GATT connection has not yet completed at that point.  Any ERG commands sent immediately after `deviceConnected` may be silently dropped.  The `js_scanAndConnect` JS function does connect and subscribe asynchronously before any data arrives, so in practice this race is rarely hit, but the signal contract is technically incorrect.

### Gap 3 — macOS: Missing `NSBluetoothAlwaysUsageDescription` in Info.plist

**Affected file:** _None — `Info.plist` does not exist in the project_  
**Severity:** Medium  
**Description:** macOS 12 (Monterey) and later require the `NSBluetoothAlwaysUsageDescription` key in `Info.plist` for any app that accesses CoreBluetooth.  Without it, the system silently refuses BLE access with no error.  An `Info.plist` must be created (or the qmake `QMAKE_INFO_PLIST` variable used to point to one) and the key added.

### Gap 4 — Linux: Bluetooth Runtime Permissions Not Documented

**Affected files:** `README.md`, CI workflow  
**Severity:** Low  
**Description:** New Linux users may not have their account in the `bluetooth` group, causing the BLE scanner to return an empty device list with no actionable error.  Installation instructions should include the group membership command.

### Gap 5 — Windows: Minimum OS Version (Windows 10) Not Documented

**Affected files:** `README.md`  
**Severity:** Low  
**Description:** Qt Bluetooth on Windows uses WinRT APIs that are unavailable on Windows 7/8.  The README does not state the Windows 10+ requirement.

### Gap 6 — FTMS Request Control Opcode Missing from WASM Path

**Affected file:** `src/btle/webbluetooth_bridge.cpp`  
**Severity:** Medium  
**Description:** The native `BtleHub` sends FTMS opcode 0x00 (Request Control) to the control-point characteristic (0x2AD9) before issuing any ERG commands.  Some trainers (e.g. Tacx NEO) require this handshake and will silently reject `Set Target Power` without it.  The WASM bridge sends ERG commands directly without requesting control first.

---

## 7. Test Coverage Summary

| Test suite | Location | Tests | Coverage |
|-----------|----------|-------|---------|
| Unit (parsing) | `tests/btle/tst_btle_hub.cpp` | 26 | HR, CSC, Power, FTMS, Moxy, SimulatorHub, 3 brand simulators |
| Integration | `tests/integration/tst_btle_integration.cpp` | 1 | SimulatorHub → SensorDisplayWidget lifecycle |
| WASM / browser | `tests/playwright/wasm_webapp.spec.js` | 3 | Asset availability, page load, no "not deployed" sentinel |

All 26 unit tests pass via `./build/tests/btle_tests`.  The integration test
requires Xvfb and runs in CI (`test_btle_integration` job).

---

## 8. Recommendations

1. **Add `Info.plist`** with `NSBluetoothAlwaysUsageDescription` for macOS builds (Gap 3).
2. **Document Linux group membership** in `README.md` (Gap 4).
3. **Document Windows 10+ requirement** in `README.md` (Gap 5).
4. **Implement FTMS Request Control** in the WASM bridge before first ERG command (Gap 6).
5. **Add a reconnect UI prompt** in the WASM path when `gattserverdisconnected` fires (Gap 1).
6. **Defer `deviceConnected` signal** in `BtleHubWasm` until the JS async connection callback confirms GATT is ready (Gap 2).

---

*Generated as part of the Qt-upgrade BTLE validation audit (see issue #Validate-BTLE-QT-Upgrade).*
