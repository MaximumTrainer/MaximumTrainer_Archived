INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += gui/mainwindow.cpp\
gui/workoutdialog.cpp \
    gui/main_workoutpage.cpp \
    gui/dialogconfig.cpp \
    gui/dialoglogin.cpp \
    gui/z_stylesheet.cpp \
    gui/updatedialog.cpp \
    gui/savingwindow.cpp \
    gui/dialogmainwindowconfig.cpp \
    gui/dialoginfowebview.cpp \
    #$$PWD/main_coursepage.cpp

HEADERS += gui/mainwindow.h\
gui/workoutdialog.h \
    gui/main_workoutpage.h \
    gui/dialogconfig.h \
    gui/dialoglogin.h \
    gui/z_stylesheet.h \
    gui/updatedialog.h \
    gui/savingwindow.h \
    gui/dialogmainwindowconfig.h \
    gui/dialoginfowebview.h \
    #$$PWD/main_coursepage.h

FORMS    += gui/mainwindow.ui \
    gui/workoutdialog.ui \
    gui/main_workoutpage.ui \
    gui/dialogconfig.ui \
    gui/dialoglogin.ui \
    gui/z_stylesheet.ui \
    gui/updatedialog.ui \
    gui/savingwindow.ui \
    gui/dialogmainwindowconfig.ui \
    gui/dialoginfowebview.ui \
    #$$PWD/main_coursepage.ui


