#ifndef CLIENTSBOOKING_H
#define CLIENTSBOOKING_H

#include <QObject>

class ClientsBooking : public QObject
{
    Q_OBJECT

public:
    explicit ClientsBooking(QObject *parent = nullptr);

public slots:
    // Клиенты
    void onClientAdd();
    void onClientView();
    void onClientEdit();
    void onClientDelete();

    // Бронирование
    void onBookingNew();
    void onBookingView();
    void onBookingEdit();
    void onBookingCancel();
    void onBookingCheckIn();
    void onBookingCheckOut();

private:
    void showNotImplemented(const QString &actionName);
};

#endif // CLIENTSBOOKING_H
