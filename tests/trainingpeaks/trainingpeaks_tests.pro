###############################################################################
# tests/trainingpeaks/trainingpeaks_tests.pro
#
# Standalone Qt Test project for TrainingPeaksService.
#
# Build:
#   qmake trainingpeaks_tests.pro && make
# Run:
#   ./../../build/tests/trainingpeaks_tests -v2
#
# Optional live tests:
#   Set TP_ACCESS_TOKEN + TP_REFRESH_TOKEN env vars to run real-API tests.
###############################################################################

QT       += core network testlib
QT       -= gui

CONFIG   += qt c++17 console
CONFIG   -= app_bundle

TARGET   = trainingpeaks_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

# TP_CLIENT_SECRET must match the value used in the main build.
# For CI, supply via environment; for local dev export before running qmake.
TP_SECRET_VAL = $$(TP_CLIENT_SECRET)
!isEmpty(TP_SECRET_VAL) {
    DEFINES += TP_CLIENT_SECRET=\\\"$$TP_SECRET_VAL\\\"
} else {
    DEFINES += TP_CLIENT_SECRET=\\\"\\\"
}

INCLUDEPATH += \
    . \
    ../../src/persistence/db \
    ../../src/app

# ── Logger ────────────────────────────────────────────────────────────────────
SOURCES += \
    ../../src/app/logger.cpp

HEADERS += \
    ../../src/app/logger.h

# ── TrainingPeaksService sources ──────────────────────────────────────────────
SOURCES += \
    ../../src/persistence/db/trainingpeaks_service.cpp \
    tst_trainingpeaks_service.cpp

HEADERS += \
    ../../src/persistence/db/trainingpeaks_service.h \
    ../../src/persistence/db/environnement.h
