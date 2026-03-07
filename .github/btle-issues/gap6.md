## Context

Source TODO: `src/btle/webbluetooth_bridge.cpp` — above `js_sendFtmsCommand`
Audit reference: [`btle-audit.md`](../../btle-audit.md) section 3 and section 6, Gap 6
Severity: **Medium**

## Problem

The FTMS specification requires that a client sends **opcode 0x00
(Request Control)** to the FTMS Control Point characteristic (0x2AD9) before
issuing any training-load commands. Some trainers — notably the **Tacx NEO**
family — enforce this handshake and silently reject `Set Target Power` (0x05)
and `Set Indoor Bike Simulation` (0x11) commands without it.

The native `BtleHub` desktop path calls `requestFtmsControl()` automatically
during service discovery.

The `BtleHubWasm` / `WebBluetoothBridge` path sends ERG commands via
`js_sendFtmsCommand` **without** ever sending opcode 0x00 first. ERG mode will
silently fail on trainers that enforce the handshake.

## Requirement

1. Add a `js_requestFtmsControl()` EM_JS function in
   `webbluetooth_bridge.cpp` that writes a single byte `0x00` to
   characteristic 0x2AD9.
2. Call it once after the async GATT connection is established — ideally in
   the JS success callback of `js_scanAndConnect`, or from C++ after Gap 2 is
   resolved so the call happens after `deviceConnected`.
3. Expose it through the `WebBluetoothBridge` namespace so `BtleHubWasm` can
   call it from `scanForDevice()`.

## Implementation sketch

```cpp
EM_JS(void, js_requestFtmsControl, (), {
    (async function() {
        if (!window._mtBleDevice || !window._mtBleDevice.gatt.connected) return;
        try {
            const server = window._mtBleDevice.gatt;
            const service = await server.getPrimaryService(0x1826);
            const ctrl = await service.getCharacteristic(0x2AD9);
            await ctrl.writeValueWithResponse(new Uint8Array([0x00]));
        } catch (e) {
            console.warn('[MT] FTMS Request Control failed:', e);
        }
    })();
});
```

## Acceptance Criteria

- [ ] FTMS opcode 0x00 is sent to 0x2AD9 once after GATT connection is
      established in the WASM build.
- [ ] ERG commands are accepted by Tacx NEO and similar trainers in the
      browser build.
- [ ] No regression on trainers that do not require the handshake (they
      ignore opcode 0x00).
