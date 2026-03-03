INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += src/main/main.cpp\
    src/main/util.cpp \
    src/main/soundplayer.cpp \
    src/main/globalvars.cpp \
    $$PWD/simplecrypt.cpp \
    $$PWD/reportutil.cpp \

contains(DEFINES, GC_HAVE_VLCQT) {
    SOURCES += $$PWD/myvlcplayer.cpp
    HEADERS += $$PWD/myvlcplayer.h
}




HEADERS += src/main/util.h \
    src/main/soundplayer.h \
    src/main/globalvars.h \
    $$PWD/simplecrypt.h \
    $$PWD/reportutil.h \
    $$PWD/myconstants.h



macx {

    HEADERS += src/main/macutils.h \

    #Call to native objective-c for mac
    OBJECTIVE_SOURCES += src/main/MacUtils.mm \
}



