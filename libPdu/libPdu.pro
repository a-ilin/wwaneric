QT += core
QT -= gui

TEMPLATE = lib

TARGET = libPdu

DEFINES += PDU_DECODE_EXPORTS

include($$PWD/../wwaneric.pri)

win32-msvc* {
  # MSVC
  # Disable C996 Warning
  DEFINES += _CRT_SECURE_NO_WARNINGS
} else {
  # MinGW
  QMAKE_CXXFLAGS += -std=c++0x
}


DESTDIR = ..

OTHER_FILES += \
    Makefile \
    src/Makefile

HEADERS += \
    src/caststring.h \
    src/Pdu.h \
    src/pdu_7bit_packing.h \
    src/pdu_8bit_packing.h \
    src/pdu_address.h \
    src/pdu_base.h \
    src/pdu_datacoding.h \
    src/pdu_deliver.h \
    src/pdu_log.h \
    src/pdu_packed.h \
    src/pdu_submit.h \
    src/pdu_userdata.h \
    src/pdu_userdataheader.h \
    src/common.h

SOURCES += \
    src/Pdu.cpp \
    src/pdu_7bit_packing.cpp \
    src/pdu_8bit_packing.cpp \
    src/pdu_address.cpp \
    src/pdu_base.cpp \
    src/pdu_datacoding.cpp \
    src/pdu_deliver.cpp \
    src/pdu_log.cpp \
    src/pdu_packed.cpp \
    src/pdu_submit.cpp \
    src/pdu_userdata.cpp \
    src/pdu_userdataheader.cpp



