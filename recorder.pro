TARGET = recorderplugin

PLUGIN_TYPE = generic
PLUGIN_CLASS_NAME = RecorderPlugin

load(qt_plugin)

QT += quick core-private gui-private platformsupport-private concurrent

SOURCES = main.cpp

OTHER_FILES += \
    recorder.json
