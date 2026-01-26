#include "statistics.h"
#include <QMessageBox>
#include <QDebug>

Statistics::Statistics(QObject *parent) : QObject(parent) {}

void Statistics::showNotImplemented(const QString &actionName)
{
    QMessageBox::information(nullptr, "В разработке",
        QString("Функция '%1' находится в разработке.").arg(actionName));
}

void Statistics::onServiceAdd()
{
    showNotImplemented("Добавить услугу");
    qDebug() << "Statistics: Добавить услугу";
}

void Statistics::onServiceView()
{
    QMessageBox::information(nullptr, "Список услуг",
        "Отображение списка дополнительных услуг...");
    qDebug() << "Statistics: Просмотр услуг";
}

void Statistics::onServiceAssign()
{
    showNotImplemented("Назначить услугу");
    qDebug() << "Statistics: Назначить услугу";
}

void Statistics::onStatsOccupancy()
{
    QMessageBox::information(nullptr, "Статистика загрузки",
        "Статистика загрузки номеров за выбранный период...");
    qDebug() << "Statistics: Статистика загрузки";
}

void Statistics::onStatsRevenue()
{
    QMessageBox::information(nullptr, "Финансовая статистика",
        "Финансовая статистика за выбранный период...");
    qDebug() << "Statistics: Финансовая статистика";
}

void Statistics::onStatsClients()
{
    QMessageBox::information(nullptr, "Статистика клиентов",
        "Статистика по клиентам: активность, предпочтения и т.д.");
    qDebug() << "Statistics: Статистика клиентов";
}
