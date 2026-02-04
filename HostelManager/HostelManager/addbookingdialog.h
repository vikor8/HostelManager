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
#include <QRadioButton>
#include <QButtonGroup>

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
    QString paymentMethod() const;
    double paidAmount() const { return paidSpin->value(); } // Новый геттер для оплаченной суммы

    // Проверка доступности койки
    bool isBedAvailable() const;

    //  методы для предварительного заполнения
    void setRoomAndBed(int roomId, int bedId);
    void setDates(const QDate &checkIn, const QDate &checkOut);

private slots:
    void loadClients();
    void loadRooms();
    void loadBeds();
    void calculateTotalPrice();
    void updatePriceFromDatabase();
    void validateDates();
    void onAccept();
    void validateForm();
    void updatePaidAmount(); // Новый слот для обновления оплаченной суммы
    void onAddClientButtonClicked();

private:
    Database *database;

    QComboBox *clientCombo;
    QDateEdit *checkInEdit;
    QDateEdit *checkOutEdit;
    QComboBox *roomCombo;
    QComboBox *bedCombo;
    QDoubleSpinBox *priceSpin;
    QDoubleSpinBox *paidSpin; // Новое поле для оплаченной суммы
    QLabel *totalPriceLabel;
    QLabel *balanceLabel; // Метка для отображения остатка

    // Новые элементы для оплаты
    QRadioButton *cashRadio;
    QRadioButton *cardRadio;
    QRadioButton *transferRadio;
    QButtonGroup *paymentGroup;

    QPushButton *addClientButton;

    QMap<int, QString> clientMap; // clientId -> displayName
    QMap<int, QString> roomMap;   // roomId -> roomNumber
    QMap<int, QString> bedMap;    // bedId -> displayName

    void setupUi();
    void populateClientCombo(const QString& filter = QString());
    void populateRoomCombo();
    void populateBedCombo(int roomId);

};

#endif // ADDBOOKINGDIALOG_H
