###############################################################################
# tests/intervals_icu/intervals_icu_tests.pro
#
# Standalone Qt Test project for:
#   • IntervalsIcuService (static HTTP-request builder)
#   • ImporterWorkoutZwo  (ZWO XML parser)
#
# Depends only on Qt Core + Qt Network + Qt Test — no GUI, Bluetooth, or QWT.
# A local util.h stub (tests/intervals_icu/util.h) shadows the real util.h so
# that workout.cpp and interval.cpp compile without pulling in QWT or Qt Gui.
#
# Build:
#   qmake intervals_icu_tests.pro && make
# Run:
#   ./build/tests/intervals_icu_tests -v2
###############################################################################

QT       += core network testlib xml
QT       -= gui

CONFIG   += qt c++17 console
CONFIG   -= app_bundle

TARGET   = intervals_icu_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

# ── Path to fixture files (made available via FIXTURES_DIR define) ────────────
FIXTURES_DIR = $$PWD/../fixtures
DEFINES += FIXTURES_DIR=\\\"$$FIXTURES_DIR\\\"

# ── INCLUDEPATH — local stub dir MUST come before model/ so that the test's
#    util.h shadows the real one (which includes QWT). ─────────────────────────
INCLUDEPATH += \
    . \
    ../../src/persistence/db \
    ../../src/persistence/file \
    ../../src/model \
    ../../src/app

# ── IntervalsIcuService sources ───────────────────────────────────────────────
SOURCES += \
    ../../src/persistence/db/intervals_icu_service.cpp \
    tst_intervals_icu_service.cpp

HEADERS += \
    ../../src/persistence/db/intervals_icu_service.h

# ── ImporterWorkoutZwo + model sources (util.h is shadowed by test stub) ──────
SOURCES += \
    ../../src/persistence/file/importerworkoutzwo.cpp \
    ../../src/model/workout.cpp \
    ../../src/model/interval.cpp \
    ../../src/model/account.cpp \
    ../../src/model/repeatdata.cpp \
    tst_importer_workout_zwo.cpp

HEADERS += \
    ../../src/persistence/file/importerworkoutzwo.h \
    ../../src/model/workout.h \
    ../../src/model/interval.h \
    ../../src/model/account.h \
    ../../src/model/repeatdata.h \
    util.h

