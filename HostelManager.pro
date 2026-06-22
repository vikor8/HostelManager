QT += core gui sql printsupport


CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adminpasswordmanager.cpp \
    main.cpp \
    addbookingdialog.cpp \
    addclientdialog.cpp \
    cancelbookingdialog.cpp \
    database.cpp \
    editpaymentdialog.cpp \   
    hostelmanager.cpp \
    reportwindow.cpp


HEADERS += \
    addbookingdialog.h \
    addclientdialog.h \
    adminpasswordmanager.h \
    cancelbookingdialog.h \
    database.h \
    editpaymentdialog.h \
    hostelmanager.h \
    reportwindow.h

FORMS += \
    hostelmanager.ui

RESOURCES += resources.qrc

win32: {
    RC_ICONS = Kolcovo.ico
}


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
