INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += $$PWD/main.cpp\
    $$PWD/logger.cpp \
    $$PWD/util.cpp \
    $$PWD/globalvars.cpp \
    $$PWD/simplecrypt.cpp \
    $$PWD/reportutil.cpp \

# SoundPlayer: full SFML implementation on native, no-op stub on Wasm
contains(QMAKE_PLATFORM, wasm) {
    SOURCES += $$PWD/soundplayer_wasm.cpp
} else {
    SOURCES += $$PWD/soundplayer.cpp
}
contains(DEFINES, GC_HAVE_VLCQT) {
    SOURCES += $$PWD/myvlcplayer.cpp
}
# Always include header so MOC generates the meta-object / vtable for the stub class
HEADERS += $$PWD/myvlcplayer.h




HEADERS += $$PWD/util.h \
    $$PWD/logger.h \
    $$PWD/soundplayer.h \
    $$PWD/globalvars.h \
    $$PWD/simplecrypt.h \
    $$PWD/reportutil.h \
    $$PWD/myconstants.h



macx {

    HEADERS += $$PWD/macutils.h \

    #Call to native objective-c for mac
    OBJECTIVE_SOURCES += $$PWD/MacUtils.mm \
}



