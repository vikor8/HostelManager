/********************************************************************************
** Form generated from reading UI file 'hostelmanager.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HOSTELMANAGER_H
#define UI_HOSTELMANAGER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_HostelManager
{
public:
    QWidget *centralwidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *HostelManager)
    {
        if (HostelManager->objectName().isEmpty())
            HostelManager->setObjectName("HostelManager");
        HostelManager->resize(800, 600);
        centralwidget = new QWidget(HostelManager);
        centralwidget->setObjectName("centralwidget");
        HostelManager->setCentralWidget(centralwidget);
        menubar = new QMenuBar(HostelManager);
        menubar->setObjectName("menubar");
        HostelManager->setMenuBar(menubar);
        statusbar = new QStatusBar(HostelManager);
        statusbar->setObjectName("statusbar");
        HostelManager->setStatusBar(statusbar);

        retranslateUi(HostelManager);

        QMetaObject::connectSlotsByName(HostelManager);
    } // setupUi

    void retranslateUi(QMainWindow *HostelManager)
    {
        HostelManager->setWindowTitle(QCoreApplication::translate("HostelManager", "HostelManager", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HostelManager: public Ui_HostelManager {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HOSTELMANAGER_H
