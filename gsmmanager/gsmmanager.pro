#
# The MIT License (MIT)
#
# Copyright (c) 2014-2017 by Aleksei Ilin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

TARGET = gsmmanager
TEMPLATE = app
QT       += core gui serialport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ..

include($$PWD/../wwaneric.pri)

#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../pdu-bin/debug/ -lpdu
#else:unix: LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu

#LIBS += -L$$OUT_PWD/../pdu-bin/release/ -lpdu
#LIBS += -L$$DESTDIR -L$$OUT_PWD -lpdu
LIBS += -L$$DESTDIR -llibPdu

#INCLUDEPATH += $$PWD/../pdu-bin/include
#DEPENDPATH += $$PWD/../pdu-bin

#win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a
#else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/debug/libpdu.a
#else:unix: PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a

#PRE_TARGETDEPS += $$OUT_PWD/../pdu-bin/release/libpdu.a

DEFINES += LOG_ENABLED LOG_MULTITHREADING

win32-msvc* {
  # MSVC
  # Disable C996 Warning
  DEFINES += _CRT_SECURE_NO_WARNINGS
} else {
  # MinGW
  QMAKE_CXXFLAGS += -std=c++0x
}


win32 {
  SOURCES += \
    crashdump_win.cpp \
    zip_win.cpp
}

SOURCES += \
    main.cpp \
    crashdump.cpp \
    log.cpp \
    Core.cpp \
    MainWindow.cpp \
    Modem.cpp \
    Database.cpp \
    Sms.cpp \
    Settings.cpp \
    SmsView.cpp \
    ModemStatusView.cpp \
    UssdView.cpp \
    Ussd.cpp \
    common.cpp \
    PortController.cpp \
    ModemSms.cpp \
    ModemStatus.cpp \
    ModemUssd.cpp \
    AppSettingsDialog.cpp \
    ModemInit.cpp \
    SerialPortSettingsView.cpp

HEADERS  += \
    crashdump.h \
    zip.h \
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
    ModemStatusView.h \
    UssdView.h \
    Ussd.h \
    PortController.h \
    ModemSms.h \
    ModemStatus.h \
    ModemUssd.h \
    AppSettingsDialog.h \
    ModemInit.h \
    SerialPortSettingsView.h

FORMS    += \
    MainWindow.ui \
    SmsView.ui \
    ModemStatusView.ui \
    UssdView.ui \
    AppSettingsDialog.ui \
    SerialPortSettingsView.ui

RESOURCES += \
    icons.qrc

win32:RC_FILE = gsmmanager.rc

OTHER_FILES += \
    gsmmanager.rc


# icons
iconsDirSrc = $$PWD/icons
iconsDirDst = $$DESTDIR/icons
win32:iconsDirSrc = $$replace(iconsDirSrc, "/", "\\")
win32:iconsDirDst = $$replace(iconsDirDst, "/", "\\")
copyicons.commands = $(COPY_DIR) $$iconsDirSrc $$iconsDirDst
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
