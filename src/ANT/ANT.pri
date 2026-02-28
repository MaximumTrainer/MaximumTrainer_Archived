INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD


SOURCES     += \
    src/ANT/hub.cpp \
    $$PWD/antmsg.cpp \
    $$PWD/ant_controller.cpp \
    $$PWD/kickrant.cpp

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
    INCLUDEPATH += $$PWD/libs/inc \
    INCLUDEPATH += $$PWD/libs/common \
    INCLUDEPATH += $$PWD/libs/libraries \
    INCLUDEPATH += $$PWD/libs/software/ANTFS \
    INCLUDEPATH += $$PWD/libs/software/serial \
    INCLUDEPATH += $$PWD/libs/software/serial/device_management \
    INCLUDEPATH += $$PWD/libs/software/system \
    INCLUDEPATH += $$PWD/libs/software/USB \
    INCLUDEPATH += $$PWD/libs/software/USB/device_handles \
    INCLUDEPATH += $$PWD/libs/software/USB/devices \


#////////////////////////////////////////////////////////////////////////////////////////////////////////
win32 {

    LIBS += -lUser32 -lAdvAPI32

    QMAKE_CXXFLAGS += /wd4996

    SOURCES += \
        $$PWD/libs/software/ANTFS/antfs_client_channel.cpp \
        $$PWD/libs/software/ANTFS/antfs_host.cpp \
        $$PWD/libs/software/ANTFS/antfs_host_channel.cpp \
        $$PWD/libs/software/ANTFS/antfs_directory.c \
        $$PWD/libs/common/checksum.c \
        $$PWD/libs/common/crc.c \
        $$PWD/libs/software/serial/device_management/dsi_ant_device.cpp \
        $$PWD/libs/libraries/dsi_cm_library.cpp \
        $$PWD/libs/software/serial/device_management/dsi_ant_device_polling.cpp \
        $$PWD/libs/software/system/dsi_convert.c \
        $$PWD/libs/software/system/dsi_debug.cpp \
        $$PWD/libs/software/serial/dsi_framer.cpp \
        $$PWD/libs/software/serial/dsi_framer_ant.cpp \
        $$PWD/libs/software/serial/dsi_framer_integrated_antfs_client.cpp \
        $$PWD/libs/libraries/dsi_libusb_library.cpp \
        $$PWD/libs/software/serial/dsi_serial.cpp \
        $$PWD/libs/software/serial/dsi_serial_generic.cpp \
        $$PWD/libs/software/serial/dsi_serial_libusb.cpp \
        $$PWD/libs/software/serial/dsi_serial_si.cpp \
        $$PWD/libs/software/serial/dsi_serial_vcp.cpp \
        $$PWD/libs/libraries/dsi_silabs_library.cpp \
        $$PWD/libs/software/system/dsi_thread_win32.c \
        $$PWD/libs/software/system/dsi_timer.cpp \
        $$PWD/libs/software/system/macros.c \
        $$PWD/libs/software/USB/devices/usb_device.cpp \
        $$PWD/libs/software/USB/device_handles/usb_device_handle_libusb.cpp \
        $$PWD/libs/software/USB/device_handles/usb_device_handle_si.cpp \
        $$PWD/libs/software/USB/device_handles/usb_device_handle_win.cpp \
        $$PWD/libs/software/USB/devices/usb_device_libusb.cpp \
        $$PWD/libs/software/USB/devices/usb_device_si.cpp \
        $$PWD/libs/software/serial/WinDevice.cpp

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
