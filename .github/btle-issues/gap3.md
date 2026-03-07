## Context

Audit reference: [`btle-audit.md`](../../btle-audit.md) section 5.3 and section 6, Gap 3
Severity: **Medium**

## Problem

macOS 12 (Monterey) and later silently block CoreBluetooth access for any
application that does not declare the `NSBluetoothAlwaysUsageDescription` key
in its `Info.plist`. The app receives no error; the BLE scanner simply returns
an empty device list with no explanation to the user.

The MaximumTrainer project has **no `Info.plist`** file at all. This means the
macOS build will fail to discover any BLE devices on macOS 12+.

## Requirement

1. Create `mac/Info.plist` containing at minimum:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>MaximumTrainer needs Bluetooth to connect to your bike trainer
  and sensors.</string>
  <key>CFBundleName</key>
  <string>MaximumTrainer</string>
</dict>
</plist>
```

2. Set `QMAKE_INFO_PLIST = mac/Info.plist` in the macOS scope of
   `PowerVelo.pro`.
3. For App Store distribution, add the
   `com.apple.security.device.bluetooth` entitlement.

## Acceptance Criteria

- [ ] macOS CI build includes the `Info.plist` in the app bundle.
- [ ] BLE scanning works on macOS 12+ (Monterey and later).
- [ ] No CoreBluetooth permission errors in the system log.
