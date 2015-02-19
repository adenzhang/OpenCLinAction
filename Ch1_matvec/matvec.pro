#-------------------------------------------------
#
# Project created by QtCreator 2013-07-02T15:58:21
#
#-------------------------------------------------

QT       -= core

QT       -= gui

TARGET = matvec
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

win32 {

CONFIG(debug, debug|release) : DESTDIR = $$PWD/../bin/debug/x64
else: CONFIG(release, debug|release) : DESTDIR = $$PWD/../bin/release/x64

#libary boost
LIBS += -LD:\onprogram\clib\boost\lib\x64
INCLUDEPATH += D:\onprogram\clib\boost\include

#library OpenCL
LIBS += -LD:\onprogram\clib\AMDAPP\lib\x86_64/ -lOpenCL
INCLUDEPATH += D:\onprogram\clib\AMDAPP\include

CONFIG(debug, debug|release) {
    LIBS += -L../bin/debug/x64 -lclUtil
    PRE_TARGETDEPS += $$PWD/../bin/debug/x64/clUtil.lib
} else:  CONFIG(release, debug|release) {
    LIBS += -L../bin/release/x64 -lclUtil
    PRE_TARGETDEPS += $$PWD/../bin/release/x64/clUtil.lib
}
INCLUDEPATH += ../clUtil
INCLUDEPATH += ../clUtil/include
DEPENDPATH += $$PWD/../clUtil/include

} else { #linux -------------------

CONFIG(debug, debug|release) : DESTDIR = $$PWD/../bin-linux/debug/x64
else: CONFIG(release, debug|release) : DESTDIR = $$PWD/../bin-linux/release/x64

LIBS += -lOpenCL

#libary boost
CONFIG(debug, debug|release) : LIBS += -L/program/linux/usr/boost/lib-x64-debug
else: CONFIG(release, debug|release) : LIBS += -L/program/linux/usr/boost/lib-x64-release
INCLUDEPATH += /program/linux/usr/boost/include

#library OpenCL
INCLUDEPATH += /program/onprogram/clib/AMDAPP/include

CONFIG(debug, debug|release) {
    LIBS += -L../bin-linux/debug/x64 -lclUtil
#    PRE_TARGETDEPS += $$PWD/../bin-linux/debug/x64/clUtil.a
} else:  CONFIG(release, debug|release) {
    LIBS += -L../bin-linux/release/x64 -lclUtil
#    PRE_TARGETDEPS += $$PWD/../bin-linux/release/x64/clUtil.a
}
INCLUDEPATH += ../clUtil
INCLUDEPATH += ../clUtil/include
}


SOURCES += \
    MatVec.cpp
#    matvec01.cpp


