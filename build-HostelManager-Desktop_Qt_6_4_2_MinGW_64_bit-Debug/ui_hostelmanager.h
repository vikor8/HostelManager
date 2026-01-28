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
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_HostelManager
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QDateEdit *dateEdit;
    QPushButton *btnToday;
    QSpacerItem *horizontalSpacer;
    QLabel *lblStatus;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QTableWidget *tableWidget;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QFrame *frameOccupied;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QFrame *frameOccupied_2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QFrame *horizontalFrame;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_5;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *btnRefresh;

    void setupUi(QMainWindow *HostelManager)
    {
        if (HostelManager->objectName().isEmpty())
            HostelManager->setObjectName("HostelManager");
        HostelManager->resize(1300, 700);
        centralwidget = new QWidget(HostelManager);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        horizontalLayout = new QHBoxLayout(groupBox);
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(groupBox);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        dateEdit = new QDateEdit(groupBox);
        dateEdit->setObjectName("dateEdit");

        horizontalLayout->addWidget(dateEdit);

        btnToday = new QPushButton(groupBox);
        btnToday->setObjectName("btnToday");

        horizontalLayout->addWidget(btnToday);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        lblStatus = new QLabel(groupBox);
        lblStatus->setObjectName("lblStatus");

        horizontalLayout->addWidget(lblStatus);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        tableWidget = new QTableWidget(groupBox_2);
        tableWidget->setObjectName("tableWidget");
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setShowGrid(true);
        tableWidget->setGridStyle(Qt::SolidLine);
        tableWidget->setCornerButtonEnabled(true);
        tableWidget->horizontalHeader()->setVisible(true);
        tableWidget->horizontalHeader()->setHighlightSections(false);
        tableWidget->horizontalHeader()->setStretchLastSection(false);
        tableWidget->verticalHeader()->setVisible(false);

        verticalLayout_2->addWidget(tableWidget);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        horizontalLayout_2->addWidget(label_2);

        frameOccupied = new QFrame(groupBox_2);
        frameOccupied->setObjectName("frameOccupied");
        frameOccupied->setStyleSheet(QString::fromUtf8("background-color: rgb(200, 200, 0);"));
        frameOccupied->setFrameShape(QFrame::Box);
        frameOccupied->setFrameShadow(QFrame::Raised);
        frameOccupied->setLineWidth(1);
        horizontalLayout_4 = new QHBoxLayout(frameOccupied);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label_4 = new QLabel(frameOccupied);
        label_4->setObjectName("label_4");

        horizontalLayout_4->addWidget(label_4);


        horizontalLayout_2->addWidget(frameOccupied);

        frameOccupied_2 = new QFrame(groupBox_2);
        frameOccupied_2->setObjectName("frameOccupied_2");
        frameOccupied_2->setStyleSheet(QString::fromUtf8("background-color: rgb(144, 238, 144);"));
        frameOccupied_2->setFrameShape(QFrame::Box);
        frameOccupied_2->setFrameShadow(QFrame::Raised);
        horizontalLayout_3 = new QHBoxLayout(frameOccupied_2);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(9, 9, 9, 9);
        label_3 = new QLabel(frameOccupied_2);
        label_3->setObjectName("label_3");

        horizontalLayout_3->addWidget(label_3);


        horizontalLayout_2->addWidget(frameOccupied_2);

        horizontalFrame = new QFrame(groupBox_2);
        horizontalFrame->setObjectName("horizontalFrame");
        horizontalFrame->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 200, 150);"));
        horizontalFrame->setFrameShape(QFrame::Box);
        horizontalFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_5 = new QHBoxLayout(horizontalFrame);
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(9, 9, 9, 9);
        label_5 = new QLabel(horizontalFrame);
        label_5->setObjectName("label_5");
        label_5->setFrameShape(QFrame::NoFrame);
        label_5->setFrameShadow(QFrame::Raised);

        horizontalLayout_5->addWidget(label_5);


        horizontalLayout_2->addWidget(horizontalFrame);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        btnRefresh = new QPushButton(groupBox_2);
        btnRefresh->setObjectName("btnRefresh");

        horizontalLayout_2->addWidget(btnRefresh);


        verticalLayout_2->addLayout(horizontalLayout_2);


        verticalLayout->addWidget(groupBox_2);

        HostelManager->setCentralWidget(centralwidget);

        retranslateUi(HostelManager);

        QMetaObject::connectSlotsByName(HostelManager);
    } // setupUi

    void retranslateUi(QMainWindow *HostelManager)
    {
        HostelManager->setWindowTitle(QCoreApplication::translate("HostelManager", "Hotel Manager - \320\241\320\270\321\201\321\202\320\265\320\274\320\260 \321\203\320\277\321\200\320\260\320\262\320\273\320\265\320\275\320\270\321\217 \320\261\321\200\320\276\320\275\320\270\321\200\320\276\320\262\320\260\320\275\320\270\321\217\320\274\320\270", nullptr));
        groupBox->setTitle(QCoreApplication::translate("HostelManager", "\320\237\320\260\321\200\320\260\320\274\320\265\321\202\321\200\321\213 \320\276\321\202\320\276\320\261\321\200\320\260\320\266\320\265\320\275\320\270\321\217", nullptr));
        label->setText(QCoreApplication::translate("HostelManager", "\320\235\320\260\321\207\320\260\320\273\321\214\320\275\320\260\321\217 \320\264\320\260\321\202\320\260:", nullptr));
        btnToday->setText(QCoreApplication::translate("HostelManager", "\320\241\320\265\320\263\320\276\320\264\320\275\321\217", nullptr));
        lblStatus->setText(QCoreApplication::translate("HostelManager", "\320\221\320\260\320\267\320\260 \320\264\320\260\320\275\320\275\321\213\321\205: \320\277\320\276\320\264\320\272\320\273\321\216\321\207\320\265\320\275\320\260", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("HostelManager", "\320\240\320\260\321\201\320\277\320\270\321\201\320\260\320\275\320\270\320\265 \320\267\320\260\320\275\321\217\321\202\320\276\321\201\321\202\320\270 \320\275\320\276\320\274\320\265\321\200\320\276\320\262 (30 \320\264\320\275\320\265\320\271)", nullptr));
        label_2->setText(QCoreApplication::translate("HostelManager", "\320\233\320\265\320\263\320\265\320\275\320\264\320\260:", nullptr));
        label_4->setText(QCoreApplication::translate("HostelManager", "\321\207\320\260\321\201\321\202\320\270\321\207\320\275\320\276 \320\276\320\277\320\273\320\260\321\207\320\265\320\275\320\276", nullptr));
        label_3->setText(QCoreApplication::translate("HostelManager", "\320\277\320\276\320\273\320\275\320\276\321\201\321\202\321\214\321\216 \320\276\320\277\320\273\320\260\321\207\320\265", nullptr));
        label_5->setText(QCoreApplication::translate("HostelManager", "\320\275\320\265 \320\276\320\277\320\273\320\260\321\207\320\265\320\275\320\276", nullptr));
        btnRefresh->setText(QCoreApplication::translate("HostelManager", "\320\236\320\261\320\275\320\276\320\262\320\270\321\202\321\214", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HostelManager: public Ui_HostelManager {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HOSTELMANAGER_H
