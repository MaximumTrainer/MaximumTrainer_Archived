INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += main/main.cpp\
    main/util.cpp \
    main/soundplayer.cpp \
    main/globalvars.cpp \
    $$PWD/simplecrypt.cpp \
    $$PWD/reportutil.cpp \
    $$PWD/myvlcplayer.cpp \




HEADERS += main/util.h \
    main/soundplayer.h \
    main/globalvars.h \
    $$PWD/simplecrypt.h \
    $$PWD/reportutil.h \
    $$PWD/myvlcplayer.h \
    $$PWD/myconstants.h



macx {

    HEADERS += main/macutils.h \

    #Call to native objective-c for mac
    OBJECTIVE_SOURCES += main/MacUtils.mm \
}



