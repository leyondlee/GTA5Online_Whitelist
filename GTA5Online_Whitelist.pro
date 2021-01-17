QT += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES += resource.qrc

SOURCES += \
    addaddressdialog.cpp \
    customaddresslistwidget.cpp \
    firewalltool.cpp \
    iptool.cpp \
    main.cpp \
    mainwindow.cpp \
    selectdevicedialog.cpp \
    sessiondialog.cpp \
    sniffer.cpp \
    snifferthread.cpp

HEADERS += \
    addaddressdialog.h \
    customaddresslistwidget.h \
    firewalltool.h \
    iptool.h \
    mainwindow.h \
    selectdevicedialog.h \
    sessiondialog.h \
    sniffer.h \
    snifferthread.h

FORMS += \
    addaddressdialog.ui \
    mainwindow.ui \
    selectdevicedialog.ui \
    sessiondialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L$$PWD/../../Downloads/npcap-sdk-1.06/Lib/x64/ -lPacket -lwpcap
INCLUDEPATH += $$PWD/../../Downloads/npcap-sdk-1.06/Include

LIBS += -lws2_32 -lole32 -loleaut32 -lcomsuppw

RC_ICONS = icons/icon.ico

include($$PWD/../../Downloads/QHotkey-1.4.2/qhotkey.pri)
include($$PWD/../../Downloads/SingleApplication-3.2.0/singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=QApplication
