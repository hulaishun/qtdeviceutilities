######################################################################
# Automatically generated by qmake (3.0) Thu Mar 7 13:00:32 2013
######################################################################

CXX_MODULE = qml
TARGET = android_omx
TARGETPATH = QtAndroidOmx
IMPORT_VERSION = 1.0

INCLUDEPATH += $$ANDROID_BUILD_TOP/development/ndk/platforms/android-14/include/

LIBS += -lOpenMAXAL -lui -lgui -lutils -lcutils -lbinder

QT += qml quick

# Input
HEADERS += omxnode.h omxplayer.h
SOURCES += omx.cpp SurfaceTexture.cpp BufferQueue.cpp omxnode.cpp omxmodule.cpp

load(qml_plugin)

