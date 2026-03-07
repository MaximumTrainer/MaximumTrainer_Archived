## Context

Source TODO: `src/btle/btle_hub_wasm.cpp` — `BtleHubWasm::disconnectFromDevice()`
Audit reference: [`btle-audit.md`](../../btle-audit.md) section 6, Gap 1
Severity: **Low**

## Problem

`BtleHub` (desktop) automatically retries the BLE connection up to 3 times at
5-second intervals after an unexpected disconnection.

`BtleHubWasm` emits `deviceDisconnected` but makes no reconnection attempt and
shows no reconnect prompt to the user. In practice the browser owns the GATT
lifecycle via the Web Bluetooth API, but a user-visible "Reconnect" button or
dialog should be shown when `gattserverdisconnected` fires so the user can
trigger `navigator.bluetooth.requestDevice()` again (which requires a user
gesture).

## Requirement

- Emit `connectionError()` from `disconnectFromDevice()` so `WorkoutDialog`
  can surface a reconnect prompt.
- Show a "Reconnect" button that calls `BtleHubWasm::scanForDevice()`.
- Optionally mirror the desktop auto-retry with a JS `gattserverdisconnected`
  event listener that calls back into C++ via an EM_JS helper.

## Acceptance Criteria

- [ ] User sees a reconnect prompt when the trainer disconnects mid-workout in
      the browser build.
- [ ] Clicking reconnect re-triggers the Web Bluetooth device picker.
- [ ] Unit/integration test or Playwright test verifies the prompt appears.
