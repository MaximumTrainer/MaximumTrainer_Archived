###############################################################################
# tests/persistence/persistence_tests.pro
#
# Standalone Qt Test project for the LocalDatabase class.
# Depends only on Qt Core + Qt Sql + Qt Test — no GUI, Bluetooth, or QWT.
#
# Build:
#   qmake persistence_tests.pro && make
# Run:
#   ./build/tests/persistence_tests -v2
###############################################################################

QT       += core sql testlib
QT       -= gui

CONFIG   += qt c++17 console
CONFIG   -= app_bundle

TARGET   = persistence_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

# ── LocalDatabase sources ─────────────────────────────────────────────────────
INCLUDEPATH += ../../src/persistence/db
INCLUDEPATH += ../../src/model
INCLUDEPATH += ../../src/app

SOURCES += \
    ../../src/persistence/db/localdatabase.cpp \
    ../../src/model/account.cpp \
    ../../src/model/sensor.cpp \
    ../../src/model/powercurve.cpp \
    tst_localdatabase.cpp

HEADERS += \
    ../../src/persistence/db/localdatabase.h \
    ../../src/model/account.h \
    ../../src/model/powercurve.h \
    ../../src/model/sensor.h
