# -------------------------------------------------
# Project created by QtCreator 2009-07-03T20:01:38
# -------------------------------------------------
TARGET = ConanDemo
CONFIG += console
CONFIG -= app_bundle
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
    win32:LIBS += ../Conan/lib/libConand1.a
    else:LIBS += ../Conan/lib/libConand.so
}
else{
    win32:LIBS += ../Conan/lib/libConan1.a
    else:LIBS += ../Conan/lib/libConan.so
}
TEMPLATE = app
DESTDIR = bin
MOC_DIR = src
RCC_DIR = src
UI_DIR = src
OBJECTS_DIR = obj
INCLUDEPATH = ../Conan/include
SOURCES += src/main.cpp \
    src/TestClasses.cpp
HEADERS += src/TestClasses.h
