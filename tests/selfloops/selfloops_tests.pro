###############################################################################
# tests/selfloops/selfloops_tests.pro
#
# Standalone Qt Test project for SelfloopsService.
#
# Build:
#   qmake selfloops_tests.pro && make
# Run:
#   ./../../build/tests/selfloops_tests -v2
#
# Optional live tests:
#   Set SELFLOOPS_EMAIL + SELFLOOPS_PW to run real-API upload tests.
###############################################################################

QT       += core network testlib
QT       -= gui

CONFIG   += qt c++17 console
CONFIG   -= app_bundle

TARGET   = selfloops_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

INCLUDEPATH += \
    . \
    ../../src/persistence/db \
    ../../src/app

# ── Logger (required by selfloops_service.cpp) ────────────────────────────────
SOURCES += ../../src/app/logger.cpp
HEADERS += ../../src/app/logger.h

# ── SelfloopsService sources ──────────────────────────────────────────────────
SOURCES += \
    ../../src/persistence/db/selfloops_service.cpp \
    tst_selfloops_service.cpp

HEADERS += \
    ../../src/persistence/db/selfloops_service.h
