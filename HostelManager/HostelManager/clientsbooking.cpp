#include "clientsbooking.h"
#include <QMessageBox>
#include <QDebug>

ClientsBooking::ClientsBooking(QObject *parent) : QObject(parent) {}

void ClientsBooking::showNotImplemented(const QString &actionName)
{
    QMessageBox::information(nullptr, "В разработке",
        QString("Функция '%1' находится в разработке.").arg(actionName));
}

void ClientsBooking::onClientAdd()
{
    showNotImplemented("Добавить клиента");
    qDebug() << "ClientsBooking: Добавить клиента";
}

void ClientsBooking::onClientView()
{
    QMessageBox::information(nullptr, "Список клиентов",
        "Отображение списка клиентов...");
    qDebug() << "ClientsBooking: Просмотр клиентов";
}

void ClientsBooking::onClientEdit()
{
    showNotImplemented("Редактировать клиента");
    qDebug() << "ClientsBooking: Редактировать клиента";
}

void ClientsBooking::onClientDelete()
{
    QMessageBox::warning(nullptr, "Удаление клиента",
        "Функция удаления клиента требует подтверждения администратора.");
    qDebug() << "ClientsBooking: Удалить клиента";
}

void ClientsBooking::onBookingNew()
{
    QMessageBox::information(nullptr, "Новое бронирование",
        "Создание нового бронирования...");
    qDebug() << "ClientsBooking: Новое бронирование";
}

void ClientsBooking::onBookingView()
{
    QMessageBox::information(nullptr, "Список бронирований",
        "Отображение списка бронирований...");
    qDebug() << "ClientsBooking: Просмотр бронирований";
}

void ClientsBooking::onBookingEdit()
{
    showNotImplemented("Изменить бронирование");
    qDebug() << "ClientsBooking: Изменить бронирование";
}

void ClientsBooking::onBookingCancel()
{
    QMessageBox::warning(nullptr, "Отмена бронирования",
        "Вы уверены, что хотите отменить бронирование?");
    qDebug() << "ClientsBooking: Отменить бронирование";
}

void ClientsBooking::onBookingCheckIn()
{
    QMessageBox::information(nullptr, "Заселение",
        "Процедура заселения гостя...");
    qDebug() << "ClientsBooking: Заселение";
}

void ClientsBooking::onBookingCheckOut()
{
    QMessageBox::information(nullptr, "Выселение",
        "Процедура выселения гостя...");
    qDebug() << "ClientsBooking: Выселение";
}
