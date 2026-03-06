INCLUDEPATH += $$PWD

# On Wasm use the WebBluetooth bridge; on all other platforms use native Qt BLE
contains(QMAKE_PLATFORM, wasm) {
    SOURCES += \
        $$PWD/btle_hub_wasm.cpp \
        $$PWD/webbluetooth_bridge.cpp \
        $$PWD/btle_scanner_dialog.cpp \
        $$PWD/simulator_hub.cpp

    HEADERS += \
        $$PWD/btle_hub_wasm.h \
        $$PWD/webbluetooth_bridge.h \
        $$PWD/btle_scanner_dialog.h \
        $$PWD/btle_uuids.h \
        $$PWD/simulator_hub.h
} else {
    SOURCES += \
        $$PWD/btle_hub.cpp \
        $$PWD/btle_scanner_dialog.cpp \
        $$PWD/simulator_hub.cpp

    HEADERS += \
        $$PWD/btle_hub.h \
        $$PWD/btle_scanner_dialog.h \
        $$PWD/btle_uuids.h \
        $$PWD/simulator_hub.h
}

FORMS += \
    $$PWD/btle_scanner_dialog.ui
