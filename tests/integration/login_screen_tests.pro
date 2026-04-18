###############################################################################
# tests/integration/login_screen_tests.pro
#
# Login Screen Full-Application Test -- MaximumTrainer
#
# Validates that a user can log into the MaximumTrainer application and
# access key functionality without error, across Windows, macOS, and Linux:
#
#   1. Offline login path: exercises the offline-mode account setup logic
#      end-to-end, then starts SimulatorHub to confirm key functionality
#      (sensor data) is accessible once logged in.
#   2. Intervals.icu OAuth URL validation: builds the OAuth2 authorization
#      URL via Environnement and verifies all required parameters are present.
#   3. Intervals.icu API login: makes a real HTTPS call to the intervals.icu
#      API using credentials from environment variables (QSKIP when absent).
#
# A labelled 1280×720 screenshot is saved as build evidence for each test.
#
# Build (Linux / macOS):
#   qmake login_screen_tests.pro && make
# Build (Windows -- MSVC developer prompt):
#   qmake login_screen_tests.pro && nmake
#
# Run headless (Linux CI):
#   Xvfb :99 -screen 0 1280x800x24 & export DISPLAY=:99
#   ../../build/tests/login_screen_tests -v2
# Run directly (Windows / macOS CI -- display is always available):
#   .\build\tests\login_screen_tests.exe -v2
###############################################################################

QT       += core gui widgets network testlib
CONFIG   += qt c++17
CONFIG   -= app_bundle

TARGET   = login_screen_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

INCLUDEPATH += \
    ../../src/btle \
    ../../src/persistence/db

SOURCES += \
    ../../src/btle/simulator_hub.cpp \
    tst_login_screen.cpp

HEADERS += \
    ../../src/btle/simulator_hub.h
