INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD



FORMS += \
    $$PWD/addbookmarkdialog.ui \
    $$PWD/bookmarks.ui \
    $$PWD/cookies.ui \
    $$PWD/cookiesexceptions.ui \
    $$PWD/downloaditem.ui \
    $$PWD/downloads.ui \
    $$PWD/history.ui \
    $$PWD/passworddialog.ui \
    $$PWD/printtopdfdialog.ui \
    $$PWD/proxy.ui \
    $$PWD/savepagedialog.ui \
    $$PWD/settings.ui

HEADERS += \
    $$PWD/autosaver.h \
    $$PWD/bookmarks.h \
    $$PWD/browserapplication.h \
    $$PWD/browsermainwindow.h \
    $$PWD/chasewidget.h \
    $$PWD/downloadmanager.h \
    $$PWD/edittableview.h \
    $$PWD/edittreeview.h \
    $$PWD/featurepermissionbar.h\
    $$PWD/fullscreennotification.h \
    $$PWD/history.h \
    $$PWD/modelmenu.h \
    $$PWD/printtopdfdialog.h \
    $$PWD/savepagedialog.h \
    $$PWD/searchlineedit.h \
    $$PWD/settings.h \
    $$PWD/squeezelabel.h \
    $$PWD/tabwidget.h \
    $$PWD/toolbarsearch.h \
    $$PWD/urllineedit.h \
    $$PWD/webview.h \
    $$PWD/xbel.h

SOURCES += \
    $$PWD/autosaver.cpp \
    $$PWD/bookmarks.cpp \
    $$PWD/browserapplication.cpp \
    $$PWD/browsermainwindow.cpp \
    $$PWD/chasewidget.cpp \
    $$PWD/downloadmanager.cpp \
    $$PWD/edittableview.cpp \
    $$PWD/edittreeview.cpp \
    $$PWD/featurepermissionbar.cpp\
    $$PWD/fullscreennotification.cpp \
    $$PWD/history.cpp \
    $$PWD/modelmenu.cpp \
    $$PWD/printtopdfdialog.cpp \
    $$PWD/savepagedialog.cpp \
    $$PWD/searchlineedit.cpp \
    $$PWD/settings.cpp \
    $$PWD/squeezelabel.cpp \
    $$PWD/tabwidget.cpp \
    $$PWD/toolbarsearch.cpp \
    $$PWD/urllineedit.cpp \
    $$PWD/webview.cpp \
    $$PWD/xbel.cpp \
    $$PWD/main.cpp

RESOURCES += $$PWD/data/data.qrc  $$PWD/htmls/htmls.qrc



win32 {
   RC_FILE = $$PWD/demobrowser.rc
}

mac {
    ICON = $$PWD/demobrowser.icns
}


