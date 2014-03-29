
QT       -= gui

TARGET = pdu
TEMPLATE = lib

DEFINES += PDU_DECODE_EXPORTS

QMAKE_CXXFLAGS += -std=c++0x

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
