#-------------------------------------------------
#
# Project created by QtCreator 2013-07-02T16:22:04
#
#-------------------------------------------------

QT       -= core gui

TARGET = clUtil
TEMPLATE = lib
CONFIG += staticlib


INCLUDEPATH += include

win32 {

CONFIG(debug, debug|release) : DESTDIR = $$PWD/../bin/debug/x64
else: CONFIG(release, debug|release) : DESTDIR = $$PWD/../bin/release/x64

#libary boost
LIBS += -LD:\onprogram\clib\boost\lib\x64
INCLUDEPATH += D:\onprogram\clib\boost\include

#library OpenCL
LIBS += -LD:\onprogram\clib\AMDAPP\lib\x86_64/ -lOpenCL
INCLUDEPATH += D:\onprogram\clib\AMDAPP\include

} else { #linux


CONFIG(debug, debug|release) : DESTDIR = $$PWD/../bin-linux/debug/x64
else: CONFIG(release, debug|release) : DESTDIR = $$PWD/../bin-linux/release/x64

LIBS += -lOpenCL

#libary boost
CONFIG(debug, debug|release) : LIBS += -L/program/linux/usr/boost/lib-x64-debug
else: CONFIG(release, debug|release) : LIBS += -L/program/linux/usr/boost/lib-x64-release
INCLUDEPATH += /program/linux/usr/boost/include

#library OpenCL
INCLUDEPATH += /program/onprogram/clib/AMDAPP/include
}
SOURCES += clutil.cpp \
    SDKThread.cpp \
    SDKFile.cpp \
    SDKCommon.cpp \
    SDKCommandArgs.cpp \
    SDKBitMap.cpp \
    SDKApplication.cpp

HEADERS += clutil.h \
    include/SDKThread.hpp \
    include/SDKFile.hpp \
    include/SDKCommon.hpp \
    include/SDKCommandArgs.hpp \
    include/SDKBitMap.hpp \
    include/SDKApplication.hpp \
    CMat.h


unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
