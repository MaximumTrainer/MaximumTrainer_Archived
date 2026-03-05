INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += $$PWD/main.cpp\
    $$PWD/util.cpp \
    $$PWD/soundplayer.cpp \
    $$PWD/globalvars.cpp \
    $$PWD/simplecrypt.cpp \
    $$PWD/reportutil.cpp \

contains(DEFINES, GC_HAVE_VLCQT) {
    SOURCES += $$PWD/myvlcplayer.cpp
}
# Always include header so MOC generates the meta-object / vtable for the stub class
HEADERS += $$PWD/myvlcplayer.h




HEADERS += $$PWD/util.h \
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



