INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD


SOURCES     += \
    $$PWD/antmsg.cpp \
    $$PWD/ant_controller.cpp \
    $$PWD/kickrant.cpp

# hub.cpp uses DSIFramerANT/DSISerialGeneric/DSIThread which are not available
# on Windows (ANT USB replaced by BTLE).  Use a no-op stub.
!win32 {
    SOURCES += src/ANT/hub.cpp
}
win32 {
    SOURCES += $$PWD/hub_win_stub.cpp
}

HEADERS     += \
    src/ANT/hub.h \
    src/ANT/common_pages.h \
    src/ANT/antplus.h \
    $$PWD/antmsg.h \
    $$PWD/antdefine.h \
    $$PWD/ant_controller.h \
    $$PWD/kickrant.h \


#////////////////////////////////////////////////////////////////////////////////////////////////////////
# ANT LIBS - Download source from thisisant.com
INCLUDEPATH += $$PWD/libs/inc
INCLUDEPATH += $$PWD/libs/common
INCLUDEPATH += $$PWD/libs/libraries
INCLUDEPATH += $$PWD/libs/software/ANTFS
INCLUDEPATH += $$PWD/libs/software/serial
INCLUDEPATH += $$PWD/libs/software/serial/device_management
INCLUDEPATH += $$PWD/libs/software/system
INCLUDEPATH += $$PWD/libs/software/USB
INCLUDEPATH += $$PWD/libs/software/USB/device_handles
INCLUDEPATH += $$PWD/libs/software/USB/devices


#////////////////////////////////////////////////////////////////////////////////////////////////////////
win32 {

    LIBS += -lUser32 -lAdvAPI32

    QMAKE_CXXFLAGS += /wd4996

    # ANT USB library sources are stubbed out on Windows (replaced by BTLE).
    # hub_win_stub.cpp (above) provides no-op Hub implementations.

}

#////////////////////////////////////////////////////////////////////////////////////////////////////////

unix:!macx {
    LIBS += $$PWD/libs/linux/libANT_LIB.a -lusb-1.0
}
macx {

    INCLUDEPATH += $$PWD/libs/software/USB/iokit_driver


    #static ANT_LIB builded with ANT_LIB project (Download source from thisisant.com, build with XCode)
    LIBS += $$PWD/libs/mac/libantbase.a

}
