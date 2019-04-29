INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD



SOURCES     +=\
    ANT/hub.cpp \
    $$PWD/antmsg.cpp \
    $$PWD/ant_controller.cpp \
    $$PWD/kickrant.cpp

HEADERS     += \
    ANT/hub.h \
    ANT/common_pages.h \
    ANT/antplus.h \
    $$PWD/antmsg.h \
    $$PWD/antdefine.h \
    $$PWD/ant_controller.h \
    $$PWD/kickrant.h \



#////////////////////////////////////////////////////////////////////////////////////////////////////////
win32 {

    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\inc
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\common
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\libraries
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\ANTFS
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\serial
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\serial\device_management
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\system
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\USB
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\USB\device_handles
    INCLUDEPATH += C:\Dropbox\ANT-SDK_PC.3.5\ANT_LIB\software\USB\devices


#    LIBS +=  "C:\Program Files (x86)\Windows Kits\8.0\Lib\win8\um\x86\User32.lib"
#    LIBS +=  "C:\Program Files (x86)\Windows Kits\8.0\Lib\win8\um\x86\AdvAPI32.lib"

   LIBS +=  "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.16299.0\um\x86\User32.lib"
   LIBS +=  "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.16299.0\um\x86\AdvAPI32.lib"


    #static ANT_LIB builded with ANT_LIB project (Download source from thisisant.com, build with VisualStudio)
    #Also include the DLL inside /Release
    #LIBS +=  "C:\Dropbox\MSVC2012\ANT_Lib.lib"
    #LIBS +=  "C:\Dropbox\MSVC2013\ANT_Lib.lib"
    LIBS +=  "C:\Dropbox\MSVC2017\ANT_Lib.lib"
#TODO: rebuild with MSVC2017 HERE




}

#////////////////////////////////////////////////////////////////////////////////////////////////////////



macx {

    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/common
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/inc
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/ANTFS
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/serial
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/serial/device_management
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/system
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/USB
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/USB/device_handles
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/USB/devices
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/ANT_LIB/software/USB/iokit_driver


    #static ANT_LIB builded with ANT_LIB project (Download source from thisisant.com, build with XCode)
    LIBS += /Users/$${MAC_USERNAME}/Dropbox/ANT-SDK_Mac.3.5/bin/libantbase.a



}






