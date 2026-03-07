## Context

Source TODO: `src/btle/btle_hub_wasm.cpp` — `BtleHubWasm::scanForDevice()`
Audit reference: [`btle-audit.md`](../../btle-audit.md) section 6, Gap 2
Severity: **Low**

## Problem

`BtleHubWasm::scanForDevice()` calls `WebBluetoothBridge::scanForDevices()`
(which launches the async JS `js_scanAndConnect` function) and then
**immediately** emits `deviceConnected()` and `serviceDiscoveryFinished()`.

The GATT connection has not completed at this point. The JS function connects
asynchronously in the browser's microtask queue. Any ERG commands (`setLoad`
or `setSlope`) sent by `WorkoutDialog` immediately after `deviceConnected` may
be silently dropped because the FTMS control-point characteristic is not yet
subscribed.

In practice this race is rarely hit because the workout start requires a user
action after the connect button, but the signal contract is technically broken.

## Requirement

- Route a "GATT ready" callback from JS back to C++ (e.g. extend `bleNotifyC`
  or add a dedicated EM_JS callback) that fires once `js_scanAndConnect` has
  successfully connected and subscribed.
- Defer the emission of `deviceConnected()` and `serviceDiscoveryFinished()`
  to that callback.

## Acceptance Criteria

- [ ] `deviceConnected` is emitted only after `js_scanAndConnect` reports
      success.
- [ ] ERG commands sent immediately after `deviceConnected` are not dropped.
- [ ] Existing unit tests for `BtleHubWasm` continue to pass.
