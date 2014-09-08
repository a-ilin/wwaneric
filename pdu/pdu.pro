QT       -= gui
TEMPLATE = lib

TARGET = pdu

DEFINES += PDU_DECODE_EXPORTS

win32-msvc* {
  # MSVC
  # Disable C996 Warning
  DEFINES += _CRT_SECURE_NO_WARNINGS snprintf=_snprintf
} else {
  # MinGW
  QMAKE_CXXFLAGS += -std=c++0x
}

SOURCES += \
    pdu.cpp

HEADERS += \
    pdu.h

DESTDIR = ..

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
