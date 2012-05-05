TARGET = recorderplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

QT += quick core-private platformsupport-private concurrent

SOURCES = main.cpp

OTHER_FILES += \
    recorder.json
