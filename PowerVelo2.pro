QT       += core gui widgets
QT       += webenginewidgets
QT       += network printsupport concurrent



TARGET = MaximumTrainer
TEMPLATE = app


qtHaveModule(uitools):!embedded: QT += uitools
else: DEFINES += QT_NO_UITOOLS


# Used if you want console output on windows
#CONFIG += console
#CONFIG += qwt qt thread
CONFIG += release
CONFIG += c++11

# For Release, disable QDebug for performance
DEFINES += QT_NO_DEBUG_OUTPUT



#////////////////////////////////////////////////////////////////////////////////////////////////////////
#---- how to deploy ---
# cmd Visual studio 2017 (C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Visual Studio 2015\Visual Studio Tools\Windows Desktop Command Prompts)
#D:\qt5.11.0\bin\windeployqt.exe C:\Users\Calo\Desktop\MT\build-PowerVelo2-Qt_5_11_rc1_msvc2017-Release\release\MaximumTrainer.exe

# add missing .dll

#to test video player:
#https://shaka-player-demo.appspot.com/demo/#asset=//storage.googleapis.com/shaka-demo-assets/angel-one/dash.mpd;lang=en-CA

win32 {
    RC_FILE = myapp.rc
    DEFINES += NOMINMAX


    #--------- VLC-QT: https://github.com/vlc-qt/vlc-qt -
    LIBS        += -lVLCQtCore -lVLCQtWidgets
    LIBS        += -LC:\Dropbox\MSVC2017\VLC-Qt-1.2.0-9b2f561\lib
    INCLUDEPATH += C:\Dropbox\MSVC2017\VLC-Qt-1.2.0-9b2f561\include
    #To deploy, copy vlc-qt DLL and libvlc.dll, libvlccore.dll


    #--------- SFML
    LIBS += -LC:/Dropbox/MSVC2017/SFML-2.5.0/lib

    CONFIG(release, debug|release): LIBS += -lsfml-audio -lsfml-system

    INCLUDEPATH += C:/Dropbox/MSVC2017/SFML-2.5.0/include
    DEPENDPATH += C:/Dropbox/MSVC2017/SFML-2.5.0/include


    #--------- QWT
    INCLUDEPATH += C:/Dropbox/qwt-6.1.3/src
    include ( C:/Dropbox/qwt-6.1.3/qwt.prf )

}
#////////////////////////////////////////////////////////////////////////////////////////////////////////
macx {
#how to deploy see down

    MAC_USERNAME = mblais2

    #set RPATH (place to look for .dylib & framework by default)
    QMAKE_RPATHDIR += @executable_path/../Frameworks
    QMAKE_RPATHDIR += @executable_path/lib
    QMAKE_RPATHDIR += @executable_path


    ICON = myappico.icns
    CONFIG += x86_64
    CONFIG -= i386

    #OSX Libs
    LIBS += -framework IOKit
    LIBS += -framework CoreFoundation


    #VLC-QT 3.0.2  ----------------
    QMAKE_LFLAGS += -F/Users/$${MAC_USERNAME}/Dropbox/vlc-qt-1.2.0-9b2f561-macos/Frameworks
    LIBS += -framework VLCQtCore
    LIBS += -framework VLCQtWidgets
    LIBS += -framework VLCQtQml
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/vlc-qt-1.2.0-9b2f561-macos/include


    #SFML ----------------
    LIBS += -L"/usr/local/lib"

    CONFIG(release, debug|release): LIBS += -lsfml-audio -lsfml-graphics -lsfml-system -lsfml-network -lsfml-window
    CONFIG(debug, debug|release): LIBS += -lsfml-audio -lsfml-graphics -lsfml-system -lsfml-network -lsfml-window

    INCLUDEPATH += "/usr/local/include"
    DEPENDPATH += "/usr/local/include"

    #QWT -----------------------
    LIBS += -F/usr/local/qwt-6.1.3/lib -framework qwt
    INCLUDEPATH += /Users/$${MAC_USERNAME}/Dropbox/qwt-6.1.3/src
    DEPENDPATH += /Users/$${MAC_USERNAME}/Dropbox/qwt-6.1.3/src
    #include ( /Users/$${MAC_USERNAME}/Dropbox/qwt-6.1.3/qwt.prf )

    # ----- DEPLOY-- Uncomment down here
    #Copy dylib necessary  VLC Libs (lib & plugins) and SFML libs that are forgotten by MacDeployQt
    copydata.commands = $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/SFML-2.5.1-macos-clang/lib $$OUT_PWD/MaximumTrainer.app/Contents/MacOS &&
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/SFML-2.5.1-macos-clang/extlibs $$OUT_PWD/MaximumTrainer.app/Contents/Frameworks &&

#     widevine (webengine support for netflix)
#    need the same version as chromium, get old version here: https://www.slimjet.com/chrome/google-chrome-old-version.php
#    mac copy it from /Applications/Google Chrome.app/Contents/Versions/72.0.3626.96/Google Chrome Framework.framework/Versions/A/Libraries/WidevineCdm/_platform_specific/mac_x64/libwidevinecdm.dylib
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/mac_dylib/widevine_72.0.3626.121/WidevineCdm $$OUT_PWD/MaximumTrainer.app/Contents/MacOS &&
#    # qwt framework
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/mac_dylib/qwt/Frameworks $$OUT_PWD/MaximumTrainer.app/Contents &&

    #Missing qt libs
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/qt_5_12_2_libs/Frameworks $$OUT_PWD/MaximumTrainer.app/Contents &&


#    #VLC 3.0.2
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/vlc-qt-1.2.0-9b2f561-macos/libvlc-processed/vlc/plugins $$OUT_PWD/MaximumTrainer.app/Contents/MacOS &&
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/vlc-qt-1.2.0-9b2f561-macos/libvlc-processed/lib $$OUT_PWD/MaximumTrainer.app/Contents/MacOS &&
    copydata.commands += $(COPY_DIR) /Users/$${MAC_USERNAME}/Dropbox/vlc-qt-1.2.0-9b2f561-macos/Frameworks $$OUT_PWD/MaximumTrainer.app/Contents


    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata



    # To run after build:
    # /Users/mblais2/DEV2/v5.12.2/bin/macdeployqt /Users/mblais2/DEV2/build-PowerVelo2-5_12_1-Release/MaximumTrainer.app

    # fix qwt (not always needed to run)
    # install_name_tool -change qwt.framework/Versions/6/qwt @executable_path/../Frameworks/qwt.framework/Versions/6/qwt /Users/mblais2/DEV2/build-PowerVelo2-5_12_1-Release/MaximumTrainer.app/Contents/MacOS/MaximumTrainer

    #Create dmg file
    # hdiutil create /Users/mblais2/DEV2/MT.dmg -volname "MaximumTrainer" -fs HFS+ -srcfolder "/Users/mblais2/DEV2/build-PowerVelo2-5_12_1-Release/MaximumTrainer.app"
    # hdiutil convert /Users/mblais2/DEV2/MT.dmg -format UDZO -o /Users/mblais2/DEV2/MaximumTrainer.dmg

    # add license
#    /Users/mblais2/Dropbox/mac_deploy/licenseDMG.py /Users/mblais2/DEV2/MaximumTrainer.dmg /Users/mblais2/Dropbox/mac_deploy/macLicense.txt



#To fix dark mode on 10.14
#add this to your Info.plist:
#    <key>NSRequiresAquaSystemAppearance</key>
#    <true/>

}





include (_db/_db.pri)
include (_subclassQT/_subclassQT.pri)
include (_subclassQWT/_subclassQWT.pri)
include (createWorkout/createWorkout.pri)
include (main/main.pri)
include (model/model.pri)
include (achievements/achievements.pri)
include (gui/gui.pri)
include (io_file/io_file.pri)
include (workout/workout.pri)
include (webBrowser/webBrowser.pri)


include (ANT/heart_rate/ANT_HeartRate.pri)
include (ANT/cadence/ANT_Cadence.pri)
include (ANT/speed/ANT_Speed.pri)
include (ANT/speed_cadence/ANT_SpeedCadence.pri)
include (ANT/power/ANT_Power.pri)
include (ANT/fec/ANT_fec.pri)
include (ANT/oxygen/ANT_Oxygen.pri)
include (ANT/ANT.pri)
include (Fit_20_16/Fit.pri)








RESOURCES += MyResources.qrc

TRANSLATIONS = powervelo_en.ts \
               powervelo_fr.ts \






