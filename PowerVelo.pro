#                                                                             #
#              ***   CORE CONFIGURATION  ***                                  #
#                                                                             #
###############################################################################

!versionAtLeast(QT_VERSION, 5.12):error("Use at least Qt version 5.12")

qtHaveModule(uitools):!embedded: QT += uitools
else: DEFINES += QT_NO_UITOOLS

# Used if you want console output on windows
#CONFIG += console

CONFIG += qwt qt thread
CONFIG += release
CONFIG += c++11

#INCLUDEPATH	+= /usr/lib/x86_64-linux-gnu/qt5

# include
QT       += core gui widgets
QT       += network printsupport concurrent

#QT += serialport
#QT += multimedia
#QT += multimediawidgets
#QT += webengine
#QT += webenginecore
#QT += webenginewidgets


# For Release, disable QDebug for performance
DEFINES += QT_NO_DEBUG_OUTPUT

TARGET = MaximumTrainer
TEMPLATE = app

!isEmpty(APP_NAME) { TARGET = $${APP_NAME} }
CONFIG(debug, debug|release) { QMAKE_CXXFLAGS += -DGC_DEBUG }

qtHaveModule(uitools):!embedded: QT += uitools
else: DEFINES += QT_NO_UITOOLS


###=============================================================
### Obuild directory
###=============================================================

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
}
CONFIG(release, debug|release) {
    DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.u

###=============================================================
### OPTIONAL => VLC [Windows, Linux and OSX]
###=============================================================

!isEmpty(VLC_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(VLC_INCLUDE) { VLC_INCLUDE = $${VLC_INSTALL}/include }
    isEmpty(VLC_LIBS)    { VLC_LIBS    = -L$${VLC_INSTALL}/lib -lvlc }

    DEFINES     += GC_HAVE_VLC
    INCLUDEPATH += $${VLC_INCLUDE}
    LIBS        += $${VLC_LIBS}
}

###=======================================================================
### Directory Structure - Split into subdirs to be more manageable
###=======================================================================
INCLUDEPATH += ./src/core
QMAKE_CFLAGS_ISYSTEM =



# to make sure we are toolchain neutral NEVER refer to a lib
# via file extensions .lib or .a in src.pro unless the section is
# platform specific. Instead we use directives -Ldir and -llib
win32 {
    #QWT is configured to build 2 libs (release/debug) on win32 (see qwtbuild.pri)
    CONFIG(release, debug|release){
    LIBS += -L$${PWD}/../qwt/lib -lqwt
    }
    CONFIG(debug, debug|release) {
    LIBS += -L$${PWD}/../qwt/lib -lqwtd
    }

} else {
    #QWT is configured to build 1 lib for all other OS (see qwtbuild.pri)
    LIBS += -L$${PWD}/../qwt/lib -lqwt
}

# compress and math libs must be defined in gcconfig.pri
# if they're not part of the QT include
INCLUDEPATH += $${LIBZ_INCLUDE}
LIBS += $${LIBZ_LIBS}

# GNU Scientific Library
INCLUDEPATH += $${GSL_INCLUDES}
LIBS += $${GSL_LIBS}

###===============================
### PLATFORM SPECIFIC DEPENDENCIES
###===============================

# Microsoft Visual Studion toolchain dependencies
win32-msvc* {

    # we need windows kit 8.2 or higher with MSVC, offer default location
    isEmpty(WINKIT_INSTALL) WINKIT_INSTALL= "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64"
    LIBS += -L$${WINKIT_INSTALL} -lGdi32 -lUser32
    CONFIG += force_debug_info


} else {

    # gnu toolchain wants math libs
    LIBS += -lm

    unix:!macx {
        # Linux gcc 5 grumbles about unused static globals and leads
        # to a gazillion warnings that are harmless so lets remove them
        QMAKE_CXXFLAGS += -Wno-unused-variable

        # Linux Flex compiler grumbles about unsigned comparisons
        QMAKE_CXXFLAGS += -Wno-sign-compare
    }
}


#////////////////////////////////////////////////////////////////////////////////////////////////////////

macx {
    # Mac native widget support
    QT += macextras

    QMAKEFEATURES += /usr/local/qwt-5.12.9/features
    CONFIG += qwt
    INCLUDEPATH += /usr/local/qwt-5.12.9/lib/qwt.framework/Headers
    LIBS += -F/usr/local/qwt-5.12.9/lib -framework qwt

    # on mac we use native buttons and video, but have native fullscreen support
    LIBS    += -lobjc -framework IOKit -framework AppKit
    
}


# X11
if (defined(GC_WANT_X11)) {
    LIBS += -lX11
}



include (src/_db/_db.pri)
include (src/_subclassQT/_subclassQT.pri)
include (src/_subclassQWT/_subclassQWT.pri)
include (src/createWorkout/createWorkout.pri)
include (src/main/main.pri)
include (src/model/model.pri)
include (src/achievements/achievements.pri)
include (src/gui/gui.pri)
include (src/io_file/io_file.pri)
include (src/workout/workout.pri)
include (src/webBrowser/webBrowser.pri)

include (src/ANT/heart_rate/ANT_HeartRate.pri)
include (src/ANT/cadence/ANT_Cadence.pri)
include (src/ANT/speed/ANT_Speed.pri)
include (src/ANT/speed_cadence/ANT_SpeedCadence.pri)
include (src/ANT/power/ANT_Power.pri)
include (src/ANT/fec/ANT_fec.pri)
include (src/ANT/oxygen/ANT_Oxygen.pri)
include (src/ANT/ANT.pri)
include (src/Fit_20_16/Fit.pri)


RESOURCES += MyResources.qrc
