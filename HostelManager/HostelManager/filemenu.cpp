#include "filemenu.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QDebug>

FileMenu::FileMenu(QObject *parent) : QObject(parent) {}

void FileMenu::showNotImplemented(const QString &actionName)
{
    QMessageBox::information(nullptr, "В разработке",
        QString("Функция '%1' находится в разработке.").arg(actionName));
}

void FileMenu::onFileNew()
{
    showNotImplemented("Новый файл");
    qDebug() << "FileMenu: Новый файл";
}

void FileMenu::onFileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Открыть файл", "", "Все файлы (*)");
    if (!fileName.isEmpty()) {
        QMessageBox::information(nullptr, "Открыть файл",
            QString("Открытие файла: %1").arg(fileName));
        qDebug() << "FileMenu: Открыть файл:" << fileName;
    }
}

void FileMenu::onFileSave()
{
    showNotImplemented("Сохранить");
    qDebug() << "FileMenu: Сохранить";
}

void FileMenu::onFileExport()
{
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Экспорт данных",
        "export.xlsx", "Excel файлы (*.xlsx);;CSV файлы (*.csv)");
    if (!fileName.isEmpty()) {
        QMessageBox::information(nullptr, "Экспорт",
            QString("Экспорт в файл: %1").arg(fileName));
        qDebug() << "FileMenu: Экспорт в:" << fileName;
    }
}

void FileMenu::onFilePrint()
{
    showNotImplemented("Печать");
    qDebug() << "FileMenu: Печать";
}

void FileMenu::onFileExit()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "Выход",
        "Вы уверены, что хотите выйти из программы?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        qDebug() << "FileMenu: Выход из программы";
        qApp->quit();
    }
}

void FileMenu::onHelpAbout()
{
    QMessageBox::about(nullptr, "О программе Hostel Manager",
        "<h2>Hostel Manager</h2>"
        "<p>Версия 1.0.0</p>"
        "<p>Система управления бронированиями хостела</p>"
        "<p>Разработано с использованием Qt 6</p>"
        "<p>© 2024 Все права защищены</p>");
    qDebug() << "FileMenu: О программе";
}

void FileMenu::onHelpAboutQt()
{
    QMessageBox::aboutQt(nullptr, "О Qt");
    qDebug() << "FileMenu: О Qt";
}

void FileMenu::onHelpContents()
{
    QMessageBox::information(nullptr, "Справка",
        "Для получения справки обратитесь к документации.");
    qDebug() << "FileMenu: Справка";
}
