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
    enum BookingType { Place, WholeRoom }; // Тип бронирования: Место или Комната целиком

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
    double paidAmount() const { return paidSpin->value(); }
    BookingType bookingType() const; // Новый геттер для типа бронирования
    QList<int> getAllBedsInRoom() const; // Возвращает список ID всех коек в комнате


    // Проверка доступности койки или комнаты
    bool isBedAvailable() const;
    bool isRoomAvailable() const; // Новый метод для проверки доступности комнаты

    // методы для предварительного заполнения
    void setRoomAndBed(int roomId, int bedId);
    void setDates(const QDate &checkIn, const QDate &checkOut);
    void setBookingType(BookingType type);
private slots:
    void loadClients();
    void loadRooms();
    void loadBeds();
    void calculateTotalPrice();
    void updatePriceFromDatabase();
    void validateDates();
    void onAccept();
    void validateForm();
    void updatePaidAmount();
    void onAddClientButtonClicked();
    void onBookingTypeChanged(); // Новый слот для изменения типа бронирования

private:
    Database *database;

    QComboBox *clientCombo;
    QDateEdit *checkInEdit;
    QDateEdit *checkOutEdit;
    QComboBox *roomCombo;
    QComboBox *bedCombo;
    QDoubleSpinBox *priceSpin;
    QDoubleSpinBox *paidSpin;
    QLabel *totalPriceLabel;
    QLabel *balanceLabel;
    QLabel *daysValueLabel;
    QLabel *pricePerDayValueLabel;

    // Новые элементы для выбора типа бронирования
    QRadioButton *placeRadio;
    QRadioButton *wholeRoomRadio;
    QButtonGroup *bookingTypeGroup;
    BookingType currentBookingType;

    // Элементы для оплаты
    QRadioButton *cashRadio;
    QRadioButton *cardRadio;
    QRadioButton *transferRadio;
    QRadioButton *legalEntityRadio; // Новый способ оплаты "на р/с юрлица"
    QButtonGroup *paymentGroup;

    QPushButton *addClientButton;

    QMap<int, QString> clientMap; // clientId -> displayName
    QMap<int, QString> roomMap;   // roomId -> roomNumber
    QMap<int, QString> bedMap;    // bedId -> displayName

    void setupUi();
    void populateClientCombo(const QString& filter = QString());
    void populateRoomCombo();
    void populateBedCombo(int roomId);
    void updateBedComboVisibility(); // Новый метод для управления видимостью поля с койками
    void updatePaymentInfoPanel();// Новый метод для обновления информационной панели

    // Вспомогательные методы для расчета цены
    double getRoomTotalPrice() const; // Общая стоимость для всей комнаты
};

#endif // ADDBOOKINGDIALOG_H
