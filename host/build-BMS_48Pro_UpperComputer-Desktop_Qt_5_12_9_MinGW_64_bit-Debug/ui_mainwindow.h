/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.9
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *horizontalLayoutWidget_16;
    QHBoxLayout *horizontalLayout_13;
    QVBoxLayout *verticalLayout_7;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_12;
    QVBoxLayout *verticalLayout_5;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *CAN_Config;
    QLabel *label;
    QComboBox *comboBox;
    QHBoxLayout *CAN_Config_2;
    QLabel *label_5;
    QComboBox *comboBox_2;
    QHBoxLayout *CAN_Config_3;
    QLabel *label_24;
    QComboBox *comboBox_3;
    QHBoxLayout *horizontalLayout_11;
    QPushButton *can_connect;
    QPushButton *can_disconnect;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *dischargemos_open;
    QPushButton *dischargemos_close;
    QHBoxLayout *horizontalLayout;
    QPushButton *chargemos_close;
    QPushButton *chargemos_open;
    QHBoxLayout *horizontalLayout_14;
    QPushButton *dischargemos_open_2;
    QPushButton *dischargemos_close_2;
    QHBoxLayout *horizontalLayout_10;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_9;
    QLabel *label_25;
    QLabel *label_10;
    QLabel *label_11;
    QLabel *label_12;
    QLabel *label_13;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_14;
    QLabel *label_26;
    QLabel *label_15;
    QLabel *label_16;
    QLabel *label_17;
    QLabel *label_18;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLabel *label_27;
    QLabel *label_6;
    QLabel *label_8;
    QLabel *label_7;
    QLabel *label_4;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_20;
    QLabel *label_21;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_19;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout_9;
    QLabel *label_22;
    QLabel *label_23;
    QPlainTextEdit *rizhi;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1032, 585);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayoutWidget_16 = new QWidget(centralwidget);
        horizontalLayoutWidget_16->setObjectName(QString::fromUtf8("horizontalLayoutWidget_16"));
        horizontalLayoutWidget_16->setGeometry(QRect(40, 100, 941, 351));
        horizontalLayout_13 = new QHBoxLayout(horizontalLayoutWidget_16);
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        horizontalLayout_13->setContentsMargins(0, 0, 0, 0);
        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        CAN_Config = new QHBoxLayout();
        CAN_Config->setSpacing(2);
        CAN_Config->setObjectName(QString::fromUtf8("CAN_Config"));
        CAN_Config->setContentsMargins(20, -1, 20, -1);
        label = new QLabel(horizontalLayoutWidget_16);
        label->setObjectName(QString::fromUtf8("label"));

        CAN_Config->addWidget(label);

        comboBox = new QComboBox(horizontalLayoutWidget_16);
        comboBox->setObjectName(QString::fromUtf8("comboBox"));

        CAN_Config->addWidget(comboBox);


        verticalLayout_4->addLayout(CAN_Config);

        CAN_Config_2 = new QHBoxLayout();
        CAN_Config_2->setSpacing(2);
        CAN_Config_2->setObjectName(QString::fromUtf8("CAN_Config_2"));
        CAN_Config_2->setContentsMargins(20, -1, 20, -1);
        label_5 = new QLabel(horizontalLayoutWidget_16);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        CAN_Config_2->addWidget(label_5);

        comboBox_2 = new QComboBox(horizontalLayoutWidget_16);
        comboBox_2->setObjectName(QString::fromUtf8("comboBox_2"));

        CAN_Config_2->addWidget(comboBox_2);


        verticalLayout_4->addLayout(CAN_Config_2);

        CAN_Config_3 = new QHBoxLayout();
        CAN_Config_3->setSpacing(2);
        CAN_Config_3->setObjectName(QString::fromUtf8("CAN_Config_3"));
        CAN_Config_3->setContentsMargins(20, -1, 20, -1);
        label_24 = new QLabel(horizontalLayoutWidget_16);
        label_24->setObjectName(QString::fromUtf8("label_24"));

        CAN_Config_3->addWidget(label_24);

        comboBox_3 = new QComboBox(horizontalLayoutWidget_16);
        comboBox_3->setObjectName(QString::fromUtf8("comboBox_3"));

        CAN_Config_3->addWidget(comboBox_3);


        verticalLayout_4->addLayout(CAN_Config_3);


        verticalLayout_5->addLayout(verticalLayout_4);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        can_connect = new QPushButton(horizontalLayoutWidget_16);
        can_connect->setObjectName(QString::fromUtf8("can_connect"));

        horizontalLayout_11->addWidget(can_connect);

        can_disconnect = new QPushButton(horizontalLayoutWidget_16);
        can_disconnect->setObjectName(QString::fromUtf8("can_disconnect"));

        horizontalLayout_11->addWidget(can_disconnect);


        verticalLayout_5->addLayout(horizontalLayout_11);

        verticalLayout_5->setStretch(0, 3);
        verticalLayout_5->setStretch(1, 1);

        horizontalLayout_12->addLayout(verticalLayout_5);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        dischargemos_open = new QPushButton(horizontalLayoutWidget_16);
        dischargemos_open->setObjectName(QString::fromUtf8("dischargemos_open"));

        horizontalLayout_2->addWidget(dischargemos_open);

        dischargemos_close = new QPushButton(horizontalLayoutWidget_16);
        dischargemos_close->setObjectName(QString::fromUtf8("dischargemos_close"));

        horizontalLayout_2->addWidget(dischargemos_close);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        chargemos_close = new QPushButton(horizontalLayoutWidget_16);
        chargemos_close->setObjectName(QString::fromUtf8("chargemos_close"));

        horizontalLayout->addWidget(chargemos_close);

        chargemos_open = new QPushButton(horizontalLayoutWidget_16);
        chargemos_open->setObjectName(QString::fromUtf8("chargemos_open"));

        horizontalLayout->addWidget(chargemos_open);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
        dischargemos_open_2 = new QPushButton(horizontalLayoutWidget_16);
        dischargemos_open_2->setObjectName(QString::fromUtf8("dischargemos_open_2"));

        horizontalLayout_14->addWidget(dischargemos_open_2);

        dischargemos_close_2 = new QPushButton(horizontalLayoutWidget_16);
        dischargemos_close_2->setObjectName(QString::fromUtf8("dischargemos_close_2"));

        horizontalLayout_14->addWidget(dischargemos_close_2);


        verticalLayout->addLayout(horizontalLayout_14);


        horizontalLayout_12->addLayout(verticalLayout);


        verticalLayout_6->addLayout(horizontalLayout_12);

        verticalLayout_6->setStretch(0, 5);

        verticalLayout_7->addLayout(verticalLayout_6);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(15, -1, 15, -1);
        label_9 = new QLabel(horizontalLayoutWidget_16);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        horizontalLayout_4->addWidget(label_9);

        label_25 = new QLabel(horizontalLayoutWidget_16);
        label_25->setObjectName(QString::fromUtf8("label_25"));

        horizontalLayout_4->addWidget(label_25);

        label_10 = new QLabel(horizontalLayoutWidget_16);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        horizontalLayout_4->addWidget(label_10);

        label_11 = new QLabel(horizontalLayoutWidget_16);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        horizontalLayout_4->addWidget(label_11);

        label_12 = new QLabel(horizontalLayoutWidget_16);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        horizontalLayout_4->addWidget(label_12);

        label_13 = new QLabel(horizontalLayoutWidget_16);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        horizontalLayout_4->addWidget(label_13);


        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(15, -1, 15, -1);
        label_14 = new QLabel(horizontalLayoutWidget_16);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        horizontalLayout_5->addWidget(label_14);

        label_26 = new QLabel(horizontalLayoutWidget_16);
        label_26->setObjectName(QString::fromUtf8("label_26"));

        horizontalLayout_5->addWidget(label_26);

        label_15 = new QLabel(horizontalLayoutWidget_16);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        horizontalLayout_5->addWidget(label_15);

        label_16 = new QLabel(horizontalLayoutWidget_16);
        label_16->setObjectName(QString::fromUtf8("label_16"));

        horizontalLayout_5->addWidget(label_16);

        label_17 = new QLabel(horizontalLayoutWidget_16);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        horizontalLayout_5->addWidget(label_17);

        label_18 = new QLabel(horizontalLayoutWidget_16);
        label_18->setObjectName(QString::fromUtf8("label_18"));

        horizontalLayout_5->addWidget(label_18);


        verticalLayout_2->addLayout(horizontalLayout_5);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(15, -1, 15, -1);
        label_3 = new QLabel(horizontalLayoutWidget_16);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_3->addWidget(label_3);

        label_27 = new QLabel(horizontalLayoutWidget_16);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        horizontalLayout_3->addWidget(label_27);

        label_6 = new QLabel(horizontalLayoutWidget_16);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        horizontalLayout_3->addWidget(label_6);

        label_8 = new QLabel(horizontalLayoutWidget_16);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        horizontalLayout_3->addWidget(label_8);

        label_7 = new QLabel(horizontalLayoutWidget_16);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        horizontalLayout_3->addWidget(label_7);

        label_4 = new QLabel(horizontalLayoutWidget_16);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_3->addWidget(label_4);


        verticalLayout_2->addLayout(horizontalLayout_3);


        horizontalLayout_6->addLayout(verticalLayout_2);

        horizontalLayout_6->setStretch(0, 5);

        horizontalLayout_10->addLayout(horizontalLayout_6);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        horizontalLayout_8->setContentsMargins(15, -1, 15, -1);
        label_20 = new QLabel(horizontalLayoutWidget_16);
        label_20->setObjectName(QString::fromUtf8("label_20"));

        horizontalLayout_8->addWidget(label_20);

        label_21 = new QLabel(horizontalLayoutWidget_16);
        label_21->setObjectName(QString::fromUtf8("label_21"));

        horizontalLayout_8->addWidget(label_21);


        verticalLayout_3->addLayout(horizontalLayout_8);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(15, -1, 15, -1);
        label_19 = new QLabel(horizontalLayoutWidget_16);
        label_19->setObjectName(QString::fromUtf8("label_19"));

        horizontalLayout_7->addWidget(label_19);

        label_2 = new QLabel(horizontalLayoutWidget_16);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_7->addWidget(label_2);


        verticalLayout_3->addLayout(horizontalLayout_7);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        horizontalLayout_9->setContentsMargins(15, -1, 15, -1);
        label_22 = new QLabel(horizontalLayoutWidget_16);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        horizontalLayout_9->addWidget(label_22);

        label_23 = new QLabel(horizontalLayoutWidget_16);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        horizontalLayout_9->addWidget(label_23);


        verticalLayout_3->addLayout(horizontalLayout_9);


        horizontalLayout_10->addLayout(verticalLayout_3);

        horizontalLayout_10->setStretch(0, 6);
        horizontalLayout_10->setStretch(1, 2);

        verticalLayout_7->addLayout(horizontalLayout_10);

        verticalLayout_7->setStretch(0, 5);
        verticalLayout_7->setStretch(1, 3);

        horizontalLayout_13->addLayout(verticalLayout_7);

        rizhi = new QPlainTextEdit(horizontalLayoutWidget_16);
        rizhi->setObjectName(QString::fromUtf8("rizhi"));

        horizontalLayout_13->addWidget(rizhi);

        horizontalLayout_13->setStretch(0, 8);
        horizontalLayout_13->setStretch(1, 5);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1032, 23));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QApplication::translate("MainWindow", "CAN_Device", nullptr));
        label_5->setText(QApplication::translate("MainWindow", "CAN_Baut", nullptr));
        label_24->setText(QApplication::translate("MainWindow", "\345\270\247\346\240\274\345\274\217", nullptr));
        can_connect->setText(QApplication::translate("MainWindow", "connect", nullptr));
        can_disconnect->setText(QApplication::translate("MainWindow", "disconnect", nullptr));
        dischargemos_open->setText(QApplication::translate("MainWindow", "dischargemos_open", nullptr));
        dischargemos_close->setText(QApplication::translate("MainWindow", "dischargemos_close", nullptr));
        chargemos_close->setText(QApplication::translate("MainWindow", "chargemos_close", nullptr));
        chargemos_open->setText(QApplication::translate("MainWindow", "chargemos_open", nullptr));
        dischargemos_open_2->setText(QApplication::translate("MainWindow", "clear_fault", nullptr));
        dischargemos_close_2->setText(QApplication::translate("MainWindow", "reserve", nullptr));
        label_9->setText(QApplication::translate("MainWindow", "String1", nullptr));
        label_25->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_10->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_11->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_12->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_13->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_14->setText(QApplication::translate("MainWindow", "String2", nullptr));
        label_26->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_15->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_16->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_17->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_18->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_3->setText(QApplication::translate("MainWindow", "String3", nullptr));
        label_27->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_6->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_8->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_7->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_4->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_20->setText(QApplication::translate("MainWindow", "Current", nullptr));
        label_21->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_19->setText(QApplication::translate("MainWindow", "Total Votage", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        label_22->setText(QApplication::translate("MainWindow", "SOC", nullptr));
        label_23->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
