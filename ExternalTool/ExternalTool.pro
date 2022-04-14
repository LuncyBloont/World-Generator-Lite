QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    datacontext.cpp \
    dataview.cpp \
    main.cpp \
    mainwindow.cpp \
    ui.cpp \
    wglcore.cpp

HEADERS += \
    datacontext.h \
    dataview.h \
    helperMacro.h \
    mainwindow.h \
    ui.h \
    wglcore.h

FORMS += \
    mainwindow.ui \
    plantManager.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
