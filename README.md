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

See instructions in `PowerVelo.pro`. Needs to better handle dependencies; used to be built on the same machine.

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
