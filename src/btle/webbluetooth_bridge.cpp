/*
 * webbluetooth_bridge.cpp
 *
 * Implements the C++ ↔ JavaScript bridge for the browser WebBluetooth API.
 * Compiled only when targeting wasm-emscripten (see btle.pri).
 *
 * The EM_JS blocks below are compiled into the .wasm module as JavaScript
 * helper functions.  Emscripten's ASYNCIFY allow the async GATT operations to
 * be awaited without blocking the main thread.
 */

#include "webbluetooth_bridge.h"

#include <emscripten.h>
#include <emscripten/val.h>

#include <QDebug>
#include <cstring>

// ─── Internal state ──────────────────────────────────────────────────────────
static BleNotificationCallback g_notificationCallback;

// ─── JS callback entry point (called from JS via Module._bleNotify) ──────────
extern "C" {

// Called from JavaScript when a GATT characteristic value changes.
// uuid16  – the 16-bit BT SIG characteristic UUID
// dataPtr – pointer into Emscripten HEAP8 holding the raw bytes
// dataLen – number of bytes
EMSCRIPTEN_KEEPALIVE
void bleNotifyC(int uuid16, const uint8_t *dataPtr, int dataLen)
{
    if (g_notificationCallback) {
        QByteArray bytes(reinterpret_cast<const char *>(dataPtr), dataLen);
        g_notificationCallback(static_cast<quint16>(uuid16), bytes);
    }
}

} // extern "C"

// ─── JavaScript helpers ───────────────────────────────────────────────────────

// Scan for a BLE device, connect, discover services, and start notifications
// on the Heart Rate (0x180D), Cycling Power (0x1818), CSC (0x1816), and FTMS
// (0x1826) characteristics.
EM_JS(void, js_scanAndConnect, (), {
    (async function() {
        try {
            const device = await navigator.bluetooth.requestDevice({
                filters: [
                    { services: [0x1826] }, // FTMS
                    { services: [0x1818] }, // Cycling Power
                    { services: [0x1816] }, // CSC
                    { services: [0x180D] }, // Heart Rate
                    { services: [0xAAB0] }  // Moxy Muscle Oxygen
                ],
                optionalServices: [0x1826, 0x1818, 0x1816, 0x180D, 0xAAB0]
            });

            window._mtBleDevice = device;
            device.addEventListener('gattserverdisconnected', function() {
                console.log('[MT] BLE device disconnected');
            });

            const server = await device.gatt.connect();

            // Map of service UUID → array of characteristic UUIDs to subscribe to
            const profileMap = {
                0x180D: [0x2A37],       // HR Measurement
                0x1818: [0x2A63],       // Cycling Power Measurement
                0x1816: [0x2A5B],       // CSC Measurement
                0x1826: [0x2AD2],       // Indoor Bike Data
                0xAAB0: [0xAAB2]        // Moxy Muscle Oxygen Measurement
            };

            for (const [svcUuid, charUuids] of Object.entries(profileMap)) {
                let service;
                try {
                    service = await server.getPrimaryService(parseInt(svcUuid));
                } catch (e) {
                    continue; // service not present on this device
                }

                for (const charUuid of charUuids) {
                    let characteristic;
                    try {
                        characteristic = await service.getCharacteristic(charUuid);
                    } catch (e) {
                        continue;
                    }
                    await characteristic.startNotifications();
                    characteristic.addEventListener('characteristicvaluechanged', function(event) {
                        const value = event.target.value;
                        const bytes = new Uint8Array(value.buffer);
                        const ptr = Module._malloc(bytes.length);
                        Module.HEAPU8.set(bytes, ptr);
                        Module._bleNotifyC(charUuid, ptr, bytes.length);
                        Module._free(ptr);
                    });
                }
            }

            console.log('[MT] BLE connected and notifications started');
        } catch (err) {
            console.error('[MT] WebBluetooth error:', err);
        }
    })();
});

// Disconnect from the current GATT device
EM_JS(void, js_disconnect, (), {
    if (window._mtBleDevice && window._mtBleDevice.gatt.connected) {
        window._mtBleDevice.gatt.disconnect();
    }
});

// Send raw bytes to a FTMS control point characteristic (0x2AD9)
// TODO(Gap 6): The FTMS spec requires opcode 0x00 (Request Control) to be
// sent to 0x2AD9 before any Set Target Power (0x05) or Indoor Bike Simulation
// (0x11) commands.  BtleHub (desktop) calls requestFtmsControl() during
// service discovery.  Add a js_requestFtmsControl() EM_JS helper that writes
// opcode 0x00 once after connect, and call it from scanForDevice() (or from
// the JS async connect callback once Gap 2 is resolved).  Without this,
// trainers such as Tacx NEO silently reject ERG commands.
EM_JS(void, js_sendFtmsCommand, (const uint8_t *dataPtr, int dataLen), {
    (async function() {
        if (!window._mtBleDevice || !window._mtBleDevice.gatt.connected) return;
        try {
            const server = window._mtBleDevice.gatt;
            const service = await server.getPrimaryService(0x1826);
            const char = await service.getCharacteristic(0x2AD9);
            const bytes = Module.HEAPU8.slice(dataPtr, dataPtr + dataLen);
            await char.writeValueWithResponse(bytes);
        } catch (e) {
            console.warn('[MT] FTMS write failed:', e);
        }
    })();
});

// ─── Public C++ API ───────────────────────────────────────────────────────────

namespace WebBluetoothBridge {

void setNotificationCallback(BleNotificationCallback cb)
{
    g_notificationCallback = std::move(cb);
}

void scanForDevices()
{
    js_scanAndConnect();
}

void disconnectDevice()
{
    js_disconnect();
}

void sendFtmsSetIndoorBikeSimulation(int grade100)
{
    // FTMS Set Indoor Bike Simulation Parameters (opcode 0x11)
    // Payload: opcode(1) + wind_speed(2) + grade(2) + crr(1) + wind_res(1)
    uint8_t cmd[7] = { 0x11 };
    int16_t gradeVal = static_cast<int16_t>(grade100); // units: 0.01 %
    std::memcpy(&cmd[3], &gradeVal, 2);
    js_sendFtmsCommand(cmd, sizeof(cmd));
}

void sendFtmsSetTargetPower(int watts)
{
    // FTMS Set Target Power (opcode 0x05)
    uint8_t cmd[3] = { 0x05 };
    int16_t power = static_cast<int16_t>(watts);
    std::memcpy(&cmd[1], &power, 2);
    js_sendFtmsCommand(cmd, sizeof(cmd));
}

} // namespace WebBluetoothBridge
