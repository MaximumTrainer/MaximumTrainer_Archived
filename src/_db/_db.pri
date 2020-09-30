INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES     +=\
    src/_db/userdao.cpp \
    src/_db/extrequest.cpp \
    src/_db/versiondao.cpp \
    src/_db/environnement.cpp \
    src/_db/sensordao.cpp \
    $$PWD/radiodao.cpp \
    $$PWD/achievementdao.cpp

HEADERS     += \
    src/_db/userdao.h \
    src/_db/extrequest.h \
    src/_db/versiondao.h \
    src/_db/environnement.h \
    src/_db/sensordao.h \
    $$PWD/radiodao.h \
    $$PWD/achievementdao.h





