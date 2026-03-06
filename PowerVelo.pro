#                                                                             #
#              ***   CORE CONFIGURATION  ***                                  #
#                                                                             #
###############################################################################

qtHaveModule(uitools):!embedded: QT += uitools
else: DEFINES += QT_NO_UITOOLS

# Used if you want console output on windows
#CONFIG += console

CONFIG += qwt qt thread
CONFIG += release
CONFIG += c++17

#INCLUDEPATH	+= /usr/lib/x86_64-linux-gnu/qt5

# include
QT       += core gui widgets
QT       += network printsupport
# Only add modules that exist in the current Qt installation; this makes the
# .pro file work for both full Qt builds and the WASM singlethread target
# (which lacks concurrent, bluetooth, and webenginewidgets).
qtHaveModule(concurrent):       QT += concurrent
qtHaveModule(bluetooth):        QT += bluetooth
qtHaveModule(webenginewidgets): QT += webenginewidgets

#QT += serialport
#QT += multimedia
#QT += multimediawidgets
#QT += webengine
#QT += webenginecore

# ─── WebAssembly overrides ───────────────────────────────────────────────────
# Qt 6.5+ names the singlethread WASM mkspec "wasm-emscripten-singlethread"
# (scope: wasm_emscripten_singlethread); older versions use "wasm-emscripten"
# (scope: wasm_emscripten).  contains(QMAKE_PLATFORM, wasm) covers both.
contains(QMAKE_PLATFORM, wasm) | wasm_emscripten | wasm_emscripten_singlethread {
    # Stubs make #include <QWebEngineView> etc. resolve to no-op classes
    INCLUDEPATH = $$PWD/src/ui/wasm_stubs $$INCLUDEPATH
    INCLUDEPATH += $$PWD/src/ui/wasm_stubs/QtWebEngineWidgets

    # Asyncify is required for the async WebBluetooth JS bridge
    QMAKE_LFLAGS += -s ASYNCIFY=1
    QMAKE_LFLAGS += "SHELL:-s ASYNCIFY_IMPORTS=['qt_asyncify_resume_js']"

    DEFINES += GC_WASM_BUILD
}
# ────────────────────────────────────────────────────────────────────────────

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
QMAKE_CFLAGS_ISYSTEM =



# to make sure we are toolchain neutral NEVER refer to a lib
# via file extensions .lib or .a in src.pro unless the section is
# platform specific. Instead we use directives -Ldir and -llib
win32 {
    #QWT is configured to build 2 libs (release/debug) on win32 (see qwtbuild.pri)
    INCLUDEPATH += ../qwt/include
    INCLUDEPATH += ../qwt/include/qwt

    CONFIG(release, debug|release){
        LIBS += -L$${PWD}/../qwt/lib -lqwt
    }
    CONFIG(debug, debug|release) {
        LIBS += -L$${PWD}/../qwt/lib -lqwtd
    }
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

    # Gdi32 and User32 are standard system libs resolved automatically by the
    # MSVC linker via the LIB environment variable.  No explicit -L path needed.
    LIBS += -lGdi32 -lUser32
    CONFIG += force_debug_info


} else {

    # gnu toolchain wants math libs
    LIBS += -lm

    unix:!macx:!wasm_emscripten {
        # Linux gcc 5 grumbles about unused static globals and leads
        # to a gazillion warnings that are harmless so lets remove them
        QMAKE_CXXFLAGS += -Wno-unused-variable

        # Linux Flex compiler grumbles about unsigned comparisons
        QMAKE_CXXFLAGS += -Wno-sign-compare

        LIBS += -lsfml-audio -lsfml-system -lVLCQtCore -lVLCQtWidgets
        DEFINES += GC_HAVE_VLCQT
    }
}

win32:!wasm_emscripten {
    # Windows: VLC-Qt and SFML paths (configure via qmake variables)
    !isEmpty(VLCQT_INSTALL) {
        INCLUDEPATH += $${VLCQT_INSTALL}/include
        LIBS += -L$${VLCQT_INSTALL}/lib -lVLCQtCore -lVLCQtWidgets
    }
    !isEmpty(SFML_INSTALL) {
        INCLUDEPATH += $${SFML_INSTALL}/include
        LIBS += -L$${SFML_INSTALL}/lib -lsfml-audio -lsfml-system
    }
}


#////////////////////////////////////////////////////////////////////////////////////////////////////////

macx:!wasm_emscripten {

    # VLC-Qt (optional; enable by passing VLCQT_INSTALL=... to qmake)
    !isEmpty(VLCQT_INSTALL) {
        DEFINES += GC_HAVE_VLCQT
        INCLUDEPATH += $${VLCQT_INSTALL}/include
        LIBS += -F$${VLCQT_INSTALL}/lib -framework VLCQtCore
        LIBS += -F$${VLCQT_INSTALL}/lib -framework VLCQtWidgets
    }

    # SFML (configure via SFML_INSTALL=...)
    !isEmpty(SFML_INSTALL) {
        INCLUDEPATH += $${SFML_INSTALL}/include
        LIBS += -L$${SFML_INSTALL}/lib -lsfml-audio -lsfml-system
    }

    # on mac we use native buttons and video, but have native fullscreen support
    LIBS    += -lobjc -framework IOKit -framework AppKit

    # QWT: configure directly against the flat (non-framework) install.
    !isEmpty(QWT_INSTALL) {
        INCLUDEPATH += $${QWT_INSTALL}/include
        LIBS += -L$${QWT_INSTALL}/lib -lqwt
        DEFINES += QWT_DLL
    }

}


# X11
if (defined(GC_WANT_X11)) {
    LIBS += -lX11
}



include(src/app/app.pri)
include(src/btle/btle.pri)
include(src/model/model.pri)
include(src/persistence/persistence.pri)
include(src/fitness/fitness.pri)
include(src/ui/ui.pri)
include(src/workout/workout.pri)


RESOURCES += MyResources.qrc
