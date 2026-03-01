INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += src/gui/mainwindow.cpp\
src/gui/workoutdialog.cpp \
    src/gui/main_workoutpage.cpp \
    src/gui/dialogconfig.cpp \
    src/gui/dialoglogin.cpp \
    src/gui/z_stylesheet.cpp \
    src/gui/updatedialog.cpp \
    src/gui/savingwindow.cpp \
    src/gui/dialogmainwindowconfig.cpp \
    src/gui/dialoginfowebview.cpp \
    src/gui/dialog_connection_method.cpp \
    #$$PWD/main_coursepage.cpp

HEADERS += src/gui/mainwindow.h\
src/gui/workoutdialog.h \
    src/gui/main_workoutpage.h \
    src/gui/dialogconfig.h \
    src/gui/dialoglogin.h \
    src/gui/z_stylesheet.h \
    src/gui/updatedialog.h \
    src/gui/savingwindow.h \
    src/gui/dialogmainwindowconfig.h \
    src/gui/dialoginfowebview.h \
    src/gui/dialog_connection_method.h \
    #$$PWD/main_coursepage.h

FORMS    += src/gui/mainwindow.ui \
    src/gui/workoutdialog.ui \
    src/gui/main_workoutpage.ui \
    src/gui/dialogconfig.ui \
    src/gui/dialoglogin.ui \
    src/gui/z_stylesheet.ui \
    src/gui/updatedialog.ui \
    src/gui/savingwindow.ui \
    src/gui/dialogmainwindowconfig.ui \
    src/gui/dialoginfowebview.ui \
    src/gui/dialog_connection_method.ui \
    #$$PWD/main_coursepage.ui


