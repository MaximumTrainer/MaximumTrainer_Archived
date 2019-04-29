INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES +=\
    createWorkout/repeatwidget.cpp \
    createWorkout/mycreatorplot.cpp \
    createWorkout/workoutcreator.cpp \
    createWorkout/intervaldelegate.cpp \
    createWorkout/powereditor.cpp \
    createWorkout/cadenceeditor.cpp \
    createWorkout/hreditor.cpp \
    createWorkout/tableviewinterval.cpp \
    createWorkout/intervalviewstyle.cpp \
    createWorkout/myqwtpickermachine.cpp \
    createWorkout/myqwtplotpicker.cpp \
    $$PWD/repeatincreaseeditor.cpp

HEADERS +=\
    createWorkout/repeatwidget.h \
    createWorkout/mycreatorplot.h \
    createWorkout/workoutcreator.h \
    createWorkout/intervaldelegate.h \
    createWorkout/powereditor.h \
    createWorkout/cadenceeditor.h \
    createWorkout/hreditor.h \
    createWorkout/tableviewinterval.h \
    createWorkout/intervalviewstyle.h \
    createWorkout/myqwtpickermachine.h \
    createWorkout/myqwtplotpicker.h \
    $$PWD/repeatincreaseeditor.h

FORMS += \
    createWorkout/repeatwidget.ui \
    createWorkout/workoutcreator.ui \
    createWorkout/powereditor.ui \
    createWorkout/cadenceeditor.ui \
    createWorkout/hreditor.ui \
    $$PWD/repeatincreaseeditor.ui

