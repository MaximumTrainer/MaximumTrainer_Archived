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

### Test output (Ubuntu 22.04, Qt 5.15)

```
********* Start testing of TstBtleHub *********
Config: Using QtTest library 5.15.13, Qt 5.15.13 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 13.2.0), Ubuntu 24.04
PASS   : TstBtleHub::initTestCase()
PASS   : TstBtleHub::testHr_uint8()
PASS   : TstBtleHub::testHr_uint16()
PASS   : TstBtleHub::testHr_uint8WithRR()
PASS   : TstBtleHub::testHr_zero()
PASS   : TstBtleHub::testHr_maxBpm()
PASS   : TstBtleHub::testCsc_firstMeasurementDiscarded()
PASS   : TstBtleHub::testCsc_crankOnly()
PASS   : TstBtleHub::testCsc_wheelOnly()
PASS   : TstBtleHub::testCsc_combined()
PASS   : TstBtleHub::testCsc_uint16Rollover()
PASS   : TstBtleHub::testCsc_standstill()
PASS   : TstBtleHub::testPower_positive()
PASS   : TstBtleHub::testPower_zero()
PASS   : TstBtleHub::testPower_negative()
PASS   : TstBtleHub::testPower_tooShort_ignored()
PASS   : TstBtleHub::testFtms_speedOnly()
PASS   : TstBtleHub::testFtms_cadenceOnly()
PASS   : TstBtleHub::testFtms_powerOnly()
PASS   : TstBtleHub::testFtms_allThree()
PASS   : TstBtleHub::testFtms_zeroValues()
PASS   : TstBtleHub::testFtms_tooShort_ignored()
PASS   : TstBtleHub::testEliteTrainer_singlePacket()
PASS   : TstBtleHub::testEliteTrainer_sequence()
PASS   : TstBtleHub::testWahooKickr_powerAndCsc()
PASS   : TstBtleHub::testWahooKickr_cscRollover()
PASS   : TstBtleHub::testGarminTacx_ftmsPlusCsc()
PASS   : TstBtleHub::cleanupTestCase()
Totals: 28 passed, 0 failed, 0 skipped, 0 blacklisted, 2ms
********* Finished testing of TstBtleHub *********
```

## Language
Language files are used at runtime in the `/language` folder.
You need Qt Linguist to open and generate a new language file (`.qm` file).

## TODO
Project now going through new revisions, with plans to enhance as open source interval trainer for indoor cycling & indoor rowing.
