#ifndef EDITBOOKINGDIALOG_H
#define EDITBOOKINGDIALOG_H

#include <QDialog>
#include <QDate>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>

class Database;

class EditBookingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditBookingDialog(Database *db, int bookingId, QWidget *parent = nullptr);
    ~EditBookingDialog();

    // Геттеры для данных бронирования
    int clientId() const;
    int roomId() const;
    int bedId() const;
    QDate checkInDate() const { return checkInEdit->date(); }
    QDate checkOutDate() const { return checkOutEdit->date(); }
    double pricePerDay() const { return priceSpin->value(); }
    double totalPrice() const;
    QString paymentMethod() const;
    double paidAmount() const { return paidSpin->value(); }

private slots:
    void loadBookingData();
    void loadClients();
    void loadRooms();
    void loadBeds();
    void calculateTotalPrice();
    void updatePriceFromDatabase();
    void validateDates();
    void onAccept();
    void validateForm();
    void updatePaidAmount();

private:
    Database *database;
    int bookingId;

    QComboBox *clientCombo;
    QDateEdit *checkInEdit;
    QDateEdit *checkOutEdit;
    QComboBox *roomCombo;
    QComboBox *bedCombo;
    QDoubleSpinBox *priceSpin;
    QDoubleSpinBox *paidSpin;
    QLabel *totalPriceLabel;
    QLabel *balanceLabel;

    // Элементы для оплаты
    QRadioButton *cashRadio;
    QRadioButton *cardRadio;
    QRadioButton *transferRadio;
    QButtonGroup *paymentGroup;

    QMap<int, QString> clientMap;
    QMap<int, QString> roomMap;
    QMap<int, QString> bedMap;

    void setupUi();
    void populateClientCombo(const QString& filter = QString());
    void populateRoomCombo();
    void populateBedCombo(int roomId);
    bool checkBedAvailabilityExceptCurrent();
};

#endif // EDITBOOKINGDIALOG_H
