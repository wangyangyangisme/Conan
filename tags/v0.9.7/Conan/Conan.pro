# -------------------------------------------------
# Project created by QtCreator 2009-06-18T19:49:43
# -------------------------------------------------
TARGET = Conan
CONFIG(debug, debug|release):TARGET = $$join(TARGET,,,d)
TEMPLATE = lib
DEFINES += CONAN_EXPORTS
DESTDIR = bin
MOC_DIR = src
RCC_DIR = src
UI_DIR = src
INCLUDEPATH = $(QTDIR)/src
SOURCES += src/ObjectUtility.cpp \
    src/signalspy.cpp \
    src/ConnectionModel.cpp \
    src/ObjectModel.cpp \
    src/ConanWidget.cpp \
    src/AboutDialog.cpp \
    src/ConanDebug.cpp \
    src/KeyValueTableModel.cpp
HEADERS += src/ConanDefines.h \
    src/ObjectUtility.h \
    src/signalspy.h \
    src/ConnectionModel.h \
    src/ConanWidget_p.h \
    src/ConanWidget.h \
    src/ConanDefines.h \
    src/AboutDialog.h \
    src/ObjectModel.h \
    src/ConanDebug.h \
    src/KeyValueTableModel.h
FORMS += src/ConanWidget.ui \
    src/AboutDialog.ui
RESOURCES += src/Conan.qrc
