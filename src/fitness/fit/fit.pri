INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD


#INCLUDEPATH += C:/Dropbox/FitSDKRelease_20.16.00/cpp


SOURCES += \
    $$PWD/fit_unicode.cpp \
    $$PWD/fit_protocol_validator.cpp \
    $$PWD/fit_profile.cpp \
    $$PWD/fit_mesg_with_event_broadcaster.cpp \
    $$PWD/fit_mesg_definition.cpp \
    $$PWD/fit_mesg_broadcaster.cpp \
    $$PWD/fit_mesg.cpp \
    $$PWD/fit_field_definition.cpp \
    $$PWD/fit_field_base.cpp \
    $$PWD/fit_field.cpp \
    $$PWD/fit_factory.cpp \
    $$PWD/fit_encode.cpp \
    $$PWD/fit_developer_field_description.cpp \
    $$PWD/fit_developer_field_definition.cpp \
    $$PWD/fit_developer_field.cpp \
    $$PWD/fit_decode.cpp \
    $$PWD/fit_date_time.cpp \
    $$PWD/fit_crc.cpp \
    $$PWD/fit_buffer_encode.cpp \
    $$PWD/fit_buffered_record_mesg_broadcaster.cpp \
    $$PWD/fit_buffered_mesg_broadcaster.cpp \
    $$PWD/fit_accumulator.cpp \
    $$PWD/fit_accumulated_field.cpp \
    $$PWD/fit.cpp

HEADERS += $$PWD/*.h
HEADERS += $$PWD/*.hpp
