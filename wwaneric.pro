TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = \
    libPdu \
    gsmmanager


gsmmanager.depends = libPdu
