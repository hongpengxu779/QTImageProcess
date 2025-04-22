QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# OpenCV 配置
INCLUDEPATH += E:/C++/QIImageProcess/3rdparty/opencv/include
LIBS += -LE:/C++/QIImageProcess/3rdparty/opencv/lib -lopencv_world490

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ImageProcessor/ImageProcessor.cpp \
    ImageView/ProcessingWidget.cpp \
    ImageView/ImageProcessorThread.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ImageProcessor/ImageProcessor.h \
    ImageView/ProcessingWidget.h \
    ImageView/ImageProcessorThread.h \
    mainwindow.h

INCLUDEPATH += $$PWD

INCLUDEPATH += D:/xu/YOLOproject/opencv470/build/install/include
LIBS += -LD:/xu/YOLOproject/opencv470/build/install/x64/vc16/lib -lopencv_world470d

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
