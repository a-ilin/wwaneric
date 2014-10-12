TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = \
    pdu \
    libPdu \
    gsmmanager


gsmmanager.depends = pdu libPdu
