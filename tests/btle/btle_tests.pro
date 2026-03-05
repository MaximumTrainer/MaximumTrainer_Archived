###############################################################################
# tests/btle/btle_tests.pro
#
# Standalone Qt Test project for BtleHub (no GUI, no ANT+, no VLC-Qt, no QWT).
# Depends only on: Qt Core + Qt Bluetooth + Qt Test.
#
# Build:
#   qmake btle_tests.pro && make
# Run:
#   ./btle_tests -v2
###############################################################################

QT       += core bluetooth testlib
QT       -= gui

CONFIG   += qt c++11 console
CONFIG   -= app_bundle

TARGET   = btle_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

# ── BtleHub sources ──────────────────────────────────────────────────────────
INCLUDEPATH += ../../src/BTLE

SOURCES += \
    ../../src/BTLE/btle_hub.cpp \
    ../../src/BTLE/simulator_hub.cpp \
    tst_btle_hub.cpp

HEADERS += \
    ../../src/BTLE/btle_hub.h \
    ../../src/BTLE/simulator_hub.h \
    btle_device_simulator.h
