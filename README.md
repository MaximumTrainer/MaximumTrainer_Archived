# MaximumTrainer
Qt software for indoor cycling, similar to trainerroad, with the ability to get plans from trainerday.com.
Old Qt App under renovation for fun with Github Copilot.
Connects to cycle trainers and sensors exclusively via **Bluetooth Low Energy (BTLE)**.

## Sensor Connection

MaximumTrainer uses **Bluetooth Low Energy (BTLE) only** to connect to cycle trainers and sensors on all platforms (Windows, Linux, macOS).

When starting a workout, a Bluetooth device scanner dialog appears. Select your trainer or sensor from the list of discovered BTLE devices to begin. Supported sensor profiles:

| Profile | UUID | Data |
|---------|------|------|
| Heart Rate Monitor | 0x180D | Heart rate (bpm) |
| Cycling Speed & Cadence | 0x1816 | Speed (km/h), cadence (RPM) |
| Cycling Power | 0x1818 | Power (W) |
| Fitness Machine (FTMS) | 0x1826 | Speed, cadence, power + trainer control |

## Building
Build with the `.pro` file inside the project & GitHub Actions CI.

### Dependencies

| Dependency | Version | Notes |
|------------|---------|-------|
| Qt | 5.15.2 (Win/Linux) / 6.7.3 (macOS) | Core framework |
| QWT | 6.2.0 (Win/Linux) / 6.3.0 (macOS) | Plotting widgets |
| VLC-Qt | 1.1.0 (Win) / built from source (Linux/macOS) | Video playback |
| SFML | 2.6.1 (Win) / system (Linux/macOS) | Audio |

### Windows

**Required tools:** Qt 5.15.2 (msvc2019_64), Visual Studio 2019+ (MSVC), 7-Zip

**Environment variables** — set these before running `qmake`:

| Variable | Description | Example |
|----------|-------------|---------|
| `QTDIR` | Qt installation root for the target arch | `C:\Qt\5.15.2\msvc2019_64` |
| `VLCQT_DIR` | VLC-Qt installation root (forward slashes) | `C:/vlcqt` |
| `QMAKEFEATURES` | Path to QWT features directory | `C:\qwt\features` |

**qmake invocation:**
```powershell
$env:QMAKEFEATURES = "C:\qwt\features"
qmake PowerVelo.pro `
  "VLCQT_INSTALL=C:/vlcqt" `
  "SFML_INSTALL=C:/sfml/SFML-2.6.1"
```

> `VLCQT_INSTALL` must point to the VLC-Qt package root (containing `bin/`, `include/`, `lib/`). The pre-built 1.1.0 win64 MSVC package places both headers and import `.lib` files inside `include/`.  
> `SFML_INSTALL` must point to the SFML root (the directory containing `include/` and `lib/`).  
> The Windows Kit libraries (`Gdi32`, `User32`) are resolved automatically by the MSVC linker — no `WINKIT_INSTALL` is needed when building with a proper MSVC developer command prompt.

**Download links:**
- Qt 5.15.2: https://www.qt.io/download
- VLC-Qt 1.1.0 (win64 MSVC): https://github.com/vlc-qt/vlc-qt/releases/tag/1.1.0
- SFML 2.6.1 (vc17 64-bit): https://github.com/SFML/SFML/releases/tag/2.6.1
- QWT 6.2.0: https://sourceforge.net/projects/qwt/files/qwt/6.2.0/

### Linux (Ubuntu 22.04)

Install system packages then build:
```bash
sudo apt-get install -y qtbase5-dev qtwebengine5-dev libqt5bluetooth5-dev \
  libsfml-dev libqwt-qt5-dev
qmake PowerVelo.pro
make -j$(nproc)
```

### macOS

Uses Qt 6.7.3 (install via `brew install qt@6` or `install-qt-action`). QWT must be built from source against Qt6 (brew's QWT uses Qt6). VLC-Qt is built from source with `-DQT_VERSION=6`.

## Testing

BTLE sensor parsing is validated by a headless Qt Test suite in `tests/btle/` — no real hardware required. Tests run on Ubuntu, Windows and macOS in CI.

### Running the tests

```bash
cd tests/btle
qmake btle_tests.pro
make -j$(nproc)
../../build/tests/btle_tests -v2
```

### Test output (Ubuntu 24.04, Qt 5.15.13)

![BTLE tests — 28 passed, 0 failed](https://github.com/user-attachments/assets/62d82bce-1dd0-40c4-91fe-159ee6aeb8b9)

## Language
Language files are used at runtime in the `/language` folder.
You need Qt Linguist to open and generate a new language file (`.qm` file).

## TODO
Project now going through new revisions, with plans to enhance as open source interval trainer for indoor cycling & indoor rowing.
