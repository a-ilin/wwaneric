TEMPLATE = subdirs

# Disable auto casting QString from QByteArray
DEFINES += QT_NO_CAST_FROM_ASCII

CONFIG += ordered

SUBDIRS = \
    pdu \
    libPdu \
    gsmmanager


gsmmanager.depends = pdu libPdu
