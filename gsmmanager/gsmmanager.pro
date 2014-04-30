TARGET = gsmmanager
TEMPLATE = app
QT       += core gui serialport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ..

#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/debug/ -lpdu
#else:unix: LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu

#LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
#LIBS += -L$$DESTDIR -L$$OUT_PWD -lpdu
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
    SmsView.cpp \
    SettingsView.cpp \
    ModemStatusView.cpp \
    UssdView.cpp \
    Ussd.cpp \
    common.cpp \
    PortController.cpp \
    ModemSms.cpp \
    ModemStatus.cpp \
    ModemUssd.cpp \
    AppSettingsDialog.cpp \
    ModemInit.cpp

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
    SmsView.h \
    SettingsView.h \
    ModemStatusView.h \
    UssdView.h \
    Ussd.h \
    PortController.h \
    ModemSms.h \
    ModemStatus.h \
    ModemUssd.h \
    AppSettingsDialog.h \
    ModemInit.h

FORMS    += \
    MainWindow.ui \
    SmsView.ui \
    SettingsView.ui \
    ModemStatusView.ui \
    UssdView.ui \
    AppSettingsDialog.ui

RESOURCES += \
    icons.qrc

win32:RC_FILE = gsmmanager.rc

OTHER_FILES += \
    gsmmanager.rc


# icons
copyicons.commands = $(COPY_DIR) $$PWD/icons $$DESTDIR
first.depends = $(first) copyicons
export(first.depends)
export(copyicons.commands)
QMAKE_EXTRA_TARGETS += first copyicons


# Debug windows
CONFIG(debug, debug|release) {
HEADERS   += \
    debugParsers.h

SOURCES   += \
    debugParsers.cpp

FORMS     += \
    debugParsers.ui
}
