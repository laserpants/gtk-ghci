TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    processio.c \
    ui.c \
    commandentry.c

INCLUDEPATH += /usr/include/gtk-3.0
INCLUDEPATH += /usr/include/glib-2.0

CONFIG += link_pkgconfig
PKGCONFIG += gtk+-3.0

HEADERS += \
    processio.h \
    ui.h \
    commandentry.h

