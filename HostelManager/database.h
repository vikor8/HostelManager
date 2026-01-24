#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <QDate>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool initializeDatabase();
    bool isDatabaseConnected() const { return db.isOpen(); }
    QSqlDatabase getDatabase() const { return db; }

    // Методы для работы с комнатами
    bool addRoom(const QString& roomNumber, const QString& category, int bedsCount);
    bool removeRoom(int roomId);
    QList<QString> getAllRooms();

    // Методы для работы с кроватями
    bool addBed(int roomId, int bedNumber, double pricePerDay);
    bool removeBed(int bedId);

    // Методы для работы с клиентами
    bool addClient(const QString& firstName, const QString& lastName,
                   const QString& passport, const QString& phone,
                   const QString& email);

    // Методы для работы с бронированиями
    bool addBooking(int bedId, int clientId, const QDate& checkInDate,
                   const QDate& checkOutDate, double totalPrice);
    bool removeBooking(int bookingId);

    // Методы для проверки доступности
    bool isBedAvailable(int bedId, const QDate& checkInDate, const QDate& checkOutDate);
    QList<int> getAvailableBeds(const QDate& checkInDate, const QDate& checkOutDate);

    // Методы для получения данных
    QList<QString> getBookingsForPeriod(const QDate& startDate, const QDate& endDate);
    QMap<QString, QString> getRoomDetails(int roomId);
    double getBedPrice(int bedId);

    // Методы для статистики
    double calculateRevenue(const QDate& startDate, const QDate& endDate);
    double calculateOccupancyRate(const QDate& startDate, const QDate& endDate);

private:
    QSqlDatabase db;
    QString databasePath;

    void createTables();
    void createTriggers();
    void createViews();
    void seedTestData();
};

#endif // DATABASE_H
