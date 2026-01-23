QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    hostelmanager_booking.cpp \
    hostelmanager_clients.cpp \
    hostelmanager_database.cpp \
    hostelmanager_reports.cpp \
    hostelmanager_rooms.cpp \
    hostelmanager_ui.cpp \
    main.cpp \
    hostelmanager.cpp

HEADERS += \
    hostelmanager.h \
    hostelmanager_booking.h \
    hostelmanager_clients.h \
    hostelmanager_database.h \
    hostelmanager_reports.h \
    hostelmanager_rooms.h \
    hostelmanager_ui.h

FORMS += \
    hostelmanager.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
