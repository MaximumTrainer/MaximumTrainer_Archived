INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES +=\
    src/createWorkout/repeatwidget.cpp \
    src/createWorkout/mycreatorplot.cpp \
    src/createWorkout/workoutcreator.cpp \
    src/createWorkout/intervaldelegate.cpp \
    src/createWorkout/powereditor.cpp \
    src/createWorkout/cadenceeditor.cpp \
    src/createWorkout/hreditor.cpp \
    src/createWorkout/tableviewinterval.cpp \
    src/createWorkout/intervalviewstyle.cpp \
    src/createWorkout/myqwtpickermachine.cpp \
    src/createWorkout/myqwtplotpicker.cpp \
    $$PWD/repeatincreaseeditor.cpp

HEADERS +=\
    src/createWorkout/repeatwidget.h \
    src/createWorkout/mycreatorplot.h \
    src/createWorkout/workoutcreator.h \
    src/createWorkout/intervaldelegate.h \
    src/createWorkout/powereditor.h \
    src/createWorkout/cadenceeditor.h \
    src/createWorkout/hreditor.h \
    src/createWorkout/tableviewinterval.h \
    src/createWorkout/intervalviewstyle.h \
    src/createWorkout/myqwtpickermachine.h \
    src/createWorkout/myqwtplotpicker.h \
    $$PWD/repeatincreaseeditor.h

FORMS += \
    src/createWorkout/repeatwidget.ui \
    src/createWorkout/workoutcreator.ui \
    src/createWorkout/powereditor.ui \
    src/createWorkout/cadenceeditor.ui \
    src/createWorkout/hreditor.ui \
    $$PWD/repeatincreaseeditor.ui

