#ifndef ADDBOOKINGDIALOG_H
#define ADDBOOKINGDIALOG_H

#include <QDialog>
#include <QDate>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

class Database; // Forward declaration

class AddBookingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddBookingDialog(Database *db, QWidget *parent = nullptr);
    ~AddBookingDialog();

    // Геттеры для данных бронирования
    int clientId() const;
    int roomId() const;
    int bedId() const;
    QDate checkInDate() const { return checkInEdit->date(); }
    QDate checkOutDate() const { return checkOutEdit->date(); }
    double pricePerDay() const { return priceSpin->value(); }
    double totalPrice() const;

    // Проверка доступности койки
    bool isBedAvailable() const;

private slots:
    void loadClients();
    void loadRooms();
    void loadBeds();
    void calculateTotalPrice();
    void updatePriceFromDatabase();
    void validateDates();
    void onAccept();
    void validateForm();

private:
    Database *database;

    QComboBox *clientCombo;
    QDateEdit *checkInEdit;
    QDateEdit *checkOutEdit;
    QComboBox *roomCombo;
    QComboBox *bedCombo;
    QDoubleSpinBox *priceSpin;
    QLabel *totalPriceLabel;

    QMap<int, QString> clientMap; // clientId -> displayName
    QMap<int, QString> roomMap;   // roomId -> roomNumber
    QMap<int, QString> bedMap;    // bedId -> displayName

    void setupUi();
    void populateClientCombo(const QString& filter = QString());
    void populateRoomCombo();
    void populateBedCombo(int roomId);
};

#endif // ADDBOOKINGDIALOG_H
