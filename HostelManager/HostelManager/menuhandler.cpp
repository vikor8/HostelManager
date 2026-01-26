#include "menuhandler.h"
#include "hostelmanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QApplication>  // Добавляем этот заголовок для qApp

MenuHandler::MenuHandler(QObject *parent) : QObject(parent), mainWindow(nullptr) {}

// Реализация методов меню "Файл"
void MenuHandler::onFileNew()
{
    QMessageBox::information(nullptr, "Новый", "Создание нового файла...");
    qDebug() << "Меню: Файл -> Новый";
}

void MenuHandler::onFileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Открыть файл", "", "Файлы HostelManager (*.hm)");
    if (!fileName.isEmpty()) {
        QMessageBox::information(nullptr, "Открыть", "Открытие файла: " + fileName);
        qDebug() << "Меню: Файл -> Открыть:" << fileName;
    }
}

void MenuHandler::onFileSave()
{
    QMessageBox::information(nullptr, "Сохранить", "Сохранение файла...");
    qDebug() << "Меню: Файл -> Сохранить";
}

void MenuHandler::onFileExport()
{
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Экспорт в Excel", "", "Excel Files (*.xlsx)");
    if (!fileName.isEmpty()) {
        QMessageBox::information(nullptr, "Экспорт", "Экспорт в Excel: " + fileName);
        qDebug() << "Меню: Файл -> Экспорт в Excel:" << fileName;
    }
}

void MenuHandler::onFilePrint()
{
    QMessageBox::information(nullptr, "Печать", "Печать отчета...");
    qDebug() << "Меню: Файл -> Печать";
}

void MenuHandler::onFileExit()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "Выход", "Вы уверены, что хотите выйти?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "Меню: Файл -> Выход";
        qApp->quit();  // Теперь qApp будет доступен
    }
}

// ... остальные методы без изменений ...
