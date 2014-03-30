TARGET = gsmmanager
TEMPLATE = app
QT       += core gui serialport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ..

#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/debug/ -lpdu
#else:unix: LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu

#LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
LIBS += -L$$DESTDIR -lpdu

#INCLUDEPATH += $$PWD/../pdu-bin/include
#DEPENDPATH += $$PWD/../pdu-bin

#win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a
#else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/debug/libpdu.a
#else:unix: PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a

#PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a



QMAKE_CXXFLAGS += -std=c++0x




SOURCES += \
    main.cpp\
    log.cpp \
    Core.cpp \
    MainWindow.cpp \
    Modem.cpp \
    Database.cpp \
    Sms.cpp \
    Settings.cpp \
    views/SmsView.cpp \
    views/SettingsView.cpp \
    views/ModemStatusView.cpp \
    views/UssdView.cpp \
    Ussd.cpp

HEADERS  += \
    common.h \
    log.h \
    IView.h \
    Core.h \
    MainWindow.h \
    Modem.h \
    Database.h \
    Sms.h \
    Settings.h \
    views/SmsView.h \
    views/SettingsView.h \
    views/ModemStatusView.h \
    views/UssdView.h \
    Ussd.h

FORMS    += \
    MainWindow.ui \
    views/SmsView.ui \
    views/SettingsView.ui \
    views/ModemStatusView.ui \
    views/UssdView.ui

RESOURCES += \
    icons.qrc

win32:RC_FILE = gsmmanager.rc

OTHER_FILES += \
    gsmmanager.rc

