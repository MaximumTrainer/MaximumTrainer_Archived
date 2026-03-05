###############################################################################
# tests/integration/btle_integration_tests.pro
#
# BTLE integration test – runs SimulatorHub in a Qt window (Xvfb on CI) and
# takes a screenshot as evidence that sensor data flows through the BTLE layer.
#
# Build:
#   qmake btle_integration_tests.pro && make
# Run (headless):
#   Xvfb :99 -screen 0 1280x800x24 & export DISPLAY=:99
#   ../../build/tests/btle_integration_tests -v2
###############################################################################

QT       += core gui widgets bluetooth testlib
CONFIG   += qt c++11
CONFIG   -= app_bundle

TARGET   = btle_integration_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

INCLUDEPATH += ../../src/btle

SOURCES += \
    ../../src/btle/simulator_hub.cpp \
    tst_btle_integration.cpp

HEADERS += \
    ../../src/btle/simulator_hub.h
