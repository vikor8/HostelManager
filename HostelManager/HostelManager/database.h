#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <QDate>
#include <QColor>
#include <QVariantMap>

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

    // Методы для работы с категориями
    bool addCategory(const QString& categoryName, const QString& color = "#FFFFFF");
    bool removeCategory(const QString& categoryName);
    bool updateCategoryColor(const QString& categoryName, const QString& color);
    QList<QPair<QString, QString>> getAllCategories();
    QString getCategoryColor(const QString& categoryName);

    // Методы для работы с кроватями
    bool addBed(int roomId, int bedNumber, double pricePerDay);
    bool removeBed(int bedId);

    // Методы для работы с клиентами
    bool addClient(const QString& firstName, const QString& lastName,
                   const QString& passport, const QString& phone,
                   const QString& email);
    bool addClient(const QString& firstName, const QString& lastName,
                  const QString& middleName, const QString& passport,
                  const QString& phone, const QDate& birthDate,
                  const QString& country);
    bool updateClient(int clientId, const QString& firstName, const QString& lastName,
                     const QString& middleName, const QString& passport,
                     const QString& phone, const QDate& birthDate,
                     const QString& country);
    bool deleteClient(int clientId);
    QList<QVariantMap> getAllClients();
    QVariantMap getClientById(int clientId);
    bool clientExists(const QString& passport);

    // Методы для работы с бронированиями
    bool addBooking(int bedId, int clientId, const QDate& checkInDate,
                   const QDate& checkOutDate, double totalPrice,
                   double paidAmount = 0, const QString& paymentMethod = "Наличные");
    bool removeBooking(int bookingId);
        bool cancelBooking(int bookingId);

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


    QVariantMap getBookingInfo(const QString& roomNumber, int bedNumber, const QDate& date);

    // Методы для работы с оплатой
    bool updatePayment(int bookingId, double paidAmount, const QString& paymentMethod = QString());
    QVariantMap getBookingInfoByDate(const QString& roomNumber, int bedNumber, const QDate& date);

    QList<QVariantMap> getPaymentReport(const QDate& startDate, const QDate& endDate);

    bool addRoomBooking(int roomId, int clientId, const QDate& checkInDate,
                       const QDate& checkOutDate, double totalPrice,
                       double paidAmount = 0, const QString& paymentMethod = "Наличные");


private:
    QSqlDatabase db;
    QString databasePath;

    void createTables();
    void createTriggers();
    void createViews();
    void seedTestData();
};

#endif // DATABASE_H
