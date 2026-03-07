# MaximumTrainer — Agents & Architecture Guide

> **Audience:** New contributors, AI coding agents, and senior reviewers.  
> **Purpose:** Define the system architecture, design patterns, cross-platform
> strategy, testing approach, and operational best-practices for the
> MaximumTrainer codebase so that every change remains consistent,
> maintainable, and safe.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Code Architecture & Design Patterns](#2-code-architecture--design-patterns)
3. [Target Runtimes & Cross-Platform Strategy](#3-target-runtimes--cross-platform-strategy)
4. [Testing Approach & Quality Assurance](#4-testing-approach--quality-assurance)
5. [Best Practices for Maintainability & Scalability](#5-best-practices-for-maintainability--scalability)
6. [Performance & Safety](#6-performance--safety)

---

## 1. Project Overview

MaximumTrainer is a cross-platform cycling and rowing training application
built with **Qt / C++17**.  It connects to smart trainers, power meters, heart
rate monitors, cadence sensors, and rowing ergometers via Bluetooth Low Energy
(BLE) and ANT+, plays structured interval workouts in ERG-mode, and exports
completed activities to the Garmin FIT format.

| Attribute | Value |
|-----------|-------|
| Language | C++17 |
| UI Toolkit | Qt 6 (Widgets + WebEngineWidgets) |
| Plotting | QWT 6.3 |
| Serialisation | Garmin FIT SDK, TCX, GPX, XML |
| Hardware protocols | BLE (Qt Bluetooth), ANT+ (USB HID — stub on Windows) |
| Audio/Video | VLC-Qt (desktop), SFML (audio), platform stubs for WASM |
| Build system | qmake `.pro` / `.pri` |
| CI/CD | GitHub Actions (Linux · Windows · macOS · WebAssembly) |

---

## 2. Code Architecture & Design Patterns

### 2.1 Layered Architecture

MaximumTrainer is organised into **four horizontal layers**.  Each layer depends
only on the layers below it; the UI and hardware layers are isolated from each
other by the domain model.

```
┌─────────────────────────────────────────────────────────┐
│                     UI Layer                            │
│  src/ui/  (MainWindow, WorkoutDialog, WorkoutCreator)   │
│  Qt Widgets · QWT plots · .ui form files                │
└────────────────────┬────────────────────────────────────┘
                     │  Qt signals / slots
┌────────────────────▼────────────────────────────────────┐
│                  Domain / Training Engine               │
│  src/model/   (Workout, Interval, DataWorkout, …)       │
│  src/workout/ (WorkoutUtil — format conversion)         │
│  src/fitness/ (FIT SDK, Achievements)                   │
│  Pure C++ — no Qt hardware or UI dependencies           │
└───────────┬──────────────────────────┬──────────────────┘
            │                          │
┌───────────▼──────────┐  ┌────────────▼─────────────────┐
│  Hardware             │  │  Persistence                 │
│  Abstraction Layer    │  │  src/persistence/            │
│  (HAL)                │  │  SQLite DAOs (users, sensors,│
│  src/btle/ src/ANT/   │  │  achievements), FIT/TCX/GPX  │
│  BtleHub · SimHub     │  │  file writers/readers        │
│  AntHub (stubs)       │  │                              │
└──────────────────────┘  └──────────────────────────────┘
```

**Key invariant:** `src/model/`, `src/workout/`, and `src/fitness/` must
**never** `#include` anything from `src/btle/`, `src/ANT/`, or `src/ui/`.
Hardware and UI concerns are injected via Qt signals/slots and constructor
parameters.

### 2.2 Hardware Abstraction Layer (HAL)

All hardware communication goes through one of three interchangeable hub
classes that present **identical signal/slot contracts**:

| Class | Location | Purpose |
|-------|----------|---------|
| `BtleHub` | `src/btle/btle_hub.h` | Real BLE hardware via Qt Bluetooth |
| `BtleHubWasm` | `src/btle/btle_hub_wasm.h` | Web Bluetooth API via JS bridge |
| `SimulatorHub` | `src/btle/simulator_hub.h` | Synthetic data for CI & demos |
| `Hub` (ANT+) | `src/ANT/hub.h` | ANT+ USB; no-op stubs on all platforms |

The **signal contract** shared by every hub:

```cpp
signals:
    void signal_hr     (int userID, int hr);
    void signal_cadence(int userID, int cadence);
    void signal_speed  (int userID, double speed);   // km/h
    void signal_power  (int userID, int power);      // watts
    void signal_oxygen (int userID, double smo2, double thb);

    void deviceConnected();
    void deviceDisconnected();
    void connectionError(const QString &errorString);
```

```cpp
public slots:
    void setLoad (int antID, double watts);
    void setSlope(int antID, double grade);
    void stopDecodingMsg();
```

`WorkoutDialog` connects to whichever hub is active and is unaware of the
underlying implementation.  Swapping from real hardware to simulation is
a single `connect()` call change in `MainWindow::executeWorkout()`.

### 2.3 Observer / Reactive Data Streams

High-frequency sensor data (≥10 Hz) is processed with the **Observer pattern**
using Qt's **signal/slot** mechanism:

```
BtleHub (producer)
  ──signal_power(userId, watts)──►  WorkoutDialog (consumer)
  ──signal_hr(userId, bpm)───────►  WorkoutDialog
  ──signal_cadence(userId, rpm)──►  WorkoutDialog
                                     │
                                     ▼
                              DataWorkout (recorder)
                                     │
                                     ▼
                              WorkoutPlot (renderer)
```

All signal emissions from hardware hubs happen on the **main Qt thread**.
`BtleHub` uses `QLowEnergyController` callbacks that Qt dispatches on the
thread that owns the controller object — always `MainWindow`'s thread — so no
explicit thread-safety guards are needed in the current single-threaded-UI
design.

### 2.4 Agent-Based Model

Each subsystem is modelled as an **autonomous agent** that owns its state and
communicates exclusively through Qt signals/slots:

| Agent | Owns | Communicates via |
|-------|------|-----------------|
| `BtleHub` | BLE connection state, characteristic subscriptions | Signals: `signal_*`, `deviceConnected`, `connectionError` |
| `SimulatorHub` | Timer-driven synthetic telemetry | Same signals as BtleHub |
| `WorkoutDialog` | Active workout session state (elapsed time, target power, lap index) | Receives sensor signals; emits `setLoad` / `setSlope` to hub |
| `DataWorkout` | Per-second telemetry recording | Populated by WorkoutDialog during playback |
| `AchievementChecker` | Achievement evaluation logic | Called post-workout by WorkoutDialog |
| `FitActivityCreator` | FIT file serialisation | Invoked by WorkoutDialog on session save |

This agent topology means that each component can be unit-tested in isolation
by connecting test spies instead of real counterparts.

### 2.5 Model / View Separation

All list data exposed in the UI follows Qt's **Model/View** pattern:

| Model class | Data | Used by |
|-------------|------|---------|
| `WorkoutTableModel` | `QList<Workout>` | MainWindow workout browser |
| `IntervalTableModel` | `QList<Interval>` | WorkoutCreator interval editor |
| `CourseTableModel` | `QList<Course>` | Course browser |
| `RadioTableModel` | `QList<Radio>` | Sensor configuration dialog |
| `SortFilterProxyModel` | Wraps any above | Search boxes |

Views (`QTableView`, `QListView`) are never given raw data — only model
pointers — preventing direct coupling between UI and persistence layers.

---

## 3. Target Runtimes & Cross-Platform Strategy

### 3.1 Supported Platforms

| Platform | Qt Version | Compiler | Status |
|----------|-----------|----------|--------|
| Linux (Ubuntu 22.04+) | 6.5+ | GCC 11+ | ✅ Primary development target |
| Windows 10/11 | 6.5.3 | MSVC 2019 (x64) | ✅ Release build |
| macOS 13+ (Apple Silicon / Intel) | 6.5.3 | Clang | ✅ Release build |
| WebAssembly (browser) | 6.6.3 | Emscripten 3.1.43 | ⚠️ Best-effort (`continue-on-error`) |

### 3.2 Shared Core, Platform Adapters

The strategy is **"maximise shared C++ core; isolate platform differences
behind compile-time guards or swappable adapters"**.

```
┌────────────────────────────────────┐
│         Shared Core (all targets)  │
│   model/ · workout/ · fitness/     │
│   persistence/ · ui/               │
└──────────┬─────────────────────────┘
           │
     ┌─────▼──────────────────────────────────────────┐
     │         Platform adapters (selected at build)   │
     │                                                 │
     │  BLE:     btle_hub.cpp         (desktop)        │
     │           btle_hub_wasm.cpp    (WASM)           │
     │           webbluetooth_bridge  (WASM JS bridge) │
     │                                                 │
     │  Audio:   myvlcplayer.cpp      (desktop+VLC)    │
     │           soundplayer.cpp      (SFML desktop)   │
     │           soundplayer_wasm.cpp (WASM stub)      │
     │                                                 │
     │  Scanner: btle_scanner_dialog.cpp   (desktop)   │
     │           btle_scanner_dialog_wasm  (WASM)      │
     │                                                 │
     │  WebEngine: real QtWebEngineWidgets (desktop)   │
     │             src/ui/wasm_stubs/ header stubs     │
     │                                                 │
     │  ANT+:    hub.cpp  (Linux with libusb)          │
     │           hub_*_stub.cpp (Windows / macOS / no-op) │
     └─────────────────────────────────────────────────┘
```

Platform selection is controlled **at qmake time** using `.pro` / `.pri`
scopes (`wasm`, `win32`, `macx`, `linux`) and optional defines
(`GC_HAVE_VLCQT`).  No runtime `#ifdef` branching inside shared logic files.

### 3.3 WASM-Specific Constraints

| Concern | Constraint | Mitigation |
|---------|-----------|-----------|
| No native BLE APIs | Web Bluetooth is promise-based | `BtleHubWasm` + `WebBluetoothBridge` (Emscripten embind + JS callbacks) |
| No file-system access | `QFile` writes are in-memory | Persist via browser download prompt |
| No VLC / SFML | Shared libs unavailable | `soundplayer_wasm.cpp` stub; no video |
| Single-threaded Emscripten | `pthread` unavailable (singlethread build) | No `QThread` use in WASM paths |
| `QWebEngineWidgets` absent | Not ported to WASM | Stub headers in `src/ui/wasm_stubs/` |

### 3.4 Bluetooth LE Architecture

```
Desktop (Qt Bluetooth)              WASM (Web Bluetooth)
─────────────────────               ──────────────────────
BtleHub                             BtleHubWasm
  └─ QLowEnergyController             └─ WebBluetoothBridge (C++)
       └─ Qt platform plugin               └─ JavaScript embind bindings
            └─ OS BLE stack                     └─ navigator.bluetooth API
```

Both paths emit **identical signals** to `WorkoutDialog`, ensuring zero
divergence in training-engine logic between platforms.

---

## 4. Testing Approach & Quality Assurance

### 4.1 Testing Pyramid

```
              ╔══════════════════╗
              ║  System / E2E    ║  (Playwright WASM, integration screenshots)
              ╚══════╤═══════════╝
           ╔═════════╧════════════════╗
           ║  Integration Tests       ║  (BtleHub + WorkoutDialog + SimulatorHub)
           ╚═══════╤══════════════════╝
        ╔══════════╧═══════════════════════╗
        ║  Unit Tests                      ║  (BtleHub parsing, model, workout util)
        ╚══════════════════════════════════╝
```

### 4.2 Unit Tests — `tests/btle/`

**Project file:** `tests/btle/btle_tests.pro`  
**Runner:** `tests/btle/tst_btle_hub.cpp` (28 test cases)  
**Framework:** Qt Test (`QTest`, `QSignalSpy`)

Tests exercise `BtleHub::simulateNotification()` — a dedicated test hook that
injects raw BLE characteristic byte arrays as if received from hardware:

```cpp
// Inject a Heart Rate Measurement characteristic notification
hub.simulateNotification(0x2A37, QByteArray::fromHex("0060"));   // 96 bpm
QCOMPARE(spy.count(), 1);
QCOMPARE(spy.takeFirst().at(1).toInt(), 96);
```

**Coverage areas:**

| Group | Tests | What is verified |
|-------|-------|-----------------|
| HR parsing | 5 | 8-bit/16-bit flags, RR-interval presence, zero, max |
| CSC parsing | 6 | Crank-only, wheel-only, combined, uint16 rollover, standstill, first-measurement discard |
| Power parsing | 3 | Positive, zero, negative (track-stand) |
| FTMS parsing | 5 | Speed-only, cadence-only, power-only, all-fields, zero values |
| Trainer sims | 3 | Elite (FTMS), Wahoo KICKR (Power+CSC), Garmin Tacx (FTMS+CSC) |
| SimulatorHub | 6 | Signal emission, drift-within-bounds, slot no-ops |

**Build & run:**

```bash
cd tests/btle
qmake btle_tests.pro && make -j$(nproc)
./build/tests/btle_tests -v2
```

### 4.3 Integration Tests — `tests/integration/`

**Project file:** `tests/integration/btle_integration_tests.pro`  
**Runner:** `tests/integration/tst_btle_integration.cpp`  
**Framework:** Qt Test + Xvfb (virtual X display in CI)

Integration tests launch the full `WorkoutDialog` against `SimulatorHub`,
drive it through a workout session, and assert on the resulting `DataWorkout`
object.  Screenshots are captured at key lifecycle points and uploaded as CI
artifacts:

```bash
# CI command (build.yml: test_btle_integration job)
Xvfb :99 -screen 0 1920x1080x24 &
DISPLAY=:99 ./build/tests/btle_integration_tests -v2
```

**What integration tests cover:**

- Full workout session lifecycle (start → pause → resume → stop)
- `SimulatorHub` data flowing through `WorkoutDialog` into `DataWorkout`
- ERG-mode load commands sent from `WorkoutDialog` back to the hub
- Lap transitions and interval advancement logic

### 4.4 Hardware Mocks & Stubs

| Mechanism | File | Used in |
|-----------|------|---------|
| `SimulatorHub` | `src/btle/simulator_hub.cpp` | UI simulation mode, integration tests |
| `BtleDeviceSimulator` | `tests/btle/btle_device_simulator.h` | Unit test byte-level fake device |
| ANT+ no-op stubs | `src/ANT/hub_*_stub.cpp` | All platforms (ANT+ never linked) |
| WASM audio stub | `src/app/soundplayer_wasm.cpp` | WASM build (no SFML) |
| WASM WebEngine stub | `src/ui/wasm_stubs/` | WASM build (no QtWebEngineWidgets) |

The design principle is: **no test should require physical hardware**.
`SimulatorHub` can replace any real hub at the `MainWindow` level.
`BtleHub::simulateNotification()` covers byte-level parsing without a BLE
adapter.

### 4.5 WASM / Browser Tests — `tests/playwright/`

**Config:** `playwright.config.js` (root)  
**Spec:** `tests/playwright/wasm_webapp.spec.js`  
**Framework:** Playwright (Chromium headless)

Playwright tests run against the live GitHub Pages deployment
(`https://maximumtrainer.github.io/MaximumTrainer_Redux/app/`).
They validate:

1. **Asset availability** — `qtloader.js`, `MaximumTrainer.js`,
   `MaximumTrainer.wasm` all return HTTP 200.
2. **Page load** — Loading screen or Qt canvas becomes visible within 4 s.
3. **No "not deployed" sentinel** — Confirms the WASM artefact has been
   published.

A `navigator.bluetooth` stub is injected via `addInitScript()` so the app
does not abort immediately on browsers without real BLE.

**Run locally (after WASM deployment):**

```bash
npx playwright install chromium
npx playwright test
```

### 4.6 CI/CD Pipeline

```
Push to branch
      │
      ├─► build_linux ──► test_btle_integration (Xvfb, screenshots)
      ├─► build_windows
      ├─► build_mac
      └─► build_wasm (continue-on-error)
              │
      (master only, all non-wasm pass)
              │
              ▼
        tag_release (auto-increment semver, dispatch release.yml)
              │
              ▼
        release.yml: create GitHub Release + attach artefacts
              │
              ▼
        pages.yml: deploy docs/ + WASM to GitHub Pages
              │
              ▼
        Playwright tests run against deployed WASM
```

All jobs run in parallel.  WASM failure does not block release publication
(`continue-on-error: true` + `always()` guard on publish job).

---

## 5. Best Practices for Maintainability & Scalability

### 5.1 Dependency Injection

- **Hub injection:** `WorkoutDialog` receives its hub via `connect()` calls
  in `MainWindow::executeWorkout()`.  It never instantiates `BtleHub` or
  `SimulatorHub` directly.
- **DAO injection:** All database access objects (`UserDAO`, `SensorDAO`, …)
  are constructed once in `Environnement` (note: French spelling — the actual
  class name in `src/persistence/db/environnement.h`) and passed to consumers.
  No static/singleton DAO access.
- **Settings injection:** `Account` and `Settings` objects are passed into
  dialogs via constructor parameters or `setAccount()` setters, not obtained
  from global state inside the dialog.
- **Test guideline:** Any class that cannot be unit-tested by replacing its
  hardware or database dependency with a stub/mock is a dependency-injection
  violation.

### 5.2 Error Handling Boundaries

| Boundary | Mechanism | Recovery |
|----------|-----------|---------|
| BLE connection loss | `BtleHub::connectionError(QString)` signal | `WorkoutDialog` shows reconnect dialog; pauses ERG commands |
| BLE reconnect | `BtleHub` internal `QTimer` (5 s) re-invokes `connectToDevice` | Up to 3 automatic retries |
| File I/O failure | Return `false` + `qWarning()` in all `XmlUtil`, `FitActivityCreator` methods | UI shows save-failure dialog |
| Database error | `QSqlQuery::lastError()` checked after every exec; logged | Operation skipped; no crash |
| WASM asset load failure | `qtloader.js` error callback sets `#loading-screen` error text | User sees graceful "load failed" page |

**Rule:** No hardware or I/O error should propagate as an unhandled exception
or cause an undefined-behaviour crash.  Errors are logged with `qWarning()` /
`qCritical()` and surfaced to the user via a status label or dialog.

### 5.3 Modularisation

The codebase is segmented into **qmake `.pri` modules**, each with a single
responsibility:

| Module (`.pri`) | Responsibility | Internal dependencies |
|-----------------|----------------|----------------------|
| `src/model/model.pri` | Pure domain model (Workout, Interval, Course, …) | None |
| `src/workout/workout.pri` | Workout file conversion utilities | `model` |
| `src/btle/btle.pri` | BLE HAL (hub + scanner + simulator) | `model` |
| `src/ANT/ANT.pri` | ANT+ HAL (stubs on all platforms) | `model` |
| `src/persistence/persistence.pri` | SQLite DAOs + file readers/writers | `model`, `fitness` |
| `src/fitness/fitness.pri` | FIT SDK + Achievement logic | `model` |
| `src/ui/ui.pri` | All UI: MainWindow, WorkoutDialog, plots, editors | All above |
| `src/app/app.pri` | Entry point + global state (VLC, audio, utils) | `ui`, `btle`, `model` |

**Adding a new feature:**

1. Define domain types in `model/`.
2. Add persistence in `persistence/`.
3. Add business logic in `workout/` or `fitness/`.
4. Wire UI in `ui/`.
5. Never skip layers.

### 5.4 Coding Conventions

- **C++17** throughout (`std::optional`, structured bindings, `if constexpr`).
- Include paths are resolved by `INCLUDEPATH` in `.pro` — use bare filenames
  without path prefixes in `#include` directives for files within the same
  module (e.g., `#include "btle_hub.h"`).  For cross-module includes or
  third-party headers, prefer the shortest unambiguous path
  (e.g., `#include "../../src/btle/btle_hub.h"` in test files that live
  outside `src/`).
- UI files (`.ui`) are machine-edited by Qt Designer; hand-edit only for
  property additions that Designer cannot express.
- `Q_OBJECT` is required on every class that uses signals or slots.
- Signal parameter types must be **value types or `const &`** — never raw
  pointers — to avoid lifetime issues across thread boundaries.
- Use `qint64` / `quint16` etc. (not `long` / `unsigned short`) for any value
  that maps to a BLE/FIT wire type.

---

## 6. Performance & Safety

### 6.1 Memory Management for Long-Duration Workouts

`DataWorkout` accumulates one `TrackPoint` per second.  A 2-hour workout
generates ~7 200 points.  At ~64 bytes each, peak heap usage for telemetry
is **< 500 KB** — well within any platform's capacity.

Guidelines:
- **Do not** store raw BLE notification byte arrays long-term; parse
  immediately in `onCharacteristicChanged()` and discard.
- `WorkoutPlot` curves are backed by `QwtSeriesData` that references the same
  `DataWorkout` vector — no copies.  Append-only; never remove mid-workout.
- `FitActivityCreator` writes the FIT file in a **single pass** at session end.
  It does not buffer the entire file in memory; records are encoded and
  written incrementally.
- After session save, `DataWorkout` is owned by the history model and not
  retained by `WorkoutDialog` (it is passed via `std::move` or pointer
  transfer to the history-list owner).

### 6.2 Thread Safety for High-Frequency Sensor Data

MaximumTrainer uses a **single-threaded event loop** design:

- All BLE callbacks (`QLowEnergyController`, `QLowEnergyService`) are delivered
  on the thread that owns the controller — the main thread.
- `SimulatorHub` fires a `QTimer` on the main thread.
- `WorkoutDialog` processes signals on the main thread.
- **No shared mutable state is accessed from multiple threads**, so no mutex
  locking is needed in the hot path.

If a future change introduces a worker thread (e.g., for FIT file export):

- Use `QThread` + `QObject::moveToThread()` — never subclass `QThread`.
- All cross-thread communication must use **queued signal/slot connections**
  (automatic when objects live on different threads in Qt).
- Any shared data structure updated from a worker thread must be protected by
  `QMutex` / `QReadWriteLock`.
- **Never** call `QWidget` methods from a non-main thread.

### 6.3 BLE Reconnection & Stability

`BtleHub` implements automatic reconnection:

```
onControllerDisconnected()
        │
        ▼ start m_reconnectTimer (5 s)
        │
        ▼ connectToDevice() [re-entry, up to 3 attempts]
        │
   success ──► serviceDiscoveryFinished() ──► re-subscribe notifications
   failure ──► emit connectionError(...)   ──► WorkoutDialog informs user
```

- BLE operations that may stall emit `connectionError` after a configurable
  timeout (`QTimer` guard on `onDiscoveryFinished`).
- ERG commands (`setLoad` / `setSlope`) are **fire-and-forget**; they do not
  block.  If the device is disconnected, the command is silently dropped and
  the reconnect flow is already in progress.

### 6.4 WASM Single-Thread Constraints

- All WASM paths are compiled with Emscripten's **single-threaded** runtime
  (`Qt WASM singlethread`).
- No `QThread`, no `std::thread`, no POSIX threads.
- Asynchronous BLE operations (Web Bluetooth) are handled via
  `emscripten::val` callbacks marshalled back to the Qt event loop through
  `WebBluetoothBridge`.
- Large synchronous operations (e.g., FIT file creation) must be kept
  **< 16 ms** per invocation to avoid blocking the browser's main thread.
  If ever they exceed this, split with `QTimer::singleShot(0, …)` to yield.

---

*This document should be updated alongside any architectural change.
When adding a new hardware protocol, runtime target, or test tier, update the
relevant section and add an entry to the layer diagram.*
